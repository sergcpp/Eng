#pragma once

#include "../framegraph/FgNode.h"

namespace Eng {
struct DrawList;
class PrimDraw;
struct probe_volume_t;
class ShaderLoader;
struct view_state_t;

class ExDebugProbes final : public FgExecutor {
  public:
    struct Args {
        FgBufROHandle shared_data;
        FgImgROHandle irradiance;
        FgImgROHandle distance;
        FgImgROHandle offset;

        FgImgRWHandle depth;
        FgImgRWHandle output;

        int volume_to_debug = 0;
        Ren::Span<const probe_volume_t> probe_volumes;
    };

    ExDebugProbes(PrimDraw &prim_draw, ShaderLoader &sh, const view_state_t *view_state, const Args *args);

    void Execute(const FgContext &fg) override;

  private:
    PrimDraw &prim_draw_;

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const Args *args_ = nullptr;

    // lazily initialized data
    Ren::ProgramHandle prog_probe_debug_;
};
} // namespace Eng