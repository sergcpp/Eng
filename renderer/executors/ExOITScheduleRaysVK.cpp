#include "ExOITScheduleRays.h"

#include <Ren/Context.h>
#include <Ren/DebugMarker.h>
#include <Ren/RastState.h>
#include <Ren/VKCtx.h>

#include "../PrimDraw.h"
#include "../shaders/oit_schedule_rays_interface.h"

namespace ExSharedInternal {
uint32_t _draw_range_ext(const Ren::ApiContext &api, VkCommandBuffer cmd_buf, const Ren::PipelineMain &pipeline,
                         Ren::Span<const uint32_t> batch_indices, Ren::Span<const Eng::basic_draw_batch_t> batches,
                         uint32_t i, uint64_t mask, uint32_t materials_per_descriptor,
                         Ren::Span<const VkDescriptorSet> descr_sets, int *draws_count);
}

void Eng::ExOITScheduleRays::DrawTransparent(const FgContext &fg, const Ren::WeakImgRef &depth_tex) {
    using namespace ExSharedInternal;

    const Ren::ApiContext &api = fg.ren_ctx().api();
    const Ren::StoragesRef &storages = fg.storages();

    const Ren::Image &noise_tex = fg.AccessROImage(noise_tex_);
    const Ren::BufferROHandle instances_buf = fg.AccessROBuffer(instances_buf_);
    const Ren::BufferROHandle instance_indices_buf = fg.AccessROBuffer(instance_indices_buf_);
    const Ren::BufferROHandle unif_shared_data_buf = fg.AccessROBuffer(shared_data_buf_);
    const Ren::BufferROHandle materials_buf = fg.AccessROBuffer(materials_buf_);
    const Ren::BufferROHandle oit_depth_buf = fg.AccessROBuffer(oit_depth_buf_);
    const Ren::BufferHandle ray_counter_buf = fg.AccessRWBuffer(ray_counter_);
    const Ren::BufferHandle ray_list_buf = fg.AccessRWBuffer(ray_list_);
    const Ren::BufferHandle ray_bitmask_buf = fg.AccessRWBuffer(ray_bitmask_);

    if ((*p_list_)->alpha_blend_start_index == -1) {
        return;
    }

    const Ren::Binding bindings[] = {{Ren::eBindTarget::UBuf, BIND_UB_SHARED_DATA_BUF, unif_shared_data_buf},
                                     {Ren::eBindTarget::UTBuf, BIND_INST_BUF, instances_buf},
                                     {Ren::eBindTarget::UTBuf, OITScheduleRays::OIT_DEPTH_BUF_SLOT, oit_depth_buf},
                                     {Ren::eBindTarget::SBufRO, BIND_INST_NDX_BUF, instance_indices_buf},
                                     {Ren::eBindTarget::SBufRO, BIND_MATERIALS_BUF, materials_buf},
                                     {Ren::eBindTarget::SBufRW, OITScheduleRays::RAY_COUNTER_SLOT, ray_counter_buf},
                                     {Ren::eBindTarget::SBufRW, OITScheduleRays::RAY_LIST_SLOT, ray_list_buf},
                                     {Ren::eBindTarget::SBufRW, OITScheduleRays::RAY_BITMASK_SLOT, ray_bitmask_buf},
                                     {Ren::eBindTarget::TexSampled, BIND_NOISE_TEX, noise_tex}};

    const Ren::PipelineMain &pi_vegetation0_main = storages.pipelines.Get(pi_vegetation_[0]).first;
    const Ren::ProgramMain &pr_vegetation0_main = storages.programs.Get(pi_vegetation0_main.prog).first;

    VkDescriptorSet descr_sets[2];
    descr_sets[0] = PrepareDescriptorSet(api, &fg.storages(), pr_vegetation0_main.descr_set_layouts[0], bindings,
                                         fg.descr_alloc(), fg.log());
    descr_sets[1] = bindless_tex_->textures_descr_sets[0];

    using BDB = basic_draw_batch_t;

    const uint32_t materials_per_descriptor = api.max_combined_image_samplers / MAX_TEX_PER_MATERIAL;

    VkCommandBuffer cmd_buf = fg.cmd_buf();

    const VkViewport viewport = {0.0f, 0.0f, float(view_state_->ren_res[0]), float(view_state_->ren_res[1]),
                                 0.0f, 1.0f};
    api.vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
    const VkRect2D scissor = {{0, 0}, {uint32_t(view_state_->ren_res[0]), uint32_t(view_state_->ren_res[1])}};
    api.vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    const Ren::Span<const basic_draw_batch_t> batches = {(*p_list_)->basic_batches};
    const Ren::Span<const uint32_t> batch_indices = {(*p_list_)->basic_batch_indices};

    int draws_count = 0;
    uint32_t i = (*p_list_)->alpha_blend_start_index;

    { // solid meshes
        const Ren::PipelineMain &pi_simple0_main = storages.pipelines.Get(pi_simple_[0]).first;
        const Ren::PipelineMain &pi_simple1_main = storages.pipelines.Get(pi_simple_[1]).first;
        const Ren::PipelineMain &pi_simple2_main = storages.pipelines.Get(pi_simple_[2]).first;

        const Ren::RenderPassMain &rp_simple0_main = storages.render_passes.Get(pi_simple0_main.render_pass).first;

        VkRenderPassBeginInfo rp_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rp_begin_info.renderPass = rp_simple0_main.handle;
        rp_begin_info.framebuffer = main_draw_fb_[api.backend_frame][fb_to_use_].vk_handle();
        rp_begin_info.renderArea = {{0, 0}, {uint32_t(view_state_->ren_res[0]), uint32_t(view_state_->ren_res[1])}};
        const VkClearValue clear_values[4] = {{}, {}, {}, {}};
        rp_begin_info.pClearValues = clear_values;
        rp_begin_info.clearValueCount = 4;
        api.vkCmdBeginRenderPass(cmd_buf, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        { // Simple meshes
            Ren::DebugMarker _m(api, cmd_buf, "SIMPLE");

            const Ren::VertexInputMain &vtx_input_main = storages.vtx_inputs.Get(pi_simple0_main.vtx_input).first;
            VertexInput_BindBuffers(api, vtx_input_main, storages.buffers, cmd_buf, 0, VK_INDEX_TYPE_UINT32);

            { // solid one-sided
                Ren::DebugMarker _mm(api, cmd_buf, "SOLID-ONE-SIDED");
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple0_main.handle);
                api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple0_main.layout, 0, 2,
                                            descr_sets, 0, nullptr);
                i = _draw_range_ext(api, cmd_buf, pi_simple0_main, batch_indices, batches, i, BDB::BitAlphaBlend,
                                    materials_per_descriptor, bindless_tex_->textures_descr_sets, &draws_count);
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple1_main.handle);
                i = _draw_range_ext(api, cmd_buf, pi_simple1_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitBackSided, materials_per_descriptor,
                                    bindless_tex_->textures_descr_sets, &draws_count);
            }
            { // solid two-sided
                Ren::DebugMarker _mm(api, cmd_buf, "SOLID-TWO-SIDED");
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple2_main.handle);
                api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple2_main.layout, 0, 2,
                                            descr_sets, 0, nullptr);
                i = _draw_range_ext(api, cmd_buf, pi_simple2_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitTwoSided, materials_per_descriptor,
                                    bindless_tex_->textures_descr_sets, &draws_count);
            }
            { // moving solid one-sided
                Ren::DebugMarker _mm(api, cmd_buf, "MOVING-SOLID-ONE-SIDED");
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple0_main.handle);
                api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple0_main.layout, 0, 2,
                                            descr_sets, 0, nullptr);
                i = _draw_range_ext(api, cmd_buf, pi_simple0_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitMoving, materials_per_descriptor,
                                    bindless_tex_->textures_descr_sets, &draws_count);
            }
            { // moving solid two-sided
                Ren::DebugMarker _mm(api, cmd_buf, "MOVING-SOLID-TWO-SIDED");
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple2_main.handle);
                api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple2_main.layout, 0, 2,
                                            descr_sets, 0, nullptr);
                i = _draw_range_ext(api, cmd_buf, pi_simple2_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitTwoSided, materials_per_descriptor,
                                    bindless_tex_->textures_descr_sets, &draws_count);
            }
            { // alpha-tested one-sided
                Ren::DebugMarker _mm(api, cmd_buf, "ALPHA-ONE-SIDED");
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple0_main.handle);
                api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple0_main.layout, 0, 2,
                                            descr_sets, 0, nullptr);
                i = _draw_range_ext(api, cmd_buf, pi_simple0_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitAlphaTest, materials_per_descriptor,
                                    bindless_tex_->textures_descr_sets, &draws_count);
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple1_main.handle);
                i = _draw_range_ext(api, cmd_buf, pi_simple1_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitAlphaTest | BDB::BitBackSided,
                                    materials_per_descriptor, bindless_tex_->textures_descr_sets, &draws_count);
            }
            { // alpha-tested two-sided
                Ren::DebugMarker _mm(api, cmd_buf, "ALPHA-TWO-SIDED");
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple2_main.handle);
                api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple2_main.layout, 0, 2,
                                            descr_sets, 0, nullptr);
                i = _draw_range_ext(api, cmd_buf, pi_simple2_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitAlphaTest | BDB::BitTwoSided, materials_per_descriptor,
                                    bindless_tex_->textures_descr_sets, &draws_count);
            }
            { // moving alpha-tested one-sided
                Ren::DebugMarker _mm(api, cmd_buf, "MOVING-ALPHA-ONE-SIDED");
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple0_main.handle);
                api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple0_main.layout, 0, 2,
                                            descr_sets, 0, nullptr);
                i = _draw_range_ext(api, cmd_buf, pi_simple0_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitAlphaTest, materials_per_descriptor,
                                    bindless_tex_->textures_descr_sets, &draws_count);
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple1_main.handle);
                i = _draw_range_ext(api, cmd_buf, pi_simple1_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitAlphaTest | BDB::BitBackSided,
                                    materials_per_descriptor, bindless_tex_->textures_descr_sets, &draws_count);
            }
            { // moving alpha-tested two-sided
                Ren::DebugMarker _mm(api, cmd_buf, "MOVING-ALPHA-TWO-SIDED");
                api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple2_main.handle);
                api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_simple2_main.layout, 0, 2,
                                            descr_sets, 0, nullptr);
                i = _draw_range_ext(api, cmd_buf, pi_simple2_main, batch_indices, batches, i,
                                    BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitAlphaTest | BDB::BitTwoSided,
                                    materials_per_descriptor, bindless_tex_->textures_descr_sets, &draws_count);
            }
        }

        api.vkCmdEndRenderPass(cmd_buf);
    }
}
