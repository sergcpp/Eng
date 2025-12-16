#include "ExRTGI.h"

#include <Ren/Context.h>
#include <Ren/Image.h>
#include <Ren/RastState.h>
#include <Ren/VKCtx.h>

#include "../../utils/ShaderLoader.h"
#include "../PrimDraw.h"
#include "../shaders/rt_gi_interface.h"

void Eng::ExRTGI::Execute_HWRT(const FgContext &fg) {
    const Ren::BufferHandle geo_data_buf = fg.AccessROBuffer(args_->geo_data);
    const Ren::BufferHandle materials_buf = fg.AccessROBuffer(args_->materials);
    const Ren::BufferHandle vtx_buf1 = fg.AccessROBuffer(args_->vtx_buf1);
    const Ren::BufferHandle ndx_buf = fg.AccessROBuffer(args_->ndx_buf);
    const Ren::BufferHandle unif_sh_data_buf = fg.AccessROBuffer(args_->shared_data);
    const Ren::Image &noise_tex = fg.AccessROImage(args_->noise_tex);
    const Ren::Image &depth_tex = fg.AccessROImage(args_->depth_tex);
    const Ren::Image &normal_tex = fg.AccessROImage(args_->normal_tex);
    const Ren::BufferHandle ray_list_buf = fg.AccessROBuffer(args_->ray_list);
    const Ren::BufferHandle indir_args_buf = fg.AccessROBuffer(args_->indir_args);
    [[maybe_unused]] const Ren::BufferHandle rt_tlas_buf = fg.AccessROBuffer(args_->tlas_buf);

    const Ren::BufferHandle ray_counter_buf = fg.AccessRWBuffer(args_->ray_counter);
    const Ren::BufferHandle out_ray_hits_buf = fg.AccessRWBuffer(args_->out_ray_hits_buf);

    const Ren::ApiContext &api = fg.ren_ctx().api();

    auto *acc_struct = static_cast<Ren::AccStructureVK *>(args_->tlas);

    VkCommandBuffer cmd_buf = api.draw_cmd_buf[api.backend_frame];

    Ren::SmallVector<Ren::Binding, 24> bindings = {
        {Ren::eBindTarget::UBuf, BIND_UB_SHARED_DATA_BUF, unif_sh_data_buf},
        {Ren::eBindTarget::TexSampled, RTGI::DEPTH_TEX_SLOT, {depth_tex, 1}},
        {Ren::eBindTarget::TexSampled, RTGI::NORM_TEX_SLOT, normal_tex},
        {Ren::eBindTarget::TexSampled, RTGI::NOISE_TEX_SLOT, noise_tex},
        {Ren::eBindTarget::SBufRW, RTGI::RAY_COUNTER_SLOT, ray_counter_buf},
        {Ren::eBindTarget::SBufRO, RTGI::RAY_LIST_SLOT, ray_list_buf},
        {Ren::eBindTarget::AccStruct, RTGI::TLAS_SLOT, *acc_struct},
        {Ren::eBindTarget::SBufRO, RTGI::GEO_DATA_BUF_SLOT, geo_data_buf},
        {Ren::eBindTarget::SBufRO, RTGI::MATERIAL_BUF_SLOT, materials_buf},
        {Ren::eBindTarget::SBufRO, RTGI::VTX_BUF1_SLOT, vtx_buf1},
        {Ren::eBindTarget::SBufRO, RTGI::NDX_BUF_SLOT, ndx_buf},
        {Ren::eBindTarget::SBufRW, RTGI::OUT_RAY_HITS_BUF_SLOT, out_ray_hits_buf}};

    const Ren::PipelineMain &pi = fg.pipelines().Get(pi_rt_gi_).first;
    const Ren::ProgramMain &pr = fg.programs().Get(pi.prog).first;

    VkDescriptorSet descr_sets[2];
    descr_sets[0] =
        PrepareDescriptorSet(api, &fg.storages(), pr.descr_set_layouts[0], bindings, fg.descr_alloc(), fg.log());
    descr_sets[1] = bindless_tex_->rt_inline_textures.descr_set;

    api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi.handle);
    api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi.layout, 0, 2, descr_sets, 0, nullptr);

    RTGI::Params uniform_params;
    uniform_params.img_size = Ren::Vec2u{view_state_->ren_res};
    uniform_params.pixel_spread_angle = view_state_->pixel_spread_angle;
    uniform_params.frame_index = view_state_->frame_index;
    uniform_params.lights_count = view_state_->stochastic_lights_count;

    api.vkCmdPushConstants(cmd_buf, pi.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uniform_params), &uniform_params);

    const auto &[indir_args_main, indir_args_cold] = fg.ren_ctx().buffers().Get(indir_args_buf);
    api.vkCmdDispatchIndirect(cmd_buf, indir_args_main.buf, VkDeviceSize(sizeof(VkTraceRaysIndirectCommandKHR)));
}
