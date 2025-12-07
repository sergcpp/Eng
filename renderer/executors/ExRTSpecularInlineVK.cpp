#include "ExRTSpecularInline.h"

#include <Ren/Context.h>
#include <Ren/DrawCall.h>
#include <Ren/Vk/VKCtx.h>

#include "../framegraph/FgBuilder.h"
#include "../shaders/rt_specular_interface.h"

void Eng::ExRTSpecularInline::Execute_HWRT(const FgContext &fg) {
    const Ren::BufferROHandle geo_data = fg.AccessROBuffer(args_->geo_data);
    const Ren::BufferROHandle materials = fg.AccessROBuffer(args_->materials);
    const Ren::BufferROHandle vtx_buf1 = fg.AccessROBuffer(args_->vtx_buf1);
    const Ren::BufferROHandle vtx_buf2 = fg.AccessROBuffer(args_->vtx_buf2);
    const Ren::BufferROHandle ndx_buf = fg.AccessROBuffer(args_->ndx_buf);
    const Ren::BufferROHandle unif_sh_data = fg.AccessROBuffer(args_->shared_data);
    const Ren::ImageROHandle depth = fg.AccessROImage(args_->depth);
    const Ren::ImageROHandle normal = fg.AccessROImage(args_->normal);
    const Ren::ImageROHandle env = fg.AccessROImage(args_->env);
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

    const Ren::ApiContext &api = fg.ren_ctx().api();
    const Ren::StoragesRef &storages = fg.storages();

    VkCommandBuffer cmd_buf = fg.cmd_buf();

    Ren::SmallVector<Ren::Binding, 24> bindings = {
        {Ren::eBindTarget::UBuf, BIND_UB_SHARED_DATA_BUF, unif_sh_data},
        {Ren::eBindTarget::TexSampled, RTSpecular::DEPTH_TEX_SLOT, {depth, 1}},
        {Ren::eBindTarget::TexSampled, RTSpecular::NORM_TEX_SLOT, normal},
        {Ren::eBindTarget::SBufRO, RTSpecular::RAY_COUNTER_SLOT, ray_counter},
        {Ren::eBindTarget::SBufRO, RTSpecular::RAY_LIST_SLOT, ray_list},
        {Ren::eBindTarget::TexSampled, RTSpecular::ENV_TEX_SLOT, env},
        {Ren::eBindTarget::AccStruct, RTSpecular::TLAS_SLOT, args_->tlas},
        {Ren::eBindTarget::SBufRO, RTSpecular::GEO_DATA_BUF_SLOT, geo_data},
        {Ren::eBindTarget::SBufRO, RTSpecular::MATERIAL_BUF_SLOT, materials},
        {Ren::eBindTarget::SBufRO, RTSpecular::VTX_BUF1_SLOT, vtx_buf1},
        {Ren::eBindTarget::SBufRO, RTSpecular::VTX_BUF2_SLOT, vtx_buf2},
        {Ren::eBindTarget::SBufRO, RTSpecular::NDX_BUF_SLOT, ndx_buf},
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

    const Ren::PipelineMain &pi = storages.pipelines[pi_rt_specular_[bool(stoch_lights)]].first;
    const Ren::ProgramMain &pr = storages.programs[pi.prog].first;

    VkDescriptorSet descr_sets[2];
    descr_sets[0] = PrepareDescriptorSet(api, storages, pr.descr_set_layouts[0], bindings, fg.descr_alloc(), fg.log());
    descr_sets[1] = bindless_tex_->rt_inline_textures.descr_set;

    api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi.pipeline);
    api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi.layout, 0, 2, descr_sets, 0, nullptr);

    RTSpecular::Params uniform_params;
    uniform_params.img_size = Ren::Vec2u{view_state_->ren_res};
    uniform_params.pixel_spread_angle = view_state_->pixel_spread_angle;
    uniform_params.lights_count = view_state_->stochastic_lights_count;

    api.vkCmdPushConstants(cmd_buf, pi.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uniform_params), &uniform_params);

    const Ren::BufferMain &indir_args_buf_main = storages.buffers[indir_args].first;
    api.vkCmdDispatchIndirect(cmd_buf, indir_args_buf_main.buf, VkDeviceSize(sizeof(VkTraceRaysIndirectCommandKHR)));
}
