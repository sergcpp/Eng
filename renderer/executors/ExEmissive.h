#pragma once

#include <Ren/Common.h>
#include <Ren/Framebuffer.h>

#include "../framegraph/FgNode.h"

namespace Eng {
struct BindlessTextureData;
struct DrawList;
struct view_state_t;
class ShaderLoader;

class ExEmissive final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data
    Ren::PipelineHandle pi_simple_[3];
    Ren::PipelineHandle pi_vegetation_[2];

    // temp data
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    const Ren::ImageAtlas *decals_atlas_ = nullptr;

    const DrawList **p_list_ = nullptr;

    FgBufROHandle vtx_buf1_;
    FgBufROHandle vtx_buf2_;
    FgBufROHandle ndx_buf_;
    FgBufROHandle instances_buf_;
    FgBufROHandle instance_indices_buf_;
    FgBufROHandle shared_data_buf_;
    FgBufROHandle materials_buf_;
    FgImgROHandle noise_tex_;
    FgImgROHandle dummy_white_;

    FgImgRWHandle out_color_tex_;
    FgImgRWHandle out_depth_tex_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle color_tex, Ren::ImageRWHandle depth_tex);
    void DrawOpaque(const FgContext &fg, Ren::ImageRWHandle color_tex, Ren::ImageRWHandle depth_tex);

  public:
    ExEmissive(const DrawList **p_list, const view_state_t *view_state, const FgBufROHandle vtx_buf1,
               const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf, const FgBufROHandle materials_buf,
               const BindlessTextureData *bindless_tex, const FgImgROHandle noise_tex, const FgImgROHandle dummy_white,
               const FgBufROHandle instances_buf, const FgBufROHandle instance_indices_buf,
               const FgBufROHandle shared_data_buf, const FgImgRWHandle out_color, const FgImgRWHandle out_depth) {
        view_state_ = view_state;
        bindless_tex_ = bindless_tex;

        p_list_ = p_list;

        vtx_buf1_ = vtx_buf1;
        vtx_buf2_ = vtx_buf2;
        ndx_buf_ = ndx_buf;
        instances_buf_ = instances_buf;
        instance_indices_buf_ = instance_indices_buf;
        shared_data_buf_ = shared_data_buf;
        materials_buf_ = materials_buf;

        noise_tex_ = noise_tex;
        dummy_white_ = dummy_white;

        out_color_tex_ = out_color;
        out_depth_tex_ = out_depth;
    }

    void Execute(const FgContext &fg) override;
};
} // namespace Eng