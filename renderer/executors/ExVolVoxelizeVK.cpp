#include "ExVolVoxelize.h"

#include <Ren/Context.h>
#include <Ren/DrawCall.h>
#include <Ren/VKCtx.h>

#include "../../utils/ShaderLoader.h"
#include "../Renderer_DrawList.h"
#include "../framegraph/FgBuilder.h"
#include "../shaders/vol_interface.h"

void Eng::ExVolVoxelize::Execute_HWRT(const FgContext &fg) {
    const Ren::BufferROHandle unif_sh_data_buf = fg.AccessROBuffer(args_->shared_data);
    const Ren::ImageROHandle stbn_tex = fg.AccessROImage(args_->stbn_tex);

    const Ren::BufferROHandle geo_data_buf = fg.AccessROBuffer(args_->geo_data);
    const Ren::BufferROHandle materials_buf = fg.AccessROBuffer(args_->materials);
    [[maybe_unused]] const Ren::BufferROHandle tlas_buf = fg.AccessROBuffer(args_->tlas_buf);

    const Ren::ImageRWHandle out_emission_tex = fg.AccessRWImage(args_->out_emission_tex);
    const Ren::ImageRWHandle out_scatter_tex = fg.AccessRWImage(args_->out_scatter_tex);

    if (view_state_->skip_volumetrics) {
        return;
    }

    auto *acc_struct = static_cast<Ren::AccStructureVK *>(args_->tlas);

    const Ren::Binding bindings[] = {{Ren::eBindTarget::UBuf, BIND_UB_SHARED_DATA_BUF, unif_sh_data_buf},
                                     {Ren::eBindTarget::TexSampled, Fog::STBN_TEX_SLOT, stbn_tex},
                                     {Ren::eBindTarget::SBufRO, Fog::GEO_DATA_BUF_SLOT, geo_data_buf},
                                     {Ren::eBindTarget::SBufRO, Fog::MATERIAL_BUF_SLOT, materials_buf},
                                     {Ren::eBindTarget::AccStruct, Fog::TLAS_SLOT, *acc_struct},
                                     {Ren::eBindTarget::ImageRW, Fog::OUT_FR_EMISSION_IMG_SLOT, out_emission_tex},
                                     {Ren::eBindTarget::ImageRW, Fog::OUT_FR_SCATTER_IMG_SLOT, out_scatter_tex}};

    const Ren::ImageCold &img_cold = fg.storages().images.Get(out_emission_tex).second;
    const auto froxel_res = Ren::Vec4i{img_cold.params.w, img_cold.params.h, img_cold.params.d, 0};

    const Ren::Vec3u grp_count = Ren::Vec3u(Ren::DivCeil(froxel_res[0], Fog::GRP_SIZE_2D_X),
                                            Ren::DivCeil(froxel_res[1], Fog::GRP_SIZE_2D_Y), 1u);

    Fog::Params uniform_params;
    uniform_params.froxel_res = froxel_res;
    uniform_params.scatter_color =
        Ren::Saturate(Ren::Vec4f{(*p_list_)->env.fog.scatter_color, (*p_list_)->env.fog.absorption});
    uniform_params.emission_color =
        Ren::Vec4f{(*p_list_)->env.fog.emission_color, std::max((*p_list_)->env.fog.density, 0.0f)};
    uniform_params.anisotropy = Ren::Clamp((*p_list_)->env.fog.anisotropy, -0.99f, 0.99f);
    uniform_params.bbox_min = Ren::Vec4f{(*p_list_)->env.fog.bbox_min};
    uniform_params.bbox_max = Ren::Vec4f{(*p_list_)->env.fog.bbox_max};
    uniform_params.frame_index = view_state_->frame_index;
    uniform_params.hist_weight = (view_state_->pre_exposure / view_state_->prev_pre_exposure);

    DispatchCompute(fg.cmd_buf(), pi_vol_voxelize_, fg.storages(), grp_count, bindings, &uniform_params,
                    sizeof(uniform_params), fg.descr_alloc(), fg.log());
}
