#include "ExTransparent.h"

#include <Ren/Context.h>

#include "../../utils/ShaderLoader.h"
#include "../Renderer_Structs.h"
#include "../framegraph/FgBuilder.h"

void Eng::ExTransparent::Execute(const FgContext &fg) {
    const Ren::ImageRWHandle color_tex = fg.AccessRWImage(color_tex_);
    const Ren::ImageRWHandle normal_tex = fg.AccessRWImage(normal_tex_);
    const Ren::ImageRWHandle spec_tex = fg.AccessRWImage(spec_tex_);
    const Ren::ImageRWHandle depth_tex = fg.AccessRWImage(depth_tex_);

    LazyInit(fg.ren_ctx(), fg.sh(), color_tex, normal_tex, spec_tex, depth_tex);
    DrawTransparent(fg, color_tex, normal_tex, spec_tex, depth_tex);
}

void Eng::ExTransparent::DrawTransparent(const FgContext &fg, const Ren::ImageRWHandle color_tex,
                                         const Ren::ImageRWHandle normal_tex, const Ren::ImageRWHandle spec_tex,
                                         const Ren::ImageRWHandle depth_tex) {
    const Ren::BufferROHandle instances_buf = fg.AccessROBuffer(instances_buf_);
    const Ren::BufferROHandle instance_indices_buf = fg.AccessROBuffer(instance_indices_buf_);
    const Ren::BufferROHandle unif_shared_data_buf = fg.AccessROBuffer(shared_data_buf_);
    const Ren::BufferROHandle materials_buf = fg.AccessROBuffer(materials_buf_);
    const Ren::BufferROHandle cells_buf = fg.AccessROBuffer(cells_buf_);
    const Ren::BufferROHandle items_buf = fg.AccessROBuffer(items_buf_);
    const Ren::BufferROHandle lights_buf = fg.AccessROBuffer(lights_buf_);
    const Ren::BufferROHandle decals_buf = fg.AccessROBuffer(decals_buf_);

    const Ren::ImageROHandle shad_tex = fg.AccessROImage(shadow_depth_);
    const Ren::ImageROHandle ssao_tex = fg.AccessROImage(ssao_tex_);

    DrawTransparent_Simple(fg, instances_buf, instance_indices_buf, unif_shared_data_buf, materials_buf, cells_buf,
                           items_buf, lights_buf, decals_buf, shad_tex, color_tex, normal_tex, spec_tex, depth_tex,
                           ssao_tex);
}

void Eng::ExTransparent::LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, const Ren::ImageRWHandle color_tex,
                                  const Ren::ImageRWHandle normal_tex, const Ren::ImageRWHandle spec_tex,
                                  const Ren::ImageRWHandle depth_tex) {
    const Ren::RenderTarget color_targets[] = {{color_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store},
                                               {normal_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store},
                                               {spec_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store}};
    const Ren::RenderTarget depth_target = {depth_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store, Ren::eLoadOp::Load,
                                            Ren::eStoreOp::Store};

    if (!initialized) {
        rp_transparent_ = sh.FindOrCreateRenderPass(depth_target, color_targets);

        [[maybe_unused]] const int buf1_stride = 16, buf2_stride = 16;

        { // VertexInput for main drawing (uses all attributes)
            const Ren::VtxAttribDesc attribs[] = {
                {0, VTX_POS_LOC, 3, Ren::eType::Float32, buf1_stride, 0, 0},
                {0, VTX_UV1_LOC, 2, Ren::eType::Float16, buf1_stride, 0, 3 * sizeof(float)},
                {1, VTX_NOR_LOC, 4, Ren::eType::Int16_snorm, buf1_stride, 0, 0},
                {1, VTX_TAN_LOC, 2, Ren::eType::Int16_snorm, buf1_stride, 0, 4 * sizeof(uint16_t)},
                {1, VTX_AUX_LOC, 1, Ren::eType::Uint32, buf1_stride, 0, 6 * sizeof(uint16_t)}};
            draw_pass_vi_ = sh.FindOrCreateVertexInput(attribs);
        }

#if defined(REN_VK_BACKEND)
        InitDescrSetLayout();
#endif

        initialized = true;
    }

    /*if (moments_b0_.id && moments_z_and_z2_.id && moments_z3_and_z4_.id) {
        const Ren::ImgHandle attachments[] = {moments_b0_, moments_z_and_z2_,
                                              moments_z3_and_z4_};
        if (!moments_fb_.Setup(attachments, 3, depth_tex.ref->handle(), {},
                               view_state_->is_multisampled)) {
            ctx.log()->Error("ExTransparent: moments_fb_ init failed!");
        }
    }*/
}