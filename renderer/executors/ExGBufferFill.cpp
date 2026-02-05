#include "ExGBufferFill.h"

#include <Ren/Context.h>

#include "../../utils/ShaderLoader.h"
#include "../Renderer_Structs.h"
#include "../framegraph/FgBuilder.h"

void Eng::ExGBufferFill::Execute(const FgContext &fg) {
    const Ren::ImageRWHandle albedo_tex = fg.AccessRWImage(out_albedo_tex_);
    const Ren::ImageRWHandle normal_tex = fg.AccessRWImage(out_normal_tex_);
    const Ren::ImageRWHandle spec_tex = fg.AccessRWImage(out_spec_tex_);
    const Ren::ImageRWHandle depth_tex = fg.AccessRWImage(out_depth_tex_);

    LazyInit(fg.ren_ctx(), fg.sh(), albedo_tex, normal_tex, spec_tex, depth_tex);
    DrawOpaque(fg, albedo_tex, normal_tex, spec_tex, depth_tex);
}

void Eng::ExGBufferFill::LazyInit(Ren::Context &ctx, ShaderLoader &sh, const Ren::ImageRWHandle albedo_tex,
                                  const Ren::ImageRWHandle normal_tex, const Ren::ImageRWHandle spec_tex,
                                  const Ren::ImageRWHandle depth_tex) {
    const Ren::RenderTarget color_targets[] = {{albedo_tex, Ren::eLoadOp::Clear, Ren::eStoreOp::Store},
                                               {normal_tex, Ren::eLoadOp::Clear, Ren::eStoreOp::Store},
                                               {spec_tex, Ren::eLoadOp::Clear, Ren::eStoreOp::Store}};
    const Ren::RenderTarget depth_target = {depth_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store, Ren::eLoadOp::Load,
                                            Ren::eStoreOp::Store};

    if (!initialized) {
#if defined(REN_GL_BACKEND)
        const bool bindless = ctx.capabilities.bindless_texture;
#else
        const bool bindless = true;
#endif

        const int buf1_stride = 16, buf2_stride = 16;

        Ren::VertexInputHandle vi_simple, vi_vegetation;

        { // VertexInput for simple and skinned meshes
            const Ren::VtxAttribDesc attribs[] = {
                {0, VTX_POS_LOC, 3, Ren::eType::Float32, buf1_stride, 0, 0},
                {0, VTX_UV1_LOC, 2, Ren::eType::Float16, buf1_stride, 0, 3 * sizeof(float)},
                {1, VTX_NOR_LOC, 4, Ren::eType::Int16_snorm, buf2_stride, 0, 0},
                {1, VTX_TAN_LOC, 2, Ren::eType::Int16_snorm, buf2_stride, 0, 4 * sizeof(uint16_t)}};
            vi_simple = sh.FindOrCreateVertexInput(attribs);
        }
        { // VertexInput for vegetation meshes (uses additional vertex color attribute)
            const Ren::VtxAttribDesc attribs[] = {
                {0, VTX_POS_LOC, 3, Ren::eType::Float32, buf1_stride, 0, 0},
                {0, VTX_UV1_LOC, 2, Ren::eType::Float16, buf1_stride, 0, 3 * sizeof(float)},
                {1, VTX_NOR_LOC, 4, Ren::eType::Int16_snorm, buf2_stride, 0, 0},
                {1, VTX_TAN_LOC, 2, Ren::eType::Int16_snorm, buf2_stride, 0, 4 * sizeof(uint16_t)},
                {1, VTX_AUX_LOC, 1, Ren::eType::Uint32, buf2_stride, 0, 6 * sizeof(uint16_t)}};
            vi_vegetation = sh.FindOrCreateVertexInput(attribs);
        }

        const Ren::ProgramHandle gbuf_simple_prog = sh.FindOrCreateProgram(
            bindless ? "internal/gbuffer_fill.vert.glsl" : "internal/gbuffer_fill@NO_BINDLESS.vert.glsl",
            bindless ? "internal/gbuffer_fill.frag.glsl" : "internal/gbuffer_fill@NO_BINDLESS.frag.glsl");
        const Ren::ProgramHandle gbuf_vegetation_prog = sh.FindOrCreateProgram(
            bindless ? "internal/gbuffer_fill@VEGETATION.vert.glsl"
                     : "internal/gbuffer_fill@VEGETATION;NO_BINDLESS.vert.glsl",
            bindless ? "internal/gbuffer_fill.frag.glsl" : "internal/gbuffer_fill@NO_BINDLESS.frag.glsl");

        const Ren::RenderPassHandle rp_main_draw = sh.FindOrCreateRenderPass(depth_target, color_targets);

        { // simple and skinned
            Ren::RastState rast_state;
            rast_state.depth.test_enabled = true;
            rast_state.depth.compare_op = uint8_t(Ren::eCompareOp::Equal);

            pi_simple_[2] = sh.FindOrCreatePipeline(rast_state, gbuf_simple_prog, vi_simple, rp_main_draw, 0);

            rast_state.poly.cull = uint8_t(Ren::eCullFace::Back);

            pi_simple_[0] = sh.FindOrCreatePipeline(rast_state, gbuf_simple_prog, vi_simple, rp_main_draw, 0);

            rast_state.poly.cull = uint8_t(Ren::eCullFace::Front);

            pi_simple_[1] = sh.FindOrCreatePipeline(rast_state, gbuf_simple_prog, vi_simple, rp_main_draw, 0);
        }
        { // vegetation
            Ren::RastState rast_state;
            rast_state.depth.test_enabled = true;
            rast_state.depth.compare_op = uint8_t(Ren::eCompareOp::Equal);

            pi_vegetation_[1] =
                sh.FindOrCreatePipeline(rast_state, gbuf_vegetation_prog, vi_vegetation, rp_main_draw, 0);

            rast_state.poly.cull = uint8_t(Ren::eCullFace::Back);

            pi_vegetation_[0] =
                sh.FindOrCreatePipeline(rast_state, gbuf_vegetation_prog, vi_vegetation, rp_main_draw, 0);
        }

        initialized = true;
    }
}