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
    FgBufROHandle instances_;
    FgBufROHandle instance_indices_;
    FgBufROHandle shared_data_;
    FgBufROHandle materials_;
    FgBufROHandle cells_;
    FgBufROHandle items_;
    FgBufROHandle lights_;
    FgBufROHandle decals_;
    FgImgROHandle noise_;
    FgImgROHandle dummy_white_;
    FgImgROHandle shadow_depth_;
    FgImgROHandle ltc_luts_;
    FgImgROHandle env_;
    FgBufROHandle oit_depth_;
    int depth_layer_index_ = -1;
    FgImgROHandle oit_specular_;

    FgImgROHandle irradiance_;
    FgImgROHandle distance_;
    FgImgROHandle offset_;

    FgImgROHandle back_color_;
    FgImgROHandle back_depth_;

    FgImgRWHandle depth_;
    FgImgRWHandle color_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle depth, Ren::ImageRWHandle color);
    void DrawTransparent(const FgContext &fg, Ren::ImageRWHandle depth, Ren::ImageRWHandle color);

  public:
    ExOITBlendLayer(PrimDraw &prim_draw, const DrawList **p_list, const view_state_t *view_state,
                    FgBufROHandle vtx_buf1, FgBufROHandle vtx_buf2, FgBufROHandle ndx_buf, FgBufROHandle materials,
                    const BindlessTextureData *bindless_tex, FgBufROHandle cells, FgBufROHandle items,
                    FgBufROHandle lights, FgBufROHandle decals, FgImgROHandle noise, FgImgROHandle dummy_white,
                    FgImgROHandle shadow_depth, FgImgROHandle ltc_luts, FgImgROHandle env, FgBufROHandle instances,
                    FgBufROHandle instance_indices, FgBufROHandle shared_data, FgImgRWHandle depth, FgImgRWHandle color,
                    FgBufROHandle oit_depth, FgImgROHandle oit_specular, int depth_layer_index,
                    FgImgROHandle irradiance, FgImgROHandle distance, FgImgROHandle offset, FgImgROHandle back_color,
                    FgImgROHandle back_depth);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng