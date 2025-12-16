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

    FgBufHandle vtx_buf1_;
    FgBufHandle vtx_buf2_;
    FgBufHandle ndx_buf_;
    FgBufHandle instances_buf_;
    FgBufHandle instance_indices_buf_;
    FgBufHandle shared_data_buf_;
    FgBufHandle materials_buf_;
    FgResRef dummy_white_;

    FgResRef depth_tex_;
    FgBufHandle out_depth_buf_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferHandle vtx_buf1, Ren::BufferHandle vtx_buf2,
                  Ren::BufferHandle ndx_buf, Ren::WeakImgRef depth_tex);
    void DrawTransparent(const FgContext &fg);

  public:
    ExOITDepthPeel(const DrawList **p_list, const view_state_t *view_state, FgBufHandle vtx_buf1, FgBufHandle vtx_buf2,
                   FgBufHandle ndx_buf, FgBufHandle materials_buf, const BindlessTextureData *bindless_tex,
                   FgResRef dummy_white, FgBufHandle instances_buf, FgBufHandle instance_indices_buf,
                   FgBufHandle shared_data_buf, FgResRef depth_tex, FgBufHandle out_depth_buf);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng