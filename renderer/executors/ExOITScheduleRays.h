#pragma once

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

#include <Ren/VertexInput.h>

namespace Eng {
class PrimDraw;

class ExOITScheduleRays final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data
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
    FgResRef noise_tex_;
    FgResRef dummy_white_;
    FgBufHandle oit_depth_buf_;
    FgBufHandle ray_counter_;
    FgBufHandle ray_list_;
    FgBufHandle ray_bitmask_;

    FgResRef depth_tex_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferHandle vtx_buf1, Ren::BufferHandle vtx_buf2,
                  Ren::BufferHandle ndx_buf, const Ren::WeakImgRef &depth_tex);
    void DrawTransparent(const FgContext &fg, const Ren::WeakImgRef &depth_tex);

  public:
    ExOITScheduleRays(const DrawList **p_list, const view_state_t *view_state, FgBufHandle vtx_buf1,
                      FgBufHandle vtx_buf2, FgBufHandle ndx_buf, FgBufHandle materials_buf,
                      const BindlessTextureData *bindless_tex, FgResRef noise_tex, FgResRef dummy_white,
                      FgBufHandle instances_buf, FgBufHandle instance_indices_buf, FgBufHandle shared_data_buf,
                      FgResRef depth_tex, FgBufHandle oit_depth_buf, FgBufHandle ray_counter, FgBufHandle ray_list,
                      FgBufHandle ray_bitmask);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng