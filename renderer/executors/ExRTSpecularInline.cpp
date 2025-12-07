#include "ExRTSpecularInline.h"

#include <Ren/Context.h>
#include <Ren/DrawCall.h>

#include "../../utils/ShaderLoader.h"
#include "../framegraph/FgBuilder.h"
#include "../shaders/rt_specular_interface.h"

void Eng::ExRTSpecularInline::Execute(const FgContext &fg) {
    LazyInit(fg.ren_ctx(), fg.sh());
    if (fg.ren_ctx().capabilities.hwrt) {
        Execute_HWRT(fg);
    } else {
        Execute_SWRT(fg);
    }
}

void Eng::ExRTSpecularInline::LazyInit(Ren::Context &ctx, ShaderLoader &sh) {
    if (!initialized_) {
        auto hwrt_select = [&ctx](std::string_view hwrt_shader, std::string_view swrt_shader) {
            return ctx.capabilities.hwrt ? hwrt_shader : swrt_shader;
        };
        auto subgroup_select = [&ctx](std::string_view subgroup_shader, std::string_view nosubgroup_shader) {
            return ctx.capabilities.subgroup ? subgroup_shader : nosubgroup_shader;
        };

        if (args_->two_bounces) {
            assert(!args_->layered);
            if (args_->irradiance) {
                pi_rt_specular_[0] = sh.FindOrCreatePipeline(hwrt_select(
                    subgroup_select("internal/rt_specular_inline_hwrt@TWO_BOUNCES;GI_CACHE.comp.glsl",
                                    "internal/rt_specular_inline_hwrt@TWO_BOUNCES;GI_CACHE;NO_SUBGROUP.comp.glsl"),
                    subgroup_select("internal/rt_specular_inline_swrt@TWO_BOUNCES;GI_CACHE.comp.glsl",
                                    "internal/rt_specular_inline_swrt@TWO_BOUNCES;GI_CACHE;NO_SUBGROUP.comp.glsl")));
            } else {
                pi_rt_specular_[0] = sh.FindOrCreatePipeline(
                    hwrt_select(subgroup_select("internal/rt_specular_inline_hwrt@TWO_BOUNCES.comp.glsl",
                                                "internal/rt_specular_inline_hwrt@TWO_BOUNCES;NO_SUBGROUP.comp.glsl"),
                                subgroup_select("internal/rt_specular_inline_swrt@TWO_BOUNCES.comp.glsl",
                                                "internal/rt_specular_inline_swrt@TWO_BOUNCES;NO_SUBGROUP.comp.glsl")));
            }

            pi_rt_specular_[1] = sh.FindOrCreatePipeline(hwrt_select(
                subgroup_select("internal/rt_specular_inline_hwrt@STOCH_LIGHTS;TWO_BOUNCES;GI_CACHE.comp.glsl",
                                "internal/rt_specular_inline_hwrt@STOCH_LIGHTS;TWO_BOUNCES;GI_CACHE;NO_SUBGROUP.comp.glsl"),
                subgroup_select("internal/rt_specular_inline_swrt@STOCH_LIGHTS;TWO_BOUNCES;GI_CACHE.comp.glsl",
                                "internal/rt_specular_inline_swrt@STOCH_LIGHTS;TWO_BOUNCES;GI_CACHE;NO_SUBGROUP.comp.glsl")));
        } else {
            if (args_->layered) {
                if (args_->irradiance) {
                    pi_rt_specular_[0] = sh.FindOrCreatePipeline(hwrt_select(
                        subgroup_select("internal/rt_specular_inline_hwrt@LAYERED;GI_CACHE.comp.glsl",
                                        "internal/rt_specular_inline_hwrt@LAYERED;GI_CACHE;NO_SUBGROUP.comp.glsl"),
                        subgroup_select("internal/rt_specular_inline_swrt@LAYERED;GI_CACHE.comp.glsl",
                                        "internal/rt_specular_inline_swrt@LAYERED;GI_CACHE;NO_SUBGROUP.comp.glsl")));
                } else {
                    pi_rt_specular_[0] = sh.FindOrCreatePipeline(
                        hwrt_select(subgroup_select("internal/rt_specular_inline_hwrt@LAYERED.comp.glsl",
                                                    "internal/rt_specular_inline_hwrt@LAYERED;NO_SUBGROUP.comp.glsl"),
                                    subgroup_select("internal/rt_specular_inline_swrt@LAYERED.comp.glsl",
                                                    "internal/rt_specular_inline_swrt@LAYERED;NO_SUBGROUP.comp.glsl")));
                }
            } else {
                if (args_->irradiance) {
                    pi_rt_specular_[0] = sh.FindOrCreatePipeline(
                        hwrt_select(subgroup_select("internal/rt_specular_inline_hwrt@GI_CACHE.comp.glsl",
                                                    "internal/rt_specular_inline_hwrt@GI_CACHE;NO_SUBGROUP.comp.glsl"),
                                    subgroup_select("internal/rt_specular_inline_swrt@GI_CACHE.comp.glsl",
                                                    "internal/rt_specular_inline_swrt@GI_CACHE;NO_SUBGROUP.comp.glsl")));
                } else {
                    pi_rt_specular_[0] = sh.FindOrCreatePipeline(
                        hwrt_select(subgroup_select("internal/rt_specular_inline_hwrt.comp.glsl",
                                                    "internal/rt_specular_inline_hwrt@NO_SUBGROUP.comp.glsl"),
                                    subgroup_select("internal/rt_specular_inline_swrt.comp.glsl",
                                                    "internal/rt_specular_inline_swrt@NO_SUBGROUP.comp.glsl")));
                }

                pi_rt_specular_[1] = sh.FindOrCreatePipeline(hwrt_select(
                    subgroup_select("internal/rt_specular_inline_hwrt@STOCH_LIGHTS;GI_CACHE.comp.glsl",
                                    "internal/rt_specular_inline_hwrt@STOCH_LIGHTS;GI_CACHE;NO_SUBGROUP.comp.glsl"),
                    subgroup_select("internal/rt_specular_inline_swrt@STOCH_LIGHTS;GI_CACHE.comp.glsl",
                                    "internal/rt_specular_inline_swrt@STOCH_LIGHTS;GI_CACHE;NO_SUBGROUP.comp.glsl")));
            }
        }
        initialized_ = true;
    }
}

void Eng::ExRTSpecularInline::Execute_SWRT(const FgContext &fg) {
    const Ren::BufferROHandle geo_data = fg.AccessROBuffer(args_->geo_data);
    const Ren::BufferROHandle materials = fg.AccessROBuffer(args_->materials);
    const Ren::BufferROHandle vtx_buf1 = fg.AccessROBuffer(args_->vtx_buf1);
    const Ren::BufferROHandle vtx_buf2 = fg.AccessROBuffer(args_->vtx_buf2);
    const Ren::BufferROHandle ndx_buf = fg.AccessROBuffer(args_->ndx_buf);
    const Ren::BufferROHandle rt_blas = fg.AccessROBuffer(args_->swrt.rt_blas);
    const Ren::BufferROHandle unif_sh_data = fg.AccessROBuffer(args_->shared_data);
    const Ren::ImageROHandle depth = fg.AccessROImage(args_->depth);
    const Ren::ImageROHandle normal = fg.AccessROImage(args_->normal);
    const Ren::ImageROHandle env = fg.AccessROImage(args_->env);
    const Ren::BufferROHandle ray_counter = fg.AccessROBuffer(args_->ray_counter);
    const Ren::BufferROHandle ray_list = fg.AccessROBuffer(args_->ray_list);
    const Ren::BufferROHandle indir_args = fg.AccessROBuffer(args_->indir_args);
    const Ren::BufferROHandle rt_tlas = fg.AccessROBuffer(args_->tlas_buf);
    const Ren::BufferROHandle prim_ndx = fg.AccessROBuffer(args_->swrt.prim_ndx);
    const Ren::BufferROHandle mesh_instances = fg.AccessROBuffer(args_->swrt.mesh_instances);
    const Ren::BufferROHandle lights = fg.AccessROBuffer(args_->lights);
    const Ren::ImageROHandle shadow_depth = fg.AccessROImage(args_->shadow_depth);
    const Ren::ImageROHandle shadow_color = fg.AccessROImage(args_->shadow_color);
    const Ren::ImageROHandle ltc_luts = fg.AccessROImage(args_->ltc_luts);
    const Ren::BufferROHandle cells = fg.AccessROBuffer(args_->cells);
    const Ren::BufferROHandle items = fg.AccessROBuffer(args_->items);

    Ren::ImageROHandle irr, dist, off;
    if (args_->irradiance) {
        irr = fg.AccessROImage(args_->irradiance);
        dist = fg.AccessROImage(args_->distance);
        off = fg.AccessROImage(args_->offset);
    }

    Ren::BufferROHandle stoch_lights = {}, light_nodes = {};
    if (args_->stoch_lights) {
        stoch_lights = fg.AccessROBuffer(args_->stoch_lights);
        light_nodes = fg.AccessROBuffer(args_->light_nodes);
    }

    Ren::BufferROHandle oit_depth = {};
    Ren::ImageROHandle noise = {};
    if (args_->oit_depth) {
        oit_depth = fg.AccessROBuffer(args_->oit_depth);
    } else {
        noise = fg.AccessROImage(args_->noise);
    }

    Ren::SmallVector<Ren::Binding, 24> bindings = {
        {Ren::eBindTarget::UBuf, BIND_UB_SHARED_DATA_BUF, unif_sh_data},
        {Ren::eBindTarget::BindlessDescriptors, BIND_BINDLESS_TEX, bindless_tex_->rt_inline_textures},
        {Ren::eBindTarget::TexSampled, RTSpecular::DEPTH_TEX_SLOT, {depth, 1}},
        {Ren::eBindTarget::TexSampled, RTSpecular::NORM_TEX_SLOT, normal},
        {Ren::eBindTarget::SBufRO, RTSpecular::RAY_COUNTER_SLOT, ray_counter},
        {Ren::eBindTarget::SBufRO, RTSpecular::RAY_LIST_SLOT, ray_list},
        {Ren::eBindTarget::TexSampled, RTSpecular::ENV_TEX_SLOT, env},
        {Ren::eBindTarget::UTBuf, RTSpecular::BLAS_BUF_SLOT, rt_blas},
        {Ren::eBindTarget::UTBuf, RTSpecular::TLAS_BUF_SLOT, rt_tlas},
        {Ren::eBindTarget::UTBuf, RTSpecular::PRIM_NDX_BUF_SLOT, prim_ndx},
        {Ren::eBindTarget::UTBuf, RTSpecular::MESH_INSTANCES_BUF_SLOT, mesh_instances},
        {Ren::eBindTarget::SBufRO, RTSpecular::GEO_DATA_BUF_SLOT, geo_data},
        {Ren::eBindTarget::SBufRO, RTSpecular::MATERIAL_BUF_SLOT, materials},
        {Ren::eBindTarget::UTBuf, RTSpecular::VTX_BUF1_SLOT, vtx_buf1},
        {Ren::eBindTarget::UTBuf, RTSpecular::VTX_BUF2_SLOT, vtx_buf2},
        {Ren::eBindTarget::UTBuf, RTSpecular::NDX_BUF_SLOT, ndx_buf},
        {Ren::eBindTarget::SBufRO, RTSpecular::LIGHTS_BUF_SLOT, lights},
        {Ren::eBindTarget::TexSampled, RTSpecular::SHADOW_DEPTH_TEX_SLOT, shadow_depth},
        {Ren::eBindTarget::TexSampled, RTSpecular::SHADOW_COLOR_TEX_SLOT, shadow_color},
        {Ren::eBindTarget::TexSampled, RTSpecular::LTC_LUTS_TEX_SLOT, ltc_luts},
        {Ren::eBindTarget::UTBuf, RTSpecular::CELLS_BUF_SLOT, cells},
        {Ren::eBindTarget::UTBuf, RTSpecular::ITEMS_BUF_SLOT, items}};
    if (irr) {
        bindings.emplace_back(Ren::eBindTarget::TexSampled, RTSpecular::IRRADIANCE_TEX_SLOT, irr);
        bindings.emplace_back(Ren::eBindTarget::TexSampled, RTSpecular::DISTANCE_TEX_SLOT, dist);
        bindings.emplace_back(Ren::eBindTarget::TexSampled, RTSpecular::OFFSET_TEX_SLOT, off);
    }
    if (stoch_lights) {
        bindings.emplace_back(Ren::eBindTarget::UTBuf, RTSpecular::STOCH_LIGHTS_BUF_SLOT, stoch_lights);
        bindings.emplace_back(Ren::eBindTarget::UTBuf, RTSpecular::LIGHT_NODES_BUF_SLOT, light_nodes);
    }
    if (noise) {
        bindings.emplace_back(Ren::eBindTarget::TexSampled, RTSpecular::NOISE_TEX_SLOT, noise);
    }
    if (oit_depth) {
        bindings.emplace_back(Ren::eBindTarget::UTBuf, RTSpecular::OIT_DEPTH_BUF_SLOT, oit_depth);
    }
    for (int i = 0; i < OIT_REFLECTION_LAYERS && args_->out_refl[i]; ++i) {
        const Ren::ImageRWHandle out_refl = fg.AccessRWImage(args_->out_refl[i]);
        bindings.emplace_back(Ren::eBindTarget::ImageRW, RTSpecular::OUT_REFL_IMG_SLOT, i, 1, out_refl);
    }

    const Ren::PipelineHandle pi = pi_rt_specular_[bool(stoch_lights)];

    RTSpecular::Params uniform_params;
    uniform_params.img_size = Ren::Vec2u{view_state_->ren_res};
    uniform_params.pixel_spread_angle = view_state_->pixel_spread_angle;
    if (oit_depth) {
        // Expected to be half resolution
        uniform_params.pixel_spread_angle *= 2.0f;
    }
    uniform_params.lights_count = view_state_->stochastic_lights_count;

    DispatchComputeIndirect(fg.cmd_buf(), pi, fg.storages(), indir_args, sizeof(VkTraceRaysIndirectCommandKHR),
                            bindings, &uniform_params, sizeof(uniform_params), fg.descr_alloc(), fg.log());
}

#if defined(REN_GL_BACKEND)
void Eng::ExRTSpecularInline::Execute_HWRT(const FgContext &fg) { assert(false && "Not implemented!"); }
#endif
