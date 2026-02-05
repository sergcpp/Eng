#include "PrimDraw.h"

#include <Ren/ApiContext.h>
#include <Ren/Context.h>
#include <Ren/ResizableBuffer.h>
#include <Sys/ScopeExit.h>

#include "../utils/ShaderLoader.h"
#include "Renderer_Structs.h"

namespace PrimDrawInternal {
extern const float fs_quad_positions[] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
#if defined(REN_VK_BACKEND)
extern const float fs_quad_norm_uvs[] = {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
#else
extern const float fs_quad_norm_uvs[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
#endif
extern const uint16_t fs_quad_indices[] = {0, 1, 2, 0, 2, 3};
// const int TempBufSize = 256;
#include "precomputed/__sphere_mesh.inl"

extern const int SphereIndicesCount = __sphere_indices_count;
} // namespace PrimDrawInternal

Ren::FramebufferHandle
Eng::FramebufferPool::FindOrCreate(Ren::Context &ctx, Ren::RenderPassROHandle render_pass,
                                   const Ren::FramebufferAttachment &depth, const Ren::FramebufferAttachment &stencil,
                                   Ren::Span<const Ren::FramebufferAttachment> color_attachments) {
    const auto it =
        partition_point(std::begin(framebuffers_), std::end(framebuffers_), [&](const Ren::FramebufferHandle lhs) {
            const auto &[fb_main, fb_cold] = ctx.framebuffers().Get(lhs);
            return Framebuffer_LessThan(fb_main, fb_cold, render_pass, depth, stencil, color_attachments);
        });
    if (it != std::end(framebuffers_)) {
        const auto &[fb_main, fb_cold] = ctx.framebuffers().Get(*it);
        if (Framebuffer_Equals(fb_main, fb_cold, render_pass, depth, stencil, color_attachments)) {
            return *it;
        }
    }
    const Ren::FramebufferHandle ret = ctx.CreateFramebuffer(render_pass, depth, stencil, color_attachments);
    if (ret) {
        framebuffers_.insert(it, ret);
    }
    return ret;
}

Ren::FramebufferHandle Eng::FramebufferPool::FindOrCreate(Ren::Context &ctx, Ren::RenderPassROHandle render_pass,
                                                          const Ren::RenderTarget &depth,
                                                          const Ren::RenderTarget &stencil,
                                                          Ren::Span<const Ren::RenderTarget> _color_attachments) {
    Ren::SmallVector<Ren::FramebufferAttachment, 16> color_attachments;
    for (const Ren::RenderTarget &rt : _color_attachments) {
        color_attachments.push_back(rt);
    }
    return FindOrCreate(ctx, render_pass, depth, stencil, color_attachments);
}

Ren::FramebufferHandle Eng::FramebufferPool::FindOrCreate(Ren::Context &ctx, Ren::RenderPassROHandle render_pass,
                                                          Ren::ImageRWHandle depth, Ren::ImageRWHandle stencil,
                                                          Ren::Span<const Ren::ImageRWHandle> _color_attachments) {
    Ren::SmallVector<Ren::FramebufferAttachment, 16> color_attachments;
    for (const Ren::ImageRWHandle img : _color_attachments) {
        color_attachments.push_back({img, 0});
    }
    return FindOrCreate(ctx, render_pass, {depth}, {stencil}, color_attachments);
}

void Eng::FramebufferPool::Clear(Ren::Context &ctx) {
    for (const Ren::FramebufferHandle fb : framebuffers_) {
        ctx.ReleaseFramebuffer(fb);
    }
    framebuffers_.clear();
}

bool Eng::PrimDraw::LazyInit(ShaderLoader &sh) {
    using namespace PrimDrawInternal;

    Ren::Context &ctx = sh.ren_ctx();
    const Ren::ApiContext &api = ctx.api();

    Ren::ResizableBuffer &vtx_buf1 = ctx.default_vertex_buf1(), &vtx_buf2 = ctx.default_vertex_buf2(),
                         &ndx_buf = ctx.default_indices_buf();

    if (!initialized_) {
        uint32_t total_mem_required = sizeof(fs_quad_positions) + sizeof(fs_quad_norm_uvs);
        total_mem_required += sizeof(fs_quad_indices);
        total_mem_required += sizeof(__sphere_positions);
        total_mem_required += sizeof(__sphere_indices);

        Ren::BufferMain temp_stage_main = {};
        Ren::BufferCold temp_stage_cold = {};
        if (!Ren::Buffer_Init(api, temp_stage_main, temp_stage_cold, Ren::String{"Temp prim buf"},
                              Ren::eBufType::Upload, total_mem_required, ctx.log())) {
            return false;
        }
        uint8_t *mapped_ptr = Buffer_Map(api, temp_stage_main, temp_stage_cold);
        SCOPE_EXIT({ Ren::Buffer_DestroyImmediately(api, temp_stage_main, temp_stage_cold); })

        uint32_t stage_mem_offset = 0;
        { // copy quad vertices
            memcpy(mapped_ptr + stage_mem_offset, fs_quad_positions, sizeof(fs_quad_positions));
            stage_mem_offset += sizeof(fs_quad_positions);
            memcpy(mapped_ptr + stage_mem_offset, fs_quad_norm_uvs, sizeof(fs_quad_norm_uvs));
            stage_mem_offset += sizeof(fs_quad_norm_uvs);
        }
        { // copy quad indices
            memcpy(mapped_ptr + stage_mem_offset, fs_quad_indices, sizeof(fs_quad_indices));
            stage_mem_offset += sizeof(fs_quad_indices);
        }
        { // copy sphere positions
            memcpy(mapped_ptr + stage_mem_offset, __sphere_positions, sizeof(__sphere_positions));
            stage_mem_offset += sizeof(__sphere_positions);
        }
        { // copy sphere indices
            memcpy(mapped_ptr + stage_mem_offset, __sphere_indices, sizeof(__sphere_indices));
            stage_mem_offset += sizeof(__sphere_indices);
        }
        assert(stage_mem_offset == total_mem_required);

        Buffer_Unmap(api, temp_stage_main, temp_stage_cold);

        Ren::CommandBuffer cmd_buf = api.BegSingleTimeCommands();

        uint32_t current_mem_offset = 0;
        { // Allocate quad vertices
            const uint32_t mem_required = sizeof(fs_quad_positions) + sizeof(fs_quad_norm_uvs);
            quad_vtx1_ = vtx_buf1.AllocSubRegion(mem_required, 16, "quad", ctx.log(), &temp_stage_main, cmd_buf,
                                                 current_mem_offset);
            quad_vtx2_ = vtx_buf2.AllocSubRegion(mem_required, 16, "quad", ctx.log(), nullptr);
            assert(quad_vtx1_.offset == quad_vtx2_.offset && "Offsets do not match!");
            current_mem_offset += mem_required;
        }
        { // Allocate quad indices
            const uint32_t mem_required = sizeof(fs_quad_indices);
            quad_ndx_ = ndx_buf.AllocSubRegion(mem_required, 4, "quad", ctx.log(), &temp_stage_main, cmd_buf,
                                               current_mem_offset);
            current_mem_offset += mem_required;
        }
        { // Allocate sphere positions
            const uint32_t mem_required = sizeof(__sphere_positions);
            sphere_vtx1_ = vtx_buf1.AllocSubRegion(mem_required, 16, "sphere", ctx.log(), &temp_stage_main, cmd_buf,
                                                   current_mem_offset);
            sphere_vtx2_ = vtx_buf2.AllocSubRegion(mem_required, 16, "sphere", ctx.log(), nullptr);
            assert(sphere_vtx1_.offset == sphere_vtx2_.offset && "Offsets do not match!");
            current_mem_offset += mem_required;
        }
        { // Allocate sphere indices
            const uint32_t mem_required = sizeof(__sphere_indices);
            sphere_ndx_ = ndx_buf.AllocSubRegion(mem_required, 4, "sphere", ctx.log(), &temp_stage_main, cmd_buf,
                                                 current_mem_offset);
            current_mem_offset += mem_required;
        }
        assert(current_mem_offset == total_mem_required);

        api.EndSingleTimeCommands(cmd_buf);

        sh_ = &sh;
        initialized_ = true;
    }

    if (!fs_quad_vtx_input_) { // setup quad vertices
        const Ren::VtxAttribDesc attribs[] = {
            {0, VTX_POS_LOC, 2, Ren::eType::Float32, 0, quad_vtx1_.offset, 0},
            {0, VTX_UV1_LOC, 2, Ren::eType::Float32, 0, uint32_t(quad_vtx1_.offset + 8 * sizeof(float)), 0}};
        fs_quad_vtx_input_ = ctx.CreateVertexInput(attribs);
    }

    if (!sphere_vtx_input_) { // setup sphere vertices
        const Ren::VtxAttribDesc attribs[] = {{0, VTX_POS_LOC, 3, Ren::eType::Float32, 0, sphere_vtx1_.offset, 0}};
        sphere_vtx_input_ = ctx.CreateVertexInput(attribs);
    }

    return true;
}

void Eng::PrimDraw::CleanUp() {
    using namespace PrimDrawInternal;

    Ren::Context &ctx = sh_->ren_ctx();

    Ren::ResizableBuffer &vtx_buf1 = ctx.default_vertex_buf1(), &vtx_buf2 = ctx.default_vertex_buf2(),
                         &ndx_buf = ctx.default_indices_buf();

    if (quad_vtx1_) {
        vtx_buf1.FreeSubRegion(quad_vtx1_);
        assert(quad_vtx2_ && quad_ndx_);
        vtx_buf2.FreeSubRegion(quad_vtx2_);
        ndx_buf.FreeSubRegion(quad_ndx_);
        quad_vtx1_ = quad_vtx2_ = quad_ndx_ = {};
    }
    if (sphere_vtx1_) {
        vtx_buf1.FreeSubRegion(sphere_vtx1_);
        assert(sphere_vtx2_ && sphere_ndx_);
        vtx_buf2.FreeSubRegion(sphere_vtx2_);
        ndx_buf.FreeSubRegion(sphere_ndx_);
        sphere_vtx1_ = sphere_vtx2_ = sphere_ndx_;
    }

    ctx.ReleaseVertexInput(fs_quad_vtx_input_);
    fs_quad_vtx_input_ = {};

    ctx.ReleaseVertexInput(sphere_vtx_input_);
    sphere_vtx_input_ = {};

    framebuffers_.Clear(ctx);
}

Eng::PrimDraw::~PrimDraw() { CleanUp(); }
