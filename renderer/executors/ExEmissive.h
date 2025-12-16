#pragma once

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

#include <Ren/VertexInput.h>

namespace Eng {
class ExEmissive final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data
    Ren::PipelineHandle pi_simple_[3];
    Ren::PipelineHandle pi_vegetation_[2];

    Ren::Framebuffer main_draw_fb_[Ren::MaxFramesInFlight][2];
    int fb_to_use_ = 0;

    // temp data
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    const Ren::ImageAtlas *decals_atlas_ = nullptr;

    const DrawList **p_list_ = nullptr;

    FgBufHandle vtx_buf1_;
    FgBufHandle vtx_buf2_;
    FgBufHandle ndx_buf_;
    FgBufHandle instances_buf_;
    FgBufHandle instance_indices_buf_;
    FgBufHandle shared_data_buf_;
    FgBufHandle materials_buf_;
    FgResRef noise_tex_;
    FgResRef dummy_white_;

    FgResRef out_color_tex_;
    FgResRef out_depth_tex_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferHandle vtx_buf1, Ren::BufferHandle vtx_buf2,
                  Ren::BufferHandle ndx_buf, const Ren::WeakImgRef &color_tex, const Ren::WeakImgRef &depth_tex);
    void DrawOpaque(const FgContext &fg);

  public:
    ExEmissive(const DrawList **p_list, const view_state_t *view_state, const FgBufHandle vtx_buf1,
               const FgBufHandle vtx_buf2, const FgBufHandle ndx_buf, const FgBufHandle materials_buf,
               const BindlessTextureData *bindless_tex, const FgResRef noise_tex, const FgResRef dummy_white,
               const FgBufHandle instances_buf, const FgBufHandle instance_indices_buf,
               const FgBufHandle shared_data_buf, const FgResRef out_color, const FgResRef out_depth) {
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