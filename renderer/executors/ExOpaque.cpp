#include "ExOpaque.h"

#include <Ren/Context.h>

#include "../../utils/ShaderLoader.h"
#include "../Renderer_Structs.h"
#include "../framegraph/FgBuilder.h"

void Eng::ExOpaque::Execute(const FgContext &fg) {
    const Ren::ImageRWHandle color_tex = fg.AccessRWImage(color_tex_);
    const Ren::ImageRWHandle normal_tex = fg.AccessRWImage(normal_tex_);
    const Ren::ImageRWHandle spec_tex = fg.AccessRWImage(spec_tex_);
    const Ren::ImageRWHandle depth_tex = fg.AccessRWImage(depth_tex_);

    LazyInit(fg.ren_ctx(), fg.sh(), color_tex, normal_tex, spec_tex, depth_tex);
    DrawOpaque(fg, color_tex, normal_tex, spec_tex, depth_tex);
}

void Eng::ExOpaque::LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, const Ren::ImageRWHandle color_tex,
                             const Ren::ImageRWHandle normal_tex, const Ren::ImageRWHandle spec_tex,
                             const Ren::ImageRWHandle depth_tex) {
    const Ren::RenderTarget color_targets[] = {{color_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store},
                                               {normal_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store},
                                               {spec_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store}};
    const Ren::RenderTarget depth_target = {depth_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store, Ren::eLoadOp::Load,
                                            Ren::eStoreOp::Store};

    if (!initialized) {
        const int buf1_stride = 16, buf2_stride = 16;

        { // VertexInput for main drawing (uses all attributes)
            const Ren::VtxAttribDesc attribs[] = {
                {0, VTX_POS_LOC, 3, Ren::eType::Float32, buf1_stride, 0, 0},
                {0, VTX_UV1_LOC, 2, Ren::eType::Float16, buf1_stride, 0, 3 * sizeof(float)},
                {1, VTX_NOR_LOC, 4, Ren::eType::Int16_snorm, buf2_stride, 0, 0},
                {1, VTX_TAN_LOC, 2, Ren::eType::Int16_snorm, buf2_stride, 0, 4 * sizeof(uint16_t)},
                {1, VTX_AUX_LOC, 1, Ren::eType::Uint32, buf2_stride, 0, 6 * sizeof(uint16_t)}};
            draw_pass_vi_ = sh.FindOrCreateVertexInput(attribs);
        }

        rp_opaque_ = sh.FindOrCreateRenderPass(depth_target, color_targets);

#if defined(REN_VK_BACKEND)
        InitDescrSetLayout();
#endif

        initialized = true;
    }
}