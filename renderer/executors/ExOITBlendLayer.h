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

    FgBufHandle vtx_buf1_;
    FgBufHandle vtx_buf2_;
    FgBufHandle ndx_buf_;
    FgBufHandle instances_buf_;
    FgBufHandle instance_indices_buf_;
    FgBufHandle shared_data_buf_;
    FgBufHandle materials_buf_;
    FgBufHandle cells_buf_;
    FgBufHandle items_buf_;
    FgBufHandle lights_buf_;
    FgBufHandle decals_buf_;
    FgResRef noise_tex_;
    FgResRef dummy_white_;
    FgResRef shadow_map_;
    FgResRef ltc_luts_tex_;
    FgResRef env_tex_;
    FgBufHandle oit_depth_buf_;
    int depth_layer_index_ = -1;
    FgResRef oit_specular_tex_;

    FgResRef irradiance_tex_;
    FgResRef distance_tex_;
    FgResRef offset_tex_;

    FgResRef back_color_tex_;
    FgResRef back_depth_tex_;

    FgResRef depth_tex_;
    FgResRef color_tex_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferHandle vtx_buf1, Ren::BufferHandle vtx_buf2,
                  Ren::BufferHandle ndx_buf, const Ren::WeakImgRef &depth_tex, const Ren::WeakImgRef &color_tex);
    void DrawTransparent(const FgContext &fg, const Ren::WeakImgRef &depth_tex);

  public:
    ExOITBlendLayer(PrimDraw &prim_draw, const DrawList **p_list, const view_state_t *view_state, FgBufHandle vtx_buf1,
                    FgBufHandle vtx_buf2, FgBufHandle ndx_buf, FgBufHandle materials_buf,
                    const BindlessTextureData *bindless_tex, FgBufHandle cells_buf, FgBufHandle items_buf,
                    FgBufHandle lights_buf, FgBufHandle decals_buf, FgResRef noise_tex, FgResRef dummy_white,
                    FgResRef shadow_map, FgResRef ltc_luts_tex, FgResRef env_tex, FgBufHandle instances_buf,
                    FgBufHandle instance_indices_buf, FgBufHandle shared_data_buf, FgResRef depth_tex,
                    FgResRef color_tex, FgBufHandle oit_depth_buf, FgResRef oit_specular_tex, int depth_layer_index,
                    FgResRef irradiance_tex, FgResRef distance_tex, FgResRef offset_tex, FgResRef back_color_tex,
                    FgResRef back_depth_tex);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng