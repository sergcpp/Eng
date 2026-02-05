#include "Renderer.h"

#include <Ren/Context.h>

#include "Renderer_Names.h"
#include "executors/ExOITBlendLayer.h"
#include "executors/ExOITDepthPeel.h"
#include "executors/ExOITScheduleRays.h"
#include "executors/ExRTReflections.h"

#include "shaders/ssr_trace_hq_interface.h"
#include "shaders/ssr_write_indirect_args_interface.h"

void Eng::Renderer::AddOITPasses(const CommonBuffers &common_buffers, const AccelerationStructures &acc_structs,
                                 const BindlessTextureData &bindless, const FgImgROHandle depth_hierarchy,
                                 const FgBufROHandle rt_geo_instances_res, const FgBufROHandle rt_obj_instances_res,
                                 FrameTextures &frame_textures) {
    using Stg = Ren::eStage;
    using Trg = Ren::eBindTarget;

    FgImgRWHandle oit_specular[OIT_REFLECTION_LAYERS];
    FgBufRWHandle oit_depth_buf, oit_ray_bitmask, oit_ray_counter;

    const int oit_layer_count =
        (settings.transparency_quality == eTransparencyQuality::Ultra) ? OIT_LAYERS_ULTRA : OIT_LAYERS_HIGH;

    { // OIT clear
        auto &oit_clear = fg_builder_.AddNode("OIT CLEAR");

        struct PassData {
            FgBufRWHandle oit_depth_buf;
            FgImgRWHandle oit_specular[OIT_REFLECTION_LAYERS];
            FgBufRWHandle oit_ray_counter;
            FgBufRWHandle oit_ray_bitmask;
        };

        auto *data = fg_builder_.AllocTempData<PassData>();

        { // output buffer
            FgBufDesc desc = {};
            desc.type = Ren::eBufType::Texture;
            desc.size = oit_layer_count * view_state_.ren_res[0] * view_state_.ren_res[1] * sizeof(uint32_t);
            desc.views.push_back(Ren::eFormat::R32UI);
            oit_depth_buf = data->oit_depth_buf = oit_clear.AddTransferOutput("Depth Values", desc);
        }

        // specular buffers (halfres)
        for (int i = 0; i < OIT_REFLECTION_LAYERS; ++i) {
            const std::string tex_name = "OIT REFL #" + std::to_string(i);

            FgImgDesc desc;
            desc.w = (view_state_.ren_res[0] / 2);
            desc.h = (view_state_.ren_res[1] / 2);
            desc.format = Ren::eFormat::RGBA16F;
            desc.sampling.filter = Ren::eFilter::Bilinear;
            desc.sampling.wrap = Ren::eWrap::ClampToEdge;

            oit_specular[i] = data->oit_specular[i] = oit_clear.AddClearImageOutput(tex_name, desc);
        }

        { // ray counter
            FgBufDesc desc = {};
            desc.type = Ren::eBufType::Storage;
            desc.size = 8 * sizeof(uint32_t);

            oit_ray_counter = data->oit_ray_counter = oit_clear.AddTransferOutput("OIT Ray Counter", desc);
        }

        { // ray bitmask (halfres)
            const int w = (((view_state_.ren_res[0] + 1) / 2) + 31) / 32;
            const int h = ((view_state_.ren_res[1] + 1) / 2);

            FgBufDesc desc = {};
            desc.type = Ren::eBufType::Storage;
            desc.size = OIT_REFLECTION_LAYERS * (w * h) * sizeof(uint32_t);

            oit_ray_bitmask = data->oit_ray_bitmask = oit_clear.AddTransferOutput("OIT Ray Bitmask", desc);
        }

        oit_clear.set_execute_cb([this, data](const FgContext &fg) {
            const Ren::BufferHandle out_depth_buf = fg.AccessRWBuffer(data->oit_depth_buf);
            const Ren::BufferHandle out_ray_counter_buf = fg.AccessRWBuffer(data->oit_ray_counter);
            const Ren::BufferHandle out_ray_bitmask_buf = fg.AccessRWBuffer(data->oit_ray_bitmask);

            { // OIT Ray Counter
                const auto &[buf_main, buf_cold] = fg.storages().buffers.Get(out_ray_counter_buf);
                Buffer_Fill(fg.ren_ctx().api(), buf_main, 0, buf_cold.size, 0, fg.cmd_buf());
            }

            if (p_list_->alpha_blend_start_index != -1) {
                const auto &[out_depth_buf_main, out_depth_buf_cold] = fg.storages().buffers.Get(out_depth_buf);
                Buffer_Fill(fg.ren_ctx().api(), out_depth_buf_main, 0, out_depth_buf_cold.size, 0, fg.cmd_buf());

                const auto &[out_ray_bitmask_buf_main, out_ray_bitmask_buf_cold] =
                    fg.storages().buffers.Get(out_ray_bitmask_buf);
                Buffer_Fill(fg.ren_ctx().api(), out_ray_bitmask_buf_main, 0, out_ray_bitmask_buf_cold.size, 0,
                            fg.cmd_buf());
            }
            for (int i = 0; i < OIT_REFLECTION_LAYERS; ++i) {
                const Ren::ImageRWHandle oit_specular = fg.AccessRWImage(data->oit_specular[i]);
                if (p_list_->alpha_blend_start_index != -1) {
                    fg.ren_ctx().CmdClearImage(oit_specular, {}, fg.cmd_buf());
                }
            }
        });
    }

    { // depth peel pass
        auto &oit_depth_peel = fg_builder_.AddNode("OIT DEPTH PEEL");
        const FgBufROHandle vtx_buf1 = oit_depth_peel.AddVertexBufferInput(common_buffers.vertex_buf1);
        const FgBufROHandle vtx_buf2 = oit_depth_peel.AddVertexBufferInput(common_buffers.vertex_buf2);
        const FgBufROHandle ndx_buf = oit_depth_peel.AddIndexBufferInput(common_buffers.indices_buf);

        const FgBufROHandle materials_buf =
            oit_depth_peel.AddStorageReadonlyInput(common_buffers.materials_buf, Stg::VertexShader);

        [[maybe_unused]] const FgImgROHandle noise_tex = oit_depth_peel.AddTextureInput(
            frame_textures.noise_tex, Ren::Bitmask{Stg::VertexShader} | Stg::FragmentShader);
        const FgImgROHandle white_tex = oit_depth_peel.AddTextureInput(
            frame_textures.dummy_white, Ren::Bitmask{Stg::VertexShader} | Stg::FragmentShader);
        const FgBufROHandle instances_buf =
            oit_depth_peel.AddStorageReadonlyInput(common_buffers.instance_buf, Stg::VertexShader);
        const FgBufROHandle instances_indices_buf =
            oit_depth_peel.AddStorageReadonlyInput(common_buffers.instance_indices, Stg::VertexShader);

        const FgBufROHandle shader_data_buf = oit_depth_peel.AddUniformBufferInput(
            common_buffers.shared_data, Ren::Bitmask{Stg::VertexShader} | Stg::FragmentShader);

        frame_textures.depth = oit_depth_peel.AddDepthOutput(MAIN_DEPTH_TEX, frame_textures.depth_desc);
        oit_depth_buf = oit_depth_peel.AddStorageOutput(oit_depth_buf, Stg::FragmentShader);

        oit_depth_peel.make_executor<ExOITDepthPeel>(&p_list_, &view_state_, vtx_buf1, vtx_buf2, ndx_buf, materials_buf,
                                                     &bindless, white_tex, instances_buf, instances_indices_buf,
                                                     shader_data_buf, frame_textures.depth, oit_depth_buf);
    }

    FgBufRWHandle oit_ray_list;

    { // schedule reflection rays
        auto &oit_schedule = fg_builder_.AddNode("OIT SCHEDULE");

        const FgBufROHandle vtx_buf1 = oit_schedule.AddVertexBufferInput(common_buffers.vertex_buf1);
        const FgBufROHandle vtx_buf2 = oit_schedule.AddVertexBufferInput(common_buffers.vertex_buf2);
        const FgBufROHandle ndx_buf = oit_schedule.AddIndexBufferInput(common_buffers.indices_buf);

        const FgBufROHandle materials_buf =
            oit_schedule.AddStorageReadonlyInput(common_buffers.materials_buf, Stg::VertexShader);

        [[maybe_unused]] const FgImgROHandle noise_tex = oit_schedule.AddTextureInput(
            frame_textures.noise_tex, Ren::Bitmask{Stg::VertexShader} | Stg::FragmentShader);
        const FgImgROHandle white_tex = oit_schedule.AddTextureInput(
            frame_textures.dummy_white, Ren::Bitmask{Stg::VertexShader} | Stg::FragmentShader);
        const FgBufROHandle instances_buf =
            oit_schedule.AddStorageReadonlyInput(common_buffers.instance_buf, Stg::VertexShader);
        const FgBufROHandle instances_indices_buf =
            oit_schedule.AddStorageReadonlyInput(common_buffers.instance_indices, Stg::VertexShader);

        const FgBufROHandle shader_data_buf = oit_schedule.AddUniformBufferInput(
            common_buffers.shared_data, Ren::Bitmask{Stg::VertexShader} | Stg::FragmentShader);

        frame_textures.depth = oit_schedule.AddDepthOutput(MAIN_DEPTH_TEX, frame_textures.depth_desc);
        oit_schedule.AddStorageReadonlyInput(oit_depth_buf, Stg::FragmentShader);

        oit_ray_counter = oit_schedule.AddStorageOutput(oit_ray_counter, Stg::FragmentShader);
        { // packed ray list
            FgBufDesc desc = {};
            desc.type = Ren::eBufType::Storage;
            desc.size = OIT_REFLECTION_LAYERS * ((view_state_.ren_res[0] + 1) / 2) *
                        ((view_state_.ren_res[1] + 1) / 2) * 2 * sizeof(uint32_t);

            oit_ray_list = oit_schedule.AddStorageOutput("OIT Ray List", desc, Stg::FragmentShader);
        }
        oit_ray_bitmask = oit_schedule.AddStorageOutput(oit_ray_bitmask, Stg::FragmentShader);

        oit_schedule.make_executor<ExOITScheduleRays>(&p_list_, &view_state_, vtx_buf1, vtx_buf2, ndx_buf,
                                                      materials_buf, &bindless, noise_tex, white_tex, instances_buf,
                                                      instances_indices_buf, shader_data_buf, frame_textures.depth,
                                                      oit_depth_buf, oit_ray_counter, oit_ray_list, oit_ray_bitmask);
    }

    FgBufROHandle indir_disp_buf;

    { // Write indirect arguments
        auto &write_indir = fg_builder_.AddNode("OIT INDIR ARGS");

        struct PassData {
            FgBufRWHandle ray_counter;
            FgBufRWHandle indir_disp_buf;
        };

        auto *data = fg_builder_.AllocTempData<PassData>();
        oit_ray_counter = data->ray_counter = write_indir.AddStorageOutput(oit_ray_counter, Stg::ComputeShader);

        { // Indirect arguments
            FgBufDesc desc = {};
            desc.type = Ren::eBufType::Indirect;
            desc.size = 3 * sizeof(Ren::DispatchIndirectCommand);

            indir_disp_buf = data->indir_disp_buf =
                write_indir.AddStorageOutput("OIT Intersect Args", desc, Stg::ComputeShader);
        }

        write_indir.set_execute_cb([this, data](const FgContext &fg) {
            using namespace SSRWriteIndirectArgs;

            const Ren::BufferHandle ray_counter_buf = fg.AccessRWBuffer(data->ray_counter);
            const Ren::BufferHandle indir_args = fg.AccessRWBuffer(data->indir_disp_buf);

            const Ren::Binding bindings[] = {{Trg::SBufRW, RAY_COUNTER_SLOT, ray_counter_buf},
                                             {Trg::SBufRW, INDIR_ARGS_SLOT, indir_args}};

            DispatchCompute(fg.cmd_buf(), pi_ssr_write_indirect_[0], fg.storages(), Ren::Vec3u{1u, 1u, 1u}, bindings,
                            nullptr, 0, fg.descr_alloc(), fg.log());
        });
    }

    FgBufRWHandle ray_rt_list;

    if (settings.reflections_quality != eReflectionsQuality::Raytraced_High) {
        auto &ssr_trace_hq = fg_builder_.AddNode("OIT SSR");

        struct PassData {
            FgBufROHandle oit_depth_buf;
            FgBufROHandle shared_data;
            FgImgROHandle color_tex, normal_tex;
            FgImgROHandle depth_hierarchy;

            FgImgROHandle albedo_tex, specular_tex;
            FgImgROHandle irradiance_tex, distance_tex, offset_tex;
            FgImgROHandle ltc_luts;

            FgBufROHandle in_ray_list, indir_args;
            FgBufRWHandle inout_ray_counter;
            FgImgRWHandle out_ssr_tex[OIT_REFLECTION_LAYERS];
            FgBufRWHandle out_ray_list;
        };

        auto *data = fg_builder_.AllocTempData<PassData>();
        data->oit_depth_buf = ssr_trace_hq.AddStorageReadonlyInput(oit_depth_buf, Stg::ComputeShader);
        data->shared_data = ssr_trace_hq.AddUniformBufferInput(common_buffers.shared_data, Stg::ComputeShader);
        data->color_tex = ssr_trace_hq.AddTextureInput(frame_textures.color, Stg::ComputeShader);
        data->normal_tex = ssr_trace_hq.AddTextureInput(frame_textures.normal, Stg::ComputeShader);
        data->depth_hierarchy = ssr_trace_hq.AddTextureInput(depth_hierarchy, Stg::ComputeShader);

        if (frame_textures.gi_cache_irradiance) {
            data->albedo_tex = ssr_trace_hq.AddTextureInput(frame_textures.albedo, Stg::ComputeShader);
            data->specular_tex = ssr_trace_hq.AddTextureInput(frame_textures.specular, Stg::ComputeShader);
            data->ltc_luts = ssr_trace_hq.AddTextureInput(frame_textures.ltc_luts, Stg::ComputeShader);
            data->irradiance_tex = ssr_trace_hq.AddTextureInput(frame_textures.gi_cache_irradiance, Stg::ComputeShader);
            data->distance_tex = ssr_trace_hq.AddTextureInput(frame_textures.gi_cache_distance, Stg::ComputeShader);
            data->offset_tex = ssr_trace_hq.AddTextureInput(frame_textures.gi_cache_offset, Stg::ComputeShader);
        }

        data->in_ray_list = ssr_trace_hq.AddStorageReadonlyInput(oit_ray_list, Stg::ComputeShader);
        data->indir_args = ssr_trace_hq.AddIndirectBufferInput(indir_disp_buf);
        data->inout_ray_counter = oit_ray_counter = ssr_trace_hq.AddStorageOutput(oit_ray_counter, Stg::ComputeShader);
        for (int i = 0; i < OIT_REFLECTION_LAYERS; ++i) {
            oit_specular[i] = data->out_ssr_tex[i] =
                ssr_trace_hq.AddStorageImageOutput(oit_specular[i], Stg::ComputeShader);
        }

        { // packed ray list
            FgBufDesc desc = {};
            desc.type = Ren::eBufType::Storage;
            desc.size = OIT_REFLECTION_LAYERS * ((view_state_.ren_res[0] + 1) / 2) *
                        ((view_state_.ren_res[1] + 1) / 2) * 2 * sizeof(uint32_t);

            ray_rt_list = data->out_ray_list =
                ssr_trace_hq.AddStorageOutput("OIT RT Ray List", desc, Stg::ComputeShader);
        }

        ssr_trace_hq.set_execute_cb([this, data](const FgContext &fg) {
            using namespace SSRTraceHQ;

            const Ren::BufferROHandle oit_depth_buf = fg.AccessROBuffer(data->oit_depth_buf);
            const Ren::BufferROHandle unif_sh_data_buf = fg.AccessROBuffer(data->shared_data);
            const Ren::ImageROHandle color_tex = fg.AccessROImage(data->color_tex);
            const Ren::ImageROHandle normal_tex = fg.AccessROImage(data->normal_tex);
            const Ren::ImageROHandle depth_hierarchy_tex = fg.AccessROImage(data->depth_hierarchy);
            const Ren::BufferROHandle in_ray_list_buf = fg.AccessROBuffer(data->in_ray_list);
            const Ren::BufferROHandle indir_args_buf = fg.AccessROBuffer(data->indir_args);

            Ren::ImageROHandle albedo_tex, specular_tex;
            Ren::ImageROHandle irr_tex, dist_tex, off_tex;
            Ren::ImageROHandle ltc_luts;
            if (data->irradiance_tex) {
                albedo_tex = fg.AccessROImage(data->albedo_tex);
                specular_tex = fg.AccessROImage(data->specular_tex);
                ltc_luts = fg.AccessROImage(data->ltc_luts);

                irr_tex = fg.AccessROImage(data->irradiance_tex);
                dist_tex = fg.AccessROImage(data->distance_tex);
                off_tex = fg.AccessROImage(data->offset_tex);
            }

            Ren::ImageRWHandle out_refl_tex[OIT_REFLECTION_LAYERS] = {};
            for (int i = 0; i < OIT_REFLECTION_LAYERS; ++i) {
                out_refl_tex[i] = fg.AccessRWImage(data->out_ssr_tex[i]);
            }
            const Ren::BufferHandle inout_ray_counter_buf = fg.AccessRWBuffer(data->inout_ray_counter);
            const Ren::BufferHandle out_ray_list_buf = fg.AccessRWBuffer(data->out_ray_list);

            Ren::SmallVector<Ren::Binding, 24> bindings = {{Trg::UBuf, BIND_UB_SHARED_DATA_BUF, unif_sh_data_buf},
                                                           {Trg::TexSampled, DEPTH_TEX_SLOT, depth_hierarchy_tex},
                                                           {Trg::TexSampled, COLOR_TEX_SLOT, color_tex},
                                                           {Trg::TexSampled, NORM_TEX_SLOT, normal_tex},
                                                           {Trg::UTBuf, OIT_DEPTH_BUF_SLOT, oit_depth_buf},
                                                           {Trg::SBufRO, IN_RAY_LIST_SLOT, in_ray_list_buf},
                                                           {Trg::SBufRW, INOUT_RAY_COUNTER_SLOT, inout_ray_counter_buf},
                                                           {Trg::SBufRW, OUT_RAY_LIST_SLOT, out_ray_list_buf}};
            for (int i = 0; i < OIT_REFLECTION_LAYERS; ++i) {
                bindings.emplace_back(Trg::ImageRW, OUT_REFL_IMG_SLOT, i, 1, out_refl_tex[i]);
            }
            if (irr_tex) {
                bindings.emplace_back(Trg::TexSampled, ALBEDO_TEX_SLOT, albedo_tex);
                bindings.emplace_back(Trg::TexSampled, SPEC_TEX_SLOT, specular_tex);
                bindings.emplace_back(Trg::TexSampled, LTC_LUTS_TEX_SLOT, ltc_luts);

                bindings.emplace_back(Trg::TexSampled, IRRADIANCE_TEX_SLOT, irr_tex);
                bindings.emplace_back(Trg::TexSampled, DISTANCE_TEX_SLOT, dist_tex);
                bindings.emplace_back(Trg::TexSampled, OFFSET_TEX_SLOT, off_tex);
            }

            Params uniform_params;
            uniform_params.resolution = Ren::Vec4u(view_state_.ren_res[0], view_state_.ren_res[1], 0, 0);

            DispatchComputeIndirect(fg.cmd_buf(), pi_ssr_trace_hq_[1][bool(irr_tex)], fg.storages(), indir_args_buf, 0,
                                    bindings, &uniform_params, sizeof(uniform_params), fg.descr_alloc(), fg.log());
        });
    }

    if ((ctx_.capabilities.hwrt || ctx_.capabilities.swrt) && acc_structs.rt_tlas_buf[int(eTLASIndex::Main)] &&
        int(settings.reflections_quality) >= int(eReflectionsQuality::Raytraced_Normal)) {
        FgBufRWHandle indir_rt_disp_buf;

        { // Prepare arguments for indirect RT dispatch
            auto &rt_disp_args = fg_builder_.AddNode("OIT RT ARGS");

            struct PassData {
                FgBufRWHandle ray_counter;
                FgBufRWHandle indir_disp_buf;
            };

            auto *data = fg_builder_.AllocTempData<PassData>();
            oit_ray_counter = data->ray_counter = rt_disp_args.AddStorageOutput(oit_ray_counter, Stg::ComputeShader);

            { // Indirect arguments
                FgBufDesc desc = {};
                desc.type = Ren::eBufType::Indirect;
                desc.size = sizeof(Ren::TraceRaysIndirectCommand) + sizeof(Ren::DispatchIndirectCommand);

                indir_rt_disp_buf = data->indir_disp_buf =
                    rt_disp_args.AddStorageOutput("OIT RT Dispatch Args", desc, Stg::ComputeShader);
            }

            rt_disp_args.set_execute_cb([this, data](const FgContext &fg) {
                using namespace SSRWriteIndirectArgs;

                const Ren::BufferHandle ray_counter_buf = fg.AccessRWBuffer(data->ray_counter);
                const Ren::BufferHandle indir_disp_buf = fg.AccessRWBuffer(data->indir_disp_buf);

                const Ren::Binding bindings[] = {{Trg::SBufRW, RAY_COUNTER_SLOT, ray_counter_buf},
                                                 {Trg::SBufRW, INDIR_ARGS_SLOT, indir_disp_buf}};

                Params params = {};
                params.counter_index = (settings.reflections_quality == eReflectionsQuality::Raytraced_High) ? 1 : 6;

                DispatchCompute(fg.cmd_buf(), pi_ssr_write_indirect_[1], fg.storages(), Ren::Vec3u{1u, 1u, 1u},
                                bindings, &params, sizeof(params), fg.descr_alloc(), fg.log());
            });
        }

        { // Trace reflection rays
            auto &rt_refl = fg_builder_.AddNode("OIT RT REFL");

            const auto stage = Stg::ComputeShader;

            auto *data = fg_builder_.AllocTempData<ExRTReflections::Args>();
            data->geo_data = rt_refl.AddStorageReadonlyInput(rt_geo_instances_res, stage);
            data->materials = rt_refl.AddStorageReadonlyInput(common_buffers.materials_buf, stage);
            data->vtx_buf1 = rt_refl.AddStorageReadonlyInput(common_buffers.vertex_buf1, stage);
            data->vtx_buf2 = rt_refl.AddStorageReadonlyInput(common_buffers.vertex_buf2, stage);
            data->ndx_buf = rt_refl.AddStorageReadonlyInput(common_buffers.indices_buf, stage);
            data->shared_data = rt_refl.AddUniformBufferInput(common_buffers.shared_data, stage);
            data->depth_tex = rt_refl.AddTextureInput(frame_textures.depth, stage);
            data->normal_tex = rt_refl.AddTextureInput(frame_textures.normal, stage);
            data->env_tex = rt_refl.AddTextureInput(frame_textures.envmap, stage);
            data->ray_counter = rt_refl.AddStorageReadonlyInput(oit_ray_counter, stage);
            if (ray_rt_list) {
                data->ray_list = rt_refl.AddStorageReadonlyInput(ray_rt_list, stage);
            } else {
                data->ray_list = rt_refl.AddStorageReadonlyInput(oit_ray_list, stage);
            }
            data->indir_args = rt_refl.AddIndirectBufferInput(indir_rt_disp_buf);
            data->tlas_buf = rt_refl.AddStorageReadonlyInput(acc_structs.rt_tlas_buf[int(eTLASIndex::Main)], stage);
            data->lights = rt_refl.AddStorageReadonlyInput(common_buffers.lights, stage);
            data->shadow_depth = rt_refl.AddTextureInput(frame_textures.shadow_depth, stage);
            data->shadow_color = rt_refl.AddTextureInput(frame_textures.shadow_color, stage);
            data->ltc_luts = rt_refl.AddTextureInput(frame_textures.ltc_luts, stage);
            data->cells = rt_refl.AddStorageReadonlyInput(common_buffers.rt_cells, stage);
            data->items = rt_refl.AddStorageReadonlyInput(common_buffers.rt_items, stage);

            if (!ctx_.capabilities.hwrt) {
                data->swrt.root_node = acc_structs.swrt.rt_root_node;
                data->swrt.rt_blas_buf = rt_refl.AddStorageReadonlyInput(acc_structs.swrt.rt_blas_buf, stage);
                data->swrt.prim_ndx_buf = rt_refl.AddStorageReadonlyInput(acc_structs.swrt.rt_prim_indices_buf, stage);
                data->swrt.mesh_instances_buf = rt_refl.AddStorageReadonlyInput(rt_obj_instances_res, stage);
            }

            data->tlas = acc_structs.rt_tlases[int(eTLASIndex::Main)];

            if (settings.gi_quality != eGIQuality::Off) {
                data->irradiance_tex = rt_refl.AddTextureInput(frame_textures.gi_cache_irradiance, stage);
                data->distance_tex = rt_refl.AddTextureInput(frame_textures.gi_cache_distance, stage);
                data->offset_tex = rt_refl.AddTextureInput(frame_textures.gi_cache_offset, stage);
            }

            data->oit_depth_buf = rt_refl.AddStorageReadonlyInput(oit_depth_buf, stage);

            for (int i = 0; i < OIT_REFLECTION_LAYERS; ++i) {
                oit_specular[i] = data->out_refl_tex[i] = rt_refl.AddStorageImageOutput(oit_specular[i], stage);
            }

            data->layered = true;
            data->four_bounces = false;

            rt_refl.make_executor<ExRTReflections>(&view_state_, &bindless, data, false);
        }
    }

    for (int i = 0; i < oit_layer_count; ++i) {
        FgImgRWHandle back_color, back_depth;
        { // copy background
            const std::string pass_name = "OIT BACK #" + std::to_string(i);

            auto &oit_back = fg_builder_.AddNode(pass_name);

            struct PassData {
                FgImgROHandle in_back_color;
                FgImgROHandle in_back_depth;

                FgImgRWHandle out_back_color;
                FgImgRWHandle out_back_depth;
            };

            auto *data = fg_builder_.AllocTempData<PassData>();
            data->in_back_color = oit_back.AddTransferImageInput(frame_textures.color);
            data->in_back_depth = oit_back.AddTransferImageInput(frame_textures.depth);
            { // background color
                const std::string tex_name = "OIT Back Color #" + std::to_string(i);
                back_color = data->out_back_color =
                    oit_back.AddTransferImageOutput(tex_name, frame_textures.color_desc);
            }
            { // background depth
                const std::string tex_name = i == 0 ? OPAQUE_DEPTH_TEX : "OIT Back Depth #" + std::to_string(i);
                back_depth = data->out_back_depth =
                    oit_back.AddTransferImageOutput(tex_name, frame_textures.depth_desc);
                if (i == 0) {
                    frame_textures.opaque_depth = back_depth;
                }
            }

            oit_back.set_execute_cb([this, data, i](const FgContext &fg) {
                const Ren::ImageROHandle in_back_color_tex = fg.AccessROImage(data->in_back_color);
                const Ren::ImageROHandle in_back_depth_tex = fg.AccessROImage(data->in_back_depth);

                const Ren::ImageRWHandle out_back_color_tex = fg.AccessRWImage(data->out_back_color);
                const Ren::ImageRWHandle out_back_depth_tex = fg.AccessRWImage(data->out_back_depth);

                // TODO: Get rid of this!
                const auto &[img_main, img_cold] = fg.storages().images.Get(in_back_color_tex);
                const int w = img_cold.params.w, h = img_cold.params.h;

                if (p_list_->alpha_blend_start_index != -1) {
                    fg.ren_ctx().CmdCopyImageToImage(fg.cmd_buf(), in_back_color_tex, 0, 0, 0, 0, out_back_color_tex, 0,
                                                     0, 0, 0, 0, w, h, 1);
                }
                if (i == 0 || p_list_->alpha_blend_start_index != -1) {
                    fg.ren_ctx().CmdCopyImageToImage(fg.cmd_buf(), in_back_depth_tex, 0, 0, 0, 0, out_back_depth_tex, 0,
                                                     0, 0, 0, 0, w, h, 1);
                }
            });
        }
        { // blend
            const std::string pass_name = "OIT BLEND #" + std::to_string(i);

            auto &oit_blend_layer = fg_builder_.AddNode(pass_name);
            const FgBufROHandle vtx_buf1 = oit_blend_layer.AddVertexBufferInput(common_buffers.vertex_buf1);
            const FgBufROHandle vtx_buf2 = oit_blend_layer.AddVertexBufferInput(common_buffers.vertex_buf2);
            const FgBufROHandle ndx_buf = oit_blend_layer.AddIndexBufferInput(common_buffers.indices_buf);

            const FgBufROHandle materials_buf =
                oit_blend_layer.AddStorageReadonlyInput(common_buffers.materials_buf, Stg::VertexShader);

            const FgImgROHandle noise_tex = oit_blend_layer.AddTextureInput(
                frame_textures.noise_tex, Ren::Bitmask{Stg::VertexShader} | Stg::FragmentShader);
            const FgImgROHandle white_tex = oit_blend_layer.AddTextureInput(
                frame_textures.dummy_white, Ren::Bitmask{Stg::VertexShader} | Stg::FragmentShader);
            const FgBufROHandle instances_buf =
                oit_blend_layer.AddStorageReadonlyInput(common_buffers.instance_buf, Stg::VertexShader);
            const FgBufROHandle instances_indices_buf =
                oit_blend_layer.AddStorageReadonlyInput(common_buffers.instance_indices, Stg::VertexShader);

            const FgBufROHandle shader_data_buf = oit_blend_layer.AddUniformBufferInput(
                common_buffers.shared_data, Ren::Bitmask{Stg::VertexShader} | Stg::FragmentShader);

            const FgBufROHandle cells_buf =
                oit_blend_layer.AddStorageReadonlyInput(common_buffers.cells, Stg::FragmentShader);
            const FgBufROHandle items_buf =
                oit_blend_layer.AddStorageReadonlyInput(common_buffers.items, Stg::FragmentShader);
            const FgBufROHandle lights_buf =
                oit_blend_layer.AddStorageReadonlyInput(common_buffers.lights, Stg::FragmentShader);
            const FgBufROHandle decals_buf =
                oit_blend_layer.AddStorageReadonlyInput(common_buffers.decals, Stg::FragmentShader);

            const FgImgROHandle shadow_map =
                oit_blend_layer.AddTextureInput(frame_textures.shadow_depth, Stg::FragmentShader);
            const FgImgROHandle ltc_luts =
                oit_blend_layer.AddTextureInput(frame_textures.ltc_luts, Stg::FragmentShader);
            const FgImgROHandle env_tex = oit_blend_layer.AddTextureInput(frame_textures.envmap, Stg::FragmentShader);

            frame_textures.depth = oit_blend_layer.AddDepthOutput(MAIN_DEPTH_TEX, frame_textures.depth_desc);
            frame_textures.color = oit_blend_layer.AddColorOutput(frame_textures.color);
            const FgBufROHandle oit_depth_buf_ro =
                oit_blend_layer.AddStorageReadonlyInput(oit_depth_buf, Stg::FragmentShader);

            const int layer_index = oit_layer_count - 1 - i;

            FgImgROHandle specular_tex;
            if (layer_index < OIT_REFLECTION_LAYERS && settings.reflections_quality != eReflectionsQuality::Off) {
                specular_tex = oit_blend_layer.AddTextureInput(oit_specular[layer_index], Stg::FragmentShader);
            }

            FgImgROHandle irradiance_tex, distance_tex, offset_tex;
            if (settings.gi_quality != eGIQuality::Off && frame_textures.gi_cache_irradiance) {
                irradiance_tex =
                    oit_blend_layer.AddTextureInput(frame_textures.gi_cache_irradiance, Stg::FragmentShader);
                distance_tex = oit_blend_layer.AddTextureInput(frame_textures.gi_cache_distance, Stg::FragmentShader);
                offset_tex = oit_blend_layer.AddTextureInput(frame_textures.gi_cache_offset, Stg::FragmentShader);
            }

            oit_blend_layer.AddTextureInput(back_color, Stg::FragmentShader);
            oit_blend_layer.AddTextureInput(back_depth, Stg::FragmentShader);

            oit_blend_layer.make_executor<ExOITBlendLayer>(
                prim_draw_, &p_list_, &view_state_, vtx_buf1, vtx_buf2, ndx_buf, materials_buf, &bindless, cells_buf,
                items_buf, lights_buf, decals_buf, noise_tex, white_tex, shadow_map, ltc_luts, env_tex, instances_buf,
                instances_indices_buf, shader_data_buf, frame_textures.depth, frame_textures.color, oit_depth_buf_ro,
                specular_tex, layer_index, irradiance_tex, distance_tex, offset_tex, back_color, back_depth);
        }
    }

    frame_textures.oit_depth_buf = oit_depth_buf;
}