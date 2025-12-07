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
    FgBufROHandle instances_;
    FgBufROHandle instance_indices_;
    FgBufROHandle shared_data_;
    FgBufROHandle materials_;
    FgBufROHandle cells_;
    FgBufROHandle items_;
    FgBufROHandle lights_;
    FgBufROHandle decals_;
    FgImgROHandle noise_;
    FgImgROHandle dummy_white_;
    FgImgROHandle dummy_black_;

    FgImgRWHandle out_albedo_;
    FgImgRWHandle out_normal_;
    FgImgRWHandle out_spec_;
    FgImgRWHandle out_depth_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle albedo, Ren::ImageRWHandle normal,
                  Ren::ImageRWHandle spec, Ren::ImageRWHandle depth);
    void DrawOpaque(const FgContext &fg, Ren::ImageRWHandle albedo, Ren::ImageRWHandle normal, Ren::ImageRWHandle spec,
                    Ren::ImageRWHandle depth);

  public:
    ExGBufferFill(const DrawList **p_list, const view_state_t *view_state, const FgBufROHandle vtx_buf1,
                  const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf, const FgBufROHandle materials,
                  const BindlessTextureData *bindless_tex, const FgImgROHandle noise, const FgImgROHandle dummy_white,
                  const FgImgROHandle dummy_black, const FgBufROHandle instances, const FgBufROHandle instance_indices,
                  const FgBufROHandle shared_data, const FgBufROHandle cells, const FgBufROHandle items,
                  const FgBufROHandle decals, const FgImgRWHandle out_albedo, const FgImgRWHandle out_normals,
                  const FgImgRWHandle out_spec, const FgImgRWHandle out_depth) {
        view_state_ = view_state;
        bindless_tex_ = bindless_tex;

        p_list_ = p_list;

        vtx_buf1_ = vtx_buf1;
        vtx_buf2_ = vtx_buf2;
        ndx_buf_ = ndx_buf;
        instances_ = instances;
        instance_indices_ = instance_indices;
        shared_data_ = shared_data;
        cells_ = cells;
        items_ = items;
        decals_ = decals;
        materials_ = materials;

        noise_ = noise;
        dummy_white_ = dummy_white;
        dummy_black_ = dummy_black;

        out_albedo_ = out_albedo;
        out_normal_ = out_normals;
        out_spec_ = out_spec;
        out_depth_ = out_depth;
    }

    void Execute(const FgContext &fg) override;
};
} // namespace Eng