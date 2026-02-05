#pragma once

#include "../framegraph/FgNode.h"

namespace Eng {
class PrimDraw;
class ShaderLoader;
struct view_state_t;

class ExPostprocess final : public FgExecutor {
  public:
    struct Args {
        FgImgROHandle exposure_tex;
        FgImgROHandle color_tex;
        FgImgROHandle bloom_tex;
        FgImgROHandle lut_tex;

        FgImgRWHandle output_tex;
        FgImgRWHandle output_tex2;

        Ren::SamplerHandle linear_sampler;

        int tonemap_mode = 1;
        float inv_gamma = 1.0f, fade = 0.0f;
        float aberration = 1.0f;
        float purkinje = 1.0f;
    };

    ExPostprocess(PrimDraw &prim_draw, ShaderLoader &sh, const view_state_t *view_state, const Args *args);

    void Execute(const FgContext &fg) override;

  private:
    PrimDraw &prim_draw_;

    Ren::ProgramHandle blit_postprocess_prog_[2][2];

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const Args *args_ = nullptr;
};
} // namespace Eng