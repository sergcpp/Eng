#pragma once

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

#include <Ren/VertexInput.h>

namespace Eng {
class ExGBufferFill final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data
    Ren::PipelineHandle pi_simple_[3];
    Ren::PipelineHandle pi_vegetation_[2];

    Ren::Framebuffer main_draw_fb_[Ren::MaxFramesInFlight][2];
    int fb_to_use_ = 0;

    // temp data (valid only between Setup and Execute calls)
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
    FgBufHandle cells_buf_;
    FgBufHandle items_buf_;
    FgBufHandle lights_buf_;
    FgBufHandle decals_buf_;
    FgResRef noise_tex_;
    FgResRef dummy_white_;
    FgResRef dummy_black_;

    FgResRef out_albedo_tex_;
    FgResRef out_normal_tex_;
    FgResRef out_spec_tex_;
    FgResRef out_depth_tex_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferHandle vtx_buf1, Ren::BufferHandle vtx_buf2,
                  Ren::BufferHandle ndx_buf, const Ren::WeakImgRef &albedo_tex, const Ren::WeakImgRef &normal_tex,
                  const Ren::WeakImgRef &spec_tex, const Ren::WeakImgRef &depth_tex);
    void DrawOpaque(const FgContext &fg);

  public:
    ExGBufferFill(const DrawList **p_list, const view_state_t *view_state, const FgBufHandle vtx_buf1,
                  const FgBufHandle vtx_buf2, const FgBufHandle ndx_buf, const FgBufHandle materials_buf,
                  const BindlessTextureData *bindless_tex, const FgResRef noise_tex, const FgResRef dummy_white,
                  const FgResRef dummy_black, const FgBufHandle instances_buf, const FgBufHandle instance_indices_buf,
                  const FgBufHandle shared_data_buf, const FgBufHandle cells_buf, const FgBufHandle items_buf,
                  const FgBufHandle decals_buf, const FgResRef out_albedo, const FgResRef out_normals,
                  const FgResRef out_spec, const FgResRef out_depth) {
        view_state_ = view_state;
        bindless_tex_ = bindless_tex;

        p_list_ = p_list;

        vtx_buf1_ = vtx_buf1;
        vtx_buf2_ = vtx_buf2;
        ndx_buf_ = ndx_buf;
        instances_buf_ = instances_buf;
        instance_indices_buf_ = instance_indices_buf;
        shared_data_buf_ = shared_data_buf;
        cells_buf_ = cells_buf;
        items_buf_ = items_buf;
        decals_buf_ = decals_buf;
        materials_buf_ = materials_buf;

        noise_tex_ = noise_tex;
        dummy_white_ = dummy_white;
        dummy_black_ = dummy_black;

        out_albedo_tex_ = out_albedo;
        out_normal_tex_ = out_normals;
        out_spec_tex_ = out_spec;
        out_depth_tex_ = out_depth;
    }

    void Execute(const FgContext &fg) override;
};
} // namespace Eng