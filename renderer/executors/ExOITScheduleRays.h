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

    FgBufROHandle vtx_buf1_;
    FgBufROHandle vtx_buf2_;
    FgBufROHandle ndx_buf_;
    FgBufROHandle instances_buf_;
    FgBufROHandle instance_indices_buf_;
    FgBufROHandle shared_data_buf_;
    FgBufROHandle materials_buf_;
    FgResRef noise_tex_;
    FgResRef dummy_white_;
    FgBufROHandle oit_depth_buf_;

    FgBufRWHandle ray_counter_;
    FgBufRWHandle ray_list_;
    FgBufRWHandle ray_bitmask_;

    FgResRef depth_tex_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferROHandle vtx_buf1, Ren::BufferROHandle vtx_buf2,
                  Ren::BufferROHandle ndx_buf, const Ren::WeakImgRef &depth_tex);
    void DrawTransparent(const FgContext &fg, const Ren::WeakImgRef &depth_tex);

  public:
    ExOITScheduleRays(const DrawList **p_list, const view_state_t *view_state, FgBufROHandle vtx_buf1,
                      FgBufROHandle vtx_buf2, FgBufROHandle ndx_buf, FgBufROHandle materials_buf,
                      const BindlessTextureData *bindless_tex, FgResRef noise_tex, FgResRef dummy_white,
                      FgBufROHandle instances_buf, FgBufROHandle instance_indices_buf, FgBufROHandle shared_data_buf,
                      FgResRef depth_tex, FgBufROHandle oit_depth_buf, FgBufRWHandle ray_counter,
                      FgBufRWHandle ray_list, FgBufRWHandle ray_bitmask);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng