#pragma once

#include <Ren/Image.h>
#include <Ren/VertexInput.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

struct view_state_t;

namespace Eng {
class ExDebugOIT final : public FgExecutor {
  public:
    struct Args {
        int layer_index = 0;

        FgBufROHandle oit_depth_buf;
        FgResRef output_tex;
    };

    ExDebugOIT(ShaderLoader &sh, const view_state_t *view_state, const Args *pass_data);

    void Execute(const FgContext &fg) override;

  private:
    // lazily initialized data
    Ren::PipelineHandle pi_debug_oit_;

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const Args *args_ = nullptr;
};
} // namespace Eng