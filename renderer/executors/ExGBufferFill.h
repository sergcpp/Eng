#pragma once

#include <Ren/Common.h>
#include <Ren/Framebuffer.h>

#include "../framegraph/FgNode.h"

namespace Eng {
struct BindlessTextureData;
struct DrawList;
struct view_state_t;
class ShaderLoader;

class ExGBufferFill final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data
    Ren::PipelineHandle pi_simple_[3];
    Ren::PipelineHandle pi_vegetation_[2];

    // temp data (valid only between Setup and Execute calls)
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
    FgBufROHandle cells_buf_;
    FgBufROHandle items_buf_;
    FgBufROHandle lights_buf_;
    FgBufROHandle decals_buf_;
    FgImgROHandle noise_tex_;
    FgImgROHandle dummy_white_;
    FgImgROHandle dummy_black_;

    FgImgRWHandle out_albedo_tex_;
    FgImgRWHandle out_normal_tex_;
    FgImgRWHandle out_spec_tex_;
    FgImgRWHandle out_depth_tex_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle albedo_tex, Ren::ImageRWHandle normal_tex,
                  Ren::ImageRWHandle spec_tex, Ren::ImageRWHandle depth_tex);
    void DrawOpaque(const FgContext &fg, Ren::ImageRWHandle albedo_tex, Ren::ImageRWHandle normal_tex,
                    Ren::ImageRWHandle spec_tex, Ren::ImageRWHandle depth_tex);

  public:
    ExGBufferFill(const DrawList **p_list, const view_state_t *view_state, const FgBufROHandle vtx_buf1,
                  const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf, const FgBufROHandle materials_buf,
                  const BindlessTextureData *bindless_tex, const FgImgROHandle noise_tex,
                  const FgImgROHandle dummy_white, const FgImgROHandle dummy_black, const FgBufROHandle instances_buf,
                  const FgBufROHandle instance_indices_buf, const FgBufROHandle shared_data_buf,
                  const FgBufROHandle cells_buf, const FgBufROHandle items_buf, const FgBufROHandle decals_buf,
                  const FgImgRWHandle out_albedo, const FgImgRWHandle out_normals, const FgImgRWHandle out_spec,
                  const FgImgRWHandle out_depth) {
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