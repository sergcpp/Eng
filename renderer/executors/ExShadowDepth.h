#pragma once

#include <Ren/Framebuffer.h>

#include "../framegraph/FgNode.h"

namespace Eng {
struct BindlessTextureData;
struct DrawList;
class ShaderLoader;

class ExShadowDepth final : public FgExecutor {
    bool initialized = false;
    int w_, h_;

    // lazily initialized data
    Ren::PipelineHandle pi_solid_[3], pi_alpha_[3];
    Ren::PipelineHandle pi_vege_solid_, pi_vege_alpha_;

    // temp data (valid only between Setup and Execute calls)
    const DrawList **p_list_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    // inputs
    FgBufROHandle vtx_buf1_;
    FgBufROHandle vtx_buf2_;
    FgBufROHandle ndx_buf_;
    FgBufROHandle instances_buf_;
    FgBufROHandle instance_indices_buf_;
    FgBufROHandle shared_data_buf_;
    FgBufROHandle materials_buf_;
    FgImgROHandle noise_tex_;

    // outputs
    FgImgRWHandle shadow_depth_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle shadow_depth);
    void DrawShadowMaps(const FgContext &fg, Ren::ImageRWHandle shadow_depth);

  public:
    ExShadowDepth(const int w, const int h, const DrawList **p_list, const FgBufROHandle vtx_buf1,
                  const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf, const FgBufROHandle materials_buf,
                  const BindlessTextureData *bindless_tex, const FgBufROHandle instances_buf,
                  const FgBufROHandle instance_indices_buf, const FgBufROHandle shared_data_buf,
                  const FgImgROHandle noise_tex, const FgImgRWHandle shadow_depth)
        : w_(w), h_(h) {
        p_list_ = p_list;
        bindless_tex_ = bindless_tex;

        vtx_buf1_ = vtx_buf1;
        vtx_buf2_ = vtx_buf2;
        ndx_buf_ = ndx_buf;

        instances_buf_ = instances_buf;
        instance_indices_buf_ = instance_indices_buf;
        shared_data_buf_ = shared_data_buf;
        materials_buf_ = materials_buf;
        noise_tex_ = noise_tex;

        shadow_depth_ = shadow_depth;
    }

    void Execute(const FgContext &fg) override;
};
} // namespace Eng