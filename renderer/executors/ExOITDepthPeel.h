#pragma once

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

#include <Ren/VertexInput.h>

namespace Eng {
class PrimDraw;

class ExOITDepthPeel final : public FgExecutor {
    Ren::PipelineHandle pi_simple_[3];

    // lazily initialized data
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
    FgResRef dummy_white_;

    FgResRef depth_tex_;
    FgBufRWHandle out_depth_buf_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferROHandle vtx_buf1, Ren::BufferROHandle vtx_buf2,
                  Ren::BufferROHandle ndx_buf, Ren::WeakImgRef depth_tex);
    void DrawTransparent(const FgContext &fg);

  public:
    ExOITDepthPeel(const DrawList **p_list, const view_state_t *view_state, FgBufROHandle vtx_buf1,
                   FgBufROHandle vtx_buf2, FgBufROHandle ndx_buf, FgBufROHandle materials_buf,
                   const BindlessTextureData *bindless_tex, FgResRef dummy_white, FgBufROHandle instances_buf,
                   FgBufROHandle instance_indices_buf, FgBufROHandle shared_data_buf, FgResRef depth_tex,
                   FgBufRWHandle out_depth_buf);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng