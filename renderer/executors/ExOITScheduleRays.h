#pragma once

#include <Ren/Common.h>
#include <Ren/Framebuffer.h>

#include "../framegraph/FgNode.h"

namespace Eng {
struct BindlessTextureData;
struct DrawList;
class PrimDraw;
struct view_state_t;
class ShaderLoader;

class ExOITScheduleRays final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data
    Ren::PipelineHandle pi_simple_[3];
    Ren::PipelineHandle pi_vegetation_[2];

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
    FgBufROHandle oit_depth_buf_;
    FgImgROHandle noise_tex_;
    FgImgROHandle dummy_white_;

    FgBufRWHandle ray_counter_;
    FgBufRWHandle ray_list_;
    FgBufRWHandle ray_bitmask_;

    FgImgRWHandle depth_tex_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle depth_tex);
    void DrawTransparent(const FgContext &fg, Ren::ImageRWHandle depth_tex);

  public:
    ExOITScheduleRays(const DrawList **p_list, const view_state_t *view_state, FgBufROHandle vtx_buf1,
                      FgBufROHandle vtx_buf2, FgBufROHandle ndx_buf, FgBufROHandle materials_buf,
                      const BindlessTextureData *bindless_tex, FgImgROHandle noise_tex, FgImgROHandle dummy_white,
                      FgBufROHandle instances_buf, FgBufROHandle instance_indices_buf, FgBufROHandle shared_data_buf,
                      FgImgRWHandle depth_tex, FgBufROHandle oit_depth_buf, FgBufRWHandle ray_counter,
                      FgBufRWHandle ray_list, FgBufRWHandle ray_bitmask);

    void Execute(const FgContext &fg) override;
};
} // namespace Eng