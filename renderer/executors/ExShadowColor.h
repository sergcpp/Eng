#pragma once

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

#include <Ren/VertexInput.h>

namespace Eng {
class ExShadowColor final : public FgExecutor {
    bool initialized = false;
    int w_, h_;

    // lazily initialized data
    Ren::PipelineHandle pi_solid_[3], pi_alpha_[3];
    Ren::PipelineHandle pi_vege_solid_, pi_vege_alpha_;

    Ren::Framebuffer shadow_fb_;

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
    FgResRef noise_tex_;

    // outputs
    FgResRef shadow_depth_tex_, shadow_color_tex_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferROHandle vtx_buf1, Ren::BufferROHandle vtx_buf2,
                  Ren::BufferROHandle ndx_buf, const Ren::WeakImgRef &shadow_depth_tex,
                  const Ren::WeakImgRef &shadow_color_tex);
    void DrawShadowMaps(const FgContext &fg);

  public:
    ExShadowColor(const int w, const int h, const DrawList **p_list, const FgBufROHandle vtx_buf1,
                  const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf, const FgBufROHandle materials_buf,
                  const BindlessTextureData *bindless_tex, const FgBufROHandle instances_buf,
                  const FgBufROHandle instance_indices_buf, const FgBufROHandle shared_data_buf,
                  const FgResRef noise_tex, const FgResRef shadow_depth_tex, const FgResRef shadow_color_tex)
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

        shadow_depth_tex_ = shadow_depth_tex;
        shadow_color_tex_ = shadow_color_tex;
    }

    void Execute(const FgContext &fg) override;
};
} // namespace Eng