#include "PrimDraw.h"

#include <Ren/ApiContext.h>
#include <Ren/Context.h>
#include <Sys/ScopeExit.h>

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

bool framebuffer_eq(const Ren::Framebuffer &fb, const Ren::RenderPassMain &rp, const Ren::WeakImgRef &depth_attachment,
                    const Ren::WeakImgRef &stencil_attachment,
                    const Ren::Span<const Ren::RenderTarget> color_attachments) {
    return !fb.Changed(rp, depth_attachment, stencil_attachment, color_attachments);
}
bool framebuffer_lt(const Ren::Framebuffer &fb, const Ren::RenderPassMain &rp, const Ren::WeakImgRef &depth_attachment,
                    const Ren::WeakImgRef &stencil_attachment,
                    const Ren::Span<const Ren::RenderTarget> color_attachments) {
    return fb.LessThan(rp, depth_attachment, stencil_attachment, color_attachments);
}
} // namespace PrimDrawInternal

bool Eng::PrimDraw::LazyInit(Ren::Context &ctx) {
    using namespace PrimDrawInternal;

    const Ren::ApiContext &api = ctx.api();

    const Ren::BufferHandle vtx_buf1 = ctx.default_vertex_buf1(), vtx_buf2 = ctx.default_vertex_buf2(),
                            ndx_buf = ctx.default_indices_buf();

    const auto &[vtx_buf1_main, vtx_buf1_cold] = ctx.buffers().Get(vtx_buf1);
    const auto &[vtx_buf2_main, vtx_buf2_cold] = ctx.buffers().Get(vtx_buf2);
    const auto &[ndx_buf_main, ndx_buf_cold] = ctx.buffers().Get(ndx_buf);

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
            quad_vtx1_ = Buffer_AllocSubRegion(api, vtx_buf1_main, vtx_buf1_cold, mem_required, 16, "quad", ctx.log(),
                                               &temp_stage_main, cmd_buf, current_mem_offset);
            quad_vtx2_ =
                Buffer_AllocSubRegion(api, vtx_buf2_main, vtx_buf2_cold, mem_required, 16, "quad", ctx.log(), nullptr);
            assert(quad_vtx1_.offset == quad_vtx2_.offset && "Offsets do not match!");
            current_mem_offset += mem_required;
        }
        { // Allocate quad indices
            const uint32_t mem_required = sizeof(fs_quad_indices);
            quad_ndx_ = Buffer_AllocSubRegion(api, ndx_buf_main, ndx_buf_cold, mem_required, 4, "quad", ctx.log(),
                                              &temp_stage_main, cmd_buf, current_mem_offset);
            current_mem_offset += mem_required;
        }
        { // Allocate sphere positions
            const uint32_t mem_required = sizeof(__sphere_positions);
            sphere_vtx1_ = Buffer_AllocSubRegion(api, vtx_buf1_main, vtx_buf1_cold, mem_required, 16, "sphere",
                                                 ctx.log(), &temp_stage_main, cmd_buf, current_mem_offset);
            sphere_vtx2_ = Buffer_AllocSubRegion(api, vtx_buf2_main, vtx_buf2_cold, mem_required, 16, "sphere",
                                                 ctx.log(), nullptr);
            assert(sphere_vtx1_.offset == sphere_vtx2_.offset && "Offsets do not match!");
            current_mem_offset += mem_required;
        }
        { // Allocate sphere indices
            const uint32_t mem_required = sizeof(__sphere_indices);
            sphere_ndx_ = Buffer_AllocSubRegion(api, ndx_buf_main, ndx_buf_cold, mem_required, 4, "sphere", ctx.log(),
                                                &temp_stage_main, cmd_buf, current_mem_offset);
            current_mem_offset += mem_required;
        }
        assert(current_mem_offset == total_mem_required);

        api.EndSingleTimeCommands(cmd_buf);

        ctx_ = &ctx;
        initialized_ = true;
    }

    { // setup quad vertices
        const Ren::VtxAttribDesc attribs[] = {
            {vtx_buf1, VTX_POS_LOC, 2, Ren::eType::Float32, 0, quad_vtx1_.offset},
            {vtx_buf1, VTX_UV1_LOC, 2, Ren::eType::Float32, 0, uint32_t(quad_vtx1_.offset + 8 * sizeof(float))}};
        fs_quad_vtx_input_ = ctx_->FindOrCreateVertexInput(attribs, ndx_buf);
    }

    { // setup sphere vertices
        const Ren::VtxAttribDesc attribs[] = {{vtx_buf1, VTX_POS_LOC, 3, Ren::eType::Float32, 0, sphere_vtx1_.offset}};
        sphere_vtx_input_ = ctx_->FindOrCreateVertexInput(attribs, ndx_buf);
    }

    return true;
}

void Eng::PrimDraw::CleanUp() {
    using namespace PrimDrawInternal;

    const auto &[vtx_buf1_main, vtx_buf1_cold] = ctx_->buffers().Get(ctx_->default_vertex_buf1());
    const auto &[vtx_buf2_main, vtx_buf2_cold] = ctx_->buffers().Get(ctx_->default_vertex_buf2());
    const auto &[ndx_buf_main, ndx_buf_cold] = ctx_->buffers().Get(ctx_->default_indices_buf());

    if (quad_vtx1_) {
        Buffer_FreeSubRegion(vtx_buf1_cold, quad_vtx1_);
        assert(quad_vtx2_ && quad_ndx_);
        Buffer_FreeSubRegion(vtx_buf2_cold, quad_vtx2_);
        Buffer_FreeSubRegion(ndx_buf_cold, quad_ndx_);
        quad_vtx1_ = quad_vtx2_ = quad_ndx_ = {};
    }
    if (sphere_vtx1_) {
        Buffer_FreeSubRegion(vtx_buf1_cold, sphere_vtx1_);
        assert(sphere_vtx2_ && sphere_ndx_);
        Buffer_FreeSubRegion(vtx_buf2_cold, sphere_vtx2_);
        Buffer_FreeSubRegion(ndx_buf_cold, sphere_ndx_);
        sphere_vtx1_ = sphere_vtx2_ = sphere_ndx_;
    }

    if (fs_quad_vtx_input_) {
        ctx_->ReleaseVertexInput(fs_quad_vtx_input_);
        fs_quad_vtx_input_ = {};
    }
    if (sphere_vtx_input_) {
        ctx_->ReleaseVertexInput(sphere_vtx_input_);
        sphere_vtx_input_ = {};
    }
}

Eng::PrimDraw::~PrimDraw() { CleanUp(); }

const Ren::Framebuffer *Eng::PrimDraw::FindOrCreateFramebuffer(const Ren::RenderPassMain *rp,
                                                               const Ren::RenderTarget depth_target,
                                                               const Ren::RenderTarget stencil_target,
                                                               Ren::Span<const Ren::RenderTarget> color_targets) {
    using namespace PrimDrawInternal;

    Ren::WeakImgRef depth_ref = depth_target.ref, stencil_ref = stencil_target.ref;

    int start = 0, count = int(framebuffers_.size());
    while (count > 0) {
        const int step = count / 2;
        const int index = start + step;
        if (framebuffer_lt(framebuffers_[index], *rp, depth_ref, stencil_ref, color_targets)) {
            start = index + 1;
            count -= step + 1;
        } else {
            count = step;
        }
    }

    if (start < int(framebuffers_.size()) &&
        framebuffer_eq(framebuffers_[start], *rp, depth_ref, stencil_ref, color_targets)) {
        return &framebuffers_[start];
    }

    const Ren::ApiContext &api = ctx_->api();

    int w = -1, h = -1;

    for (const auto &rt : color_targets) {
        if (rt.ref) {
            w = rt.ref->params.w;
            h = rt.ref->params.h;
            break;
        }
    }

    if (w == -1 && depth_ref) {
        w = depth_ref->params.w;
        h = depth_ref->params.h;
    }

    if (w == -1 && stencil_ref) {
        w = stencil_ref->params.w;
        h = stencil_ref->params.h;
    }

    Ren::Framebuffer new_framebuffer;
    if (!new_framebuffer.Setup(&api, *rp, w, h, depth_target, stencil_target, color_targets, ctx_->log())) {
        ctx_->log()->Error("Failed to create framebuffer!");
        return nullptr;
    }
    framebuffers_.insert(begin(framebuffers_) + start, std::move(new_framebuffer));
    return &framebuffers_[start];
}
