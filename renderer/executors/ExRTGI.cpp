#include "ExRTGI.h"

#include <Ren/Context.h>
#include <Ren/DrawCall.h>

#include "../../utils/ShaderLoader.h"
#include "../Renderer_Structs.h"
#include "../framegraph/FgBuilder.h"
#include "../shaders/rt_gi_interface.h"

void Eng::ExRTGI::Execute(const FgContext &fg) {
    LazyInit(fg.ren_ctx(), fg.sh());
    if (fg.ren_ctx().capabilities.hwrt) {
        Execute_HWRT(fg);
    } else {
        Execute_SWRT(fg);
    }
}

void Eng::ExRTGI::LazyInit(Ren::Context &ctx, ShaderLoader &sh) {
    if (!initialized_) {
        auto hwrt_select = [&ctx](std::string_view hwrt_shader, std::string_view swrt_shader) {
            return ctx.capabilities.hwrt ? hwrt_shader : swrt_shader;
        };
        auto subgroup_select = [&ctx](std::string_view subgroup_shader, std::string_view nosubgroup_shader) {
            return ctx.capabilities.subgroup ? subgroup_shader : nosubgroup_shader;
        };

        if (args_->second_bounce) {
            pi_rt_gi_ = sh.FindOrCreatePipeline(
                hwrt_select(subgroup_select("internal/rt_gi_hwrt@SECOND.comp.glsl",
                                            "internal/rt_gi_hwrt@SECOND;NO_SUBGROUP.comp.glsl"),
                            subgroup_select("internal/rt_gi_swrt@SECOND.comp.glsl",
                                            "internal/rt_gi_swrt@SECOND;NO_SUBGROUP.comp.glsl")),
                32);
        } else {
            pi_rt_gi_ =
                sh.FindOrCreatePipeline(hwrt_select(subgroup_select("internal/rt_gi_hwrt@FIRST.comp.glsl",
                                                                    "internal/rt_gi_hwrt@FIRST;NO_SUBGROUP.comp.glsl"),
                                                    subgroup_select("internal/rt_gi_swrt@FIRST.comp.glsl",
                                                                    "internal/rt_gi_swrt@FIRST;NO_SUBGROUP.comp.glsl")),
                                        32);
        }

        initialized_ = true;
    }
}

void Eng::ExRTGI::Execute_SWRT(const FgContext &fg) {
    const Ren::BufferROHandle geo_data_buf = fg.AccessROBuffer(args_->geo_data);
    const Ren::BufferROHandle materials_buf = fg.AccessROBuffer(args_->materials);
    const Ren::BufferROHandle vtx_buf1 = fg.AccessROBuffer(args_->vtx_buf1);
    const Ren::BufferROHandle ndx_buf = fg.AccessROBuffer(args_->ndx_buf);
    const Ren::BufferROHandle rt_blas_buf = fg.AccessROBuffer(args_->swrt.rt_blas_buf);
    const Ren::BufferROHandle unif_sh_data_buf = fg.AccessROBuffer(args_->shared_data);
    const Ren::ImageROHandle noise_tex = fg.AccessROImage(args_->noise_tex);
    const Ren::ImageROHandle depth_tex = fg.AccessROImage(args_->depth_tex);
    const Ren::ImageROHandle normal_tex = fg.AccessROImage(args_->normal_tex);
    const Ren::BufferROHandle ray_list_buf = fg.AccessROBuffer(args_->ray_list);
    const Ren::BufferROHandle indir_args_buf = fg.AccessROBuffer(args_->indir_args);
    const Ren::BufferROHandle rt_tlas_buf = fg.AccessROBuffer(args_->tlas_buf);
    const Ren::BufferROHandle prim_ndx_buf = fg.AccessROBuffer(args_->swrt.prim_ndx_buf);
    const Ren::BufferROHandle mesh_instances_buf = fg.AccessROBuffer(args_->swrt.mesh_instances_buf);

    const Ren::BufferHandle ray_counter_buf = fg.AccessRWBuffer(args_->ray_counter);
    const Ren::BufferHandle out_ray_hits_buf = fg.AccessRWBuffer(args_->out_ray_hits_buf);

    Ren::SmallVector<Ren::Binding, 24> bindings = {
        {Ren::eBindTarget::UBuf, BIND_UB_SHARED_DATA_BUF, unif_sh_data_buf},
        {Ren::eBindTarget::BindlessDescriptors, BIND_BINDLESS_TEX, bindless_tex_->rt_inline_textures},
        {Ren::eBindTarget::TexSampled, RTGI::DEPTH_TEX_SLOT, {depth_tex, 1}},
        {Ren::eBindTarget::TexSampled, RTGI::NORM_TEX_SLOT, normal_tex},
        {Ren::eBindTarget::TexSampled, RTGI::NOISE_TEX_SLOT, noise_tex},
        {Ren::eBindTarget::SBufRW, RTGI::RAY_COUNTER_SLOT, ray_counter_buf},
        {Ren::eBindTarget::SBufRO, RTGI::RAY_LIST_SLOT, ray_list_buf},
        {Ren::eBindTarget::UTBuf, RTGI::BLAS_BUF_SLOT, rt_blas_buf},
        {Ren::eBindTarget::UTBuf, RTGI::TLAS_BUF_SLOT, rt_tlas_buf},
        {Ren::eBindTarget::UTBuf, RTGI::PRIM_NDX_BUF_SLOT, prim_ndx_buf},
        {Ren::eBindTarget::UTBuf, RTGI::MESH_INSTANCES_BUF_SLOT, mesh_instances_buf},
        {Ren::eBindTarget::SBufRO, RTGI::GEO_DATA_BUF_SLOT, geo_data_buf},
        {Ren::eBindTarget::SBufRO, RTGI::MATERIAL_BUF_SLOT, materials_buf},
        {Ren::eBindTarget::UTBuf, RTGI::VTX_BUF1_SLOT, vtx_buf1},
        {Ren::eBindTarget::UTBuf, RTGI::NDX_BUF_SLOT, ndx_buf},
        {Ren::eBindTarget::SBufRW, RTGI::OUT_RAY_HITS_BUF_SLOT, out_ray_hits_buf}};

    RTGI::Params uniform_params;
    uniform_params.img_size = Ren::Vec2u{view_state_->ren_res};
    uniform_params.pixel_spread_angle = view_state_->pixel_spread_angle;
    uniform_params.frame_index = view_state_->frame_index;
    uniform_params.lights_count = view_state_->stochastic_lights_count;

    DispatchComputeIndirect(fg.cmd_buf(), pi_rt_gi_, fg.storages(), indir_args_buf,
                            sizeof(VkTraceRaysIndirectCommandKHR), bindings, &uniform_params, sizeof(uniform_params),
                            fg.descr_alloc(), fg.log());
}

#if defined(REN_GL_BACKEND)
void Eng::ExRTGI::Execute_HWRT(const FgContext &fg) { assert(false && "Not implemented!"); }
#endif
