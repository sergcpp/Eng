#pragma once

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

#include <Ren/VertexInput.h>

namespace Eng {
class PrimDraw;

class ExOITBlendLayer final : public FgExecutor {
    PrimDraw &prim_draw_;
    bool initialized = false;

    // lazily initialized data
    Ren::ProgramHandle prog_oit_blit_depth_;
    Ren::PipelineHandle pi_simple_[3];
    Ren::PipelineHandle pi_vegetation_[2];
    Ren::Framebuffer main_draw_fb_[Ren::MaxFramesInFlight][2];
    int fb_to_use_ = 0;

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
    FgResRef noise_tex_;
    FgResRef dummy_white_;
    FgResRef shadow_map_;
    FgResRef ltc_luts_tex_;
    FgResRef env_tex_;
    FgBufROHandle oit_depth_buf_;
    int depth_layer_index_ = -1;
    FgResRef oit_specular_tex_;

    FgResRef irradiance_tex_;
    FgResRef distance_tex_;
    FgResRef offset_tex_;

    FgResRef back_color_tex_;
    FgResRef back_depth_tex_;

    FgResRef depth_tex_;
    FgResRef color_tex_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferROHandle vtx_buf1, Ren::BufferROHandle vtx_buf2,
                  Ren::BufferROHandle ndx_buf, const Ren::WeakImgRef &depth_tex, const Ren::WeakImgRef &color_tex);
    void DrawTransparent(const FgContext &fg, const Ren::WeakImgRef &depth_tex);

  public:
    ExOITBlendLayer(PrimDraw &prim_draw, const DrawList **p_list, const view_state_t *view_state,
                    FgBufROHandle vtx_buf1, FgBufROHandle vtx_buf2, FgBufROHandle ndx_buf, FgBufROHandle materials_buf,
                    const BindlessTextureData *bindless_tex, FgBufROHandle cells_buf, FgBufROHandle items_buf,
                    FgBufROHandle lights_buf, FgBufROHandle decals_buf, FgResRef noise_tex, FgResRef dummy_white,
                    FgResRef shadow_map, FgResRef ltc_luts_tex, FgResRef env_tex, FgBufROHandle instances_buf,
                    FgBufROHandle instance_indices_buf, FgBufROHandle shared_data_buf, FgResRef depth_tex,
                    FgResRef color_tex, FgBufROHandle oit_depth_buf, FgResRef oit_specular_tex, int depth_layer_index,
                    FgResRef irradiance_tex, FgResRef distance_tex, FgResRef offset_tex, FgResRef back_color_tex,
                    FgResRef back_depth_tex);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng