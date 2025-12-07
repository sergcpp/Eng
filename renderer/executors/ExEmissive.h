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
    FgBufROHandle instances_;
    FgBufROHandle instance_indices_;
    FgBufROHandle shared_data_;
    FgBufROHandle materials_;
    FgImgROHandle noise_;
    FgImgROHandle dummy_white_;

    FgImgRWHandle out_color_;
    FgImgRWHandle out_depth_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle color, Ren::ImageRWHandle depth);
    void DrawOpaque(const FgContext &fg, Ren::ImageRWHandle color, Ren::ImageRWHandle depth);

  public:
    ExEmissive(const DrawList **p_list, const view_state_t *view_state, const FgBufROHandle vtx_buf1,
               const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf, const FgBufROHandle materials,
               const BindlessTextureData *bindless_tex, const FgImgROHandle noise, const FgImgROHandle dummy_white,
               const FgBufROHandle instances, const FgBufROHandle instance_indices, const FgBufROHandle shared_data,
               const FgImgRWHandle out_color, const FgImgRWHandle out_depth) {
        view_state_ = view_state;
        bindless_tex_ = bindless_tex;

        p_list_ = p_list;

        vtx_buf1_ = vtx_buf1;
        vtx_buf2_ = vtx_buf2;
        ndx_buf_ = ndx_buf;
        instances_ = instances;
        instance_indices_ = instance_indices;
        shared_data_ = shared_data;
        materials_ = materials;

        noise_ = noise;
        dummy_white_ = dummy_white;

        out_color_ = out_color;
        out_depth_ = out_depth;
    }

    void Execute(const FgContext &fg) override;
};
} // namespace Eng