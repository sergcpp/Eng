#pragma once

#include <Ren/Pipeline.h>

#include "../framegraph/FgNode.h"

namespace Eng {
class ShaderLoader;
struct view_state_t;
class ExDepthHierarchy final : public FgExecutor {
    Ren::PipelineHandle pi_depth_hierarchy_;

    const view_state_t *view_state_ = nullptr;

    FgImgROHandle depth_tex_;
    FgBufRWHandle atomic_buf_;
    FgImgRWHandle output_tex_;

  public:
    ExDepthHierarchy(ShaderLoader &sh, const view_state_t *view_state, FgImgROHandle depth_tex,
                     FgBufRWHandle atomic_counter, FgImgRWHandle output_tex);

    static const int MipCount = 7;
    // TODO: check if it is actually makes sense to use padding
    static const int TileSize = 1 << (MipCount - 1);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng