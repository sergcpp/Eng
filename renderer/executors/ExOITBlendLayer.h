#pragma once

#include <Ren/Common.h>
#include <Ren/Framebuffer.h>

#include "../framegraph/FgNode.h"

namespace Eng {
struct BindlessTextureData;
struct DrawList;
class PrimDraw;
class ShaderLoader;
struct view_state_t;

class ExOITBlendLayer final : public FgExecutor {
    PrimDraw &prim_draw_;
    bool initialized = false;

    // lazily initialized data
    Ren::ProgramHandle prog_oit_blit_depth_;
    Ren::PipelineHandle pi_simple_[3];
    Ren::PipelineHandle pi_vegetation_[2];

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    const DrawList **p_list_ = nullptr;

    FgBufROHandle vtx_buf1_;
    FgBufROHandle vtx_buf2_;
    FgBufROHandle ndx_buf_;
    FgBufROHandle instances_buf_;
    FgBufROHandle instance_indices_buf_;
    FgBufROHandle shared_data_buf_;
    FgBufROHandle materials_buf_;
    FgBufROHandle cells_buf_;
    FgBufROHandle items_buf_;
    FgBufROHandle lights_buf_;
    FgBufROHandle decals_buf_;
    FgImgROHandle noise_tex_;
    FgImgROHandle dummy_white_;
    FgImgROHandle shadow_depth_;
    FgImgROHandle ltc_luts_;
    FgImgROHandle env_tex_;
    FgBufROHandle oit_depth_buf_;
    int depth_layer_index_ = -1;
    FgImgROHandle oit_specular_tex_;

    FgImgROHandle irradiance_tex_;
    FgImgROHandle distance_tex_;
    FgImgROHandle offset_tex_;

    FgImgROHandle back_color_tex_;
    FgImgROHandle back_depth_tex_;

    FgImgRWHandle depth_tex_;
    FgImgRWHandle color_tex_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle depth_tex, Ren::ImageRWHandle color_tex);
    void DrawTransparent(const FgContext &fg, Ren::ImageRWHandle depth_tex, Ren::ImageRWHandle color_tex);

  public:
    ExOITBlendLayer(PrimDraw &prim_draw, const DrawList **p_list, const view_state_t *view_state,
                    FgBufROHandle vtx_buf1, FgBufROHandle vtx_buf2, FgBufROHandle ndx_buf, FgBufROHandle materials_buf,
                    const BindlessTextureData *bindless_tex, FgBufROHandle cells_buf, FgBufROHandle items_buf,
                    FgBufROHandle lights_buf, FgBufROHandle decals_buf, FgImgROHandle noise_tex,
                    FgImgROHandle dummy_white, FgImgROHandle shadow_depth, FgImgROHandle ltc_luts,
                    FgImgROHandle env_tex, FgBufROHandle instances_buf, FgBufROHandle instance_indices_buf,
                    FgBufROHandle shared_data_buf, FgImgRWHandle depth_tex, FgImgRWHandle color_tex,
                    FgBufROHandle oit_depth_buf, FgImgROHandle oit_specular_tex, int depth_layer_index,
                    FgImgROHandle irradiance_tex, FgImgROHandle distance_tex, FgImgROHandle offset_tex,
                    FgImgROHandle back_color_tex, FgImgROHandle back_depth_tex);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng