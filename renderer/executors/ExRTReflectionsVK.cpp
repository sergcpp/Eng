#include "ExRTReflections.h"

#include <Ren/Context.h>
#include <Ren/DrawCall.h>
#include <Ren/Vk/VKCtx.h>

#include "../framegraph/FgBuilder.h"
#include "../shaders/rt_reflections_interface.h"

void Eng::ExRTReflections::Execute_HWRT(const FgContext &fg) {
    const Ren::BufferROHandle geo_data = fg.AccessROBuffer(args_->geo_data);
    const Ren::BufferROHandle materials = fg.AccessROBuffer(args_->materials);
    const Ren::BufferROHandle vtx_buf1 = fg.AccessROBuffer(args_->vtx_buf1);
    const Ren::BufferROHandle vtx_buf2 = fg.AccessROBuffer(args_->vtx_buf2);
    const Ren::BufferROHandle ndx_buf = fg.AccessROBuffer(args_->ndx_buf);
    const Ren::BufferROHandle unif_sh_data = fg.AccessROBuffer(args_->shared_data);
    const Ren::ImageROHandle depth_tex = fg.AccessROImage(args_->depth_tex);
    const Ren::ImageROHandle normal_tex = fg.AccessROImage(args_->normal_tex);
    const Ren::ImageROHandle env_tex = fg.AccessROImage(args_->env_tex);
    const Ren::BufferROHandle ray_counter = fg.AccessROBuffer(args_->ray_counter);
    const Ren::BufferROHandle ray_list = fg.AccessROBuffer(args_->ray_list);
    const Ren::BufferROHandle indir_args = fg.AccessROBuffer(args_->indir_args);
    [[maybe_unused]] const Ren::BufferROHandle tlas = fg.AccessROBuffer(args_->tlas_buf);
    const Ren::BufferROHandle lights = fg.AccessROBuffer(args_->lights);
    const Ren::ImageROHandle shadow_depth = fg.AccessROImage(args_->shadow_depth);
    const Ren::ImageROHandle shadow_color = fg.AccessROImage(args_->shadow_color);
    const Ren::ImageROHandle ltc_luts = fg.AccessROImage(args_->ltc_luts);
    const Ren::BufferROHandle cells = fg.AccessROBuffer(args_->cells);
    const Ren::BufferROHandle items = fg.AccessROBuffer(args_->items);

    Ren::ImageROHandle irr_tex, dist_tex, off_tex;
    if (args_->irradiance_tex) {
        irr_tex = fg.AccessROImage(args_->irradiance_tex);
        dist_tex = fg.AccessROImage(args_->distance_tex);
        off_tex = fg.AccessROImage(args_->offset_tex);
    }

    Ren::BufferROHandle stoch_lights_buf = {}, light_nodes_buf = {};
    if (args_->stoch_lights_buf) {
        stoch_lights_buf = fg.AccessROBuffer(args_->stoch_lights_buf);
        light_nodes_buf = fg.AccessROBuffer(args_->light_nodes_buf);
    }

    Ren::BufferROHandle oit_depth = {};
    Ren::ImageROHandle noise = {};
    if (args_->oit_depth_buf) {
        oit_depth = fg.AccessROBuffer(args_->oit_depth_buf);
    } else {
        noise = fg.AccessROImage(args_->noise);
    }

    const Ren::ApiContext &api = fg.ren_ctx().api();
    const Ren::StoragesRef &storages = fg.storages();

    VkCommandBuffer cmd_buf = api.draw_cmd_buf[api.backend_frame];

    Ren::SmallVector<Ren::Binding, 24> bindings = {
        {Ren::eBindTarget::UBuf, BIND_UB_SHARED_DATA_BUF, unif_sh_data},
        {Ren::eBindTarget::TexSampled, RTReflections::DEPTH_TEX_SLOT, {depth_tex, 1}},
        {Ren::eBindTarget::TexSampled, RTReflections::NORM_TEX_SLOT, normal_tex},
        {Ren::eBindTarget::SBufRO, RTReflections::RAY_COUNTER_SLOT, ray_counter},
        {Ren::eBindTarget::SBufRO, RTReflections::RAY_LIST_SLOT, ray_list},
        {Ren::eBindTarget::TexSampled, RTReflections::ENV_TEX_SLOT, env_tex},
        {Ren::eBindTarget::AccStruct, RTReflections::TLAS_SLOT, args_->tlas},
        {Ren::eBindTarget::SBufRO, RTReflections::GEO_DATA_BUF_SLOT, geo_data},
        {Ren::eBindTarget::SBufRO, RTReflections::MATERIAL_BUF_SLOT, materials},
        {Ren::eBindTarget::SBufRO, RTReflections::VTX_BUF1_SLOT, vtx_buf1},
        {Ren::eBindTarget::SBufRO, RTReflections::VTX_BUF2_SLOT, vtx_buf2},
        {Ren::eBindTarget::SBufRO, RTReflections::NDX_BUF_SLOT, ndx_buf},
        {Ren::eBindTarget::SBufRO, RTReflections::LIGHTS_BUF_SLOT, lights},
        {Ren::eBindTarget::TexSampled, RTReflections::SHADOW_DEPTH_TEX_SLOT, shadow_depth},
        {Ren::eBindTarget::TexSampled, RTReflections::SHADOW_COLOR_TEX_SLOT, shadow_color},
        {Ren::eBindTarget::TexSampled, RTReflections::LTC_LUTS_TEX_SLOT, ltc_luts},
        {Ren::eBindTarget::UTBuf, RTReflections::CELLS_BUF_SLOT, cells},
        {Ren::eBindTarget::UTBuf, RTReflections::ITEMS_BUF_SLOT, items}};
    if (irr_tex) {
        bindings.emplace_back(Ren::eBindTarget::TexSampled, RTReflections::IRRADIANCE_TEX_SLOT, irr_tex);
        bindings.emplace_back(Ren::eBindTarget::TexSampled, RTReflections::DISTANCE_TEX_SLOT, dist_tex);
        bindings.emplace_back(Ren::eBindTarget::TexSampled, RTReflections::OFFSET_TEX_SLOT, off_tex);
    }
    if (stoch_lights_buf) {
        bindings.emplace_back(Ren::eBindTarget::UTBuf, RTReflections::STOCH_LIGHTS_BUF_SLOT, stoch_lights_buf);
        bindings.emplace_back(Ren::eBindTarget::UTBuf, RTReflections::LIGHT_NODES_BUF_SLOT, light_nodes_buf);
    }
    if (noise) {
        bindings.emplace_back(Ren::eBindTarget::TexSampled, RTReflections::NOISE_TEX_SLOT, noise);
    }
    if (oit_depth) {
        bindings.emplace_back(Ren::eBindTarget::UTBuf, RTReflections::OIT_DEPTH_BUF_SLOT, oit_depth);
    }
    for (int i = 0; i < OIT_REFLECTION_LAYERS && args_->out_refl_tex[i]; ++i) {
        const Ren::ImageRWHandle out_refl_tex = fg.AccessRWImage(args_->out_refl_tex[i]);
        bindings.emplace_back(Ren::eBindTarget::ImageRW, RTReflections::OUT_REFL_IMG_SLOT, i, 1, out_refl_tex);
    }

    const Ren::PipelineMain &pi = storages.pipelines.Get(pi_rt_reflections_[bool(stoch_lights_buf)]).first;
    const Ren::ProgramMain &pr = storages.programs.Get(pi.prog).first;

    VkDescriptorSet descr_sets[2];
    descr_sets[0] = PrepareDescriptorSet(api, storages, pr.descr_set_layouts[0], bindings, fg.descr_alloc(), fg.log());
    descr_sets[1] = bindless_tex_->rt_inline_textures.descr_set;

    api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi.pipeline);
    api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi.layout, 0, 2, descr_sets, 0, nullptr);

    RTReflections::Params uniform_params;
    uniform_params.img_size = Ren::Vec2u{view_state_->ren_res};
    uniform_params.pixel_spread_angle = view_state_->pixel_spread_angle;
    uniform_params.lights_count = view_state_->stochastic_lights_count;

    api.vkCmdPushConstants(cmd_buf, pi.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uniform_params), &uniform_params);

    const Ren::BufferMain &indir_args_buf_main = storages.buffers.Get(indir_args).first;
    api.vkCmdDispatchIndirect(cmd_buf, indir_args_buf_main.buf, VkDeviceSize(sizeof(VkTraceRaysIndirectCommandKHR)));
}
