#pragma once

#include <Ren/Common.h>
#include <Ren/Framebuffer.h>

#include "../framegraph/FgNode.h"

namespace Eng {
struct DrawList;
class ShaderLoader;
struct BindlessTextureData;
struct view_state_t;

class ExDepthFill final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;
    bool clear_depth_ = false;

    const DrawList **p_list_;

    FgBufROHandle vtx_buf1_;
    FgBufROHandle vtx_buf2_;
    FgBufROHandle ndx_buf_;
    FgBufROHandle instances_buf_;
    FgBufROHandle instance_indices_buf_;
    FgBufROHandle shared_data_buf_;
    FgBufROHandle materials_buf_;
    FgImgROHandle noise_tex_;

    FgImgRWHandle depth_tex_;
    FgImgRWHandle velocity_tex_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle depth_tex, Ren::ImageRWHandle velocity_tex);
    void DrawDepth(const FgContext &fg, Ren::ImageRWHandle depth_tex, Ren::ImageRWHandle velocity_tex);

    Ren::RenderPassHandle rp_depth_only_[2], rp_depth_velocity_[2];

    Ren::PipelineHandle pi_static_solid_[3], pi_static_transp_[3];
    Ren::PipelineHandle pi_moving_solid_[3], pi_moving_transp_[3];
    Ren::PipelineHandle pi_vege_static_solid_[2], pi_vege_static_transp_[2];
    Ren::PipelineHandle pi_vege_moving_solid_[2], pi_vege_moving_transp_[2];
    Ren::PipelineHandle pi_skin_static_solid_[2], pi_skin_static_transp_[2];
    Ren::PipelineHandle pi_skin_moving_solid_[2], pi_skin_moving_transp_[2];

  public:
    ExDepthFill(const DrawList **list, const view_state_t *view_state, bool clear_depth, const FgBufROHandle vtx_buf1,
                const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf, const FgBufROHandle materials_buf,
                const BindlessTextureData *bindless_tex, const FgBufROHandle instances_buf,
                const FgBufROHandle instance_indices_buf, const FgBufROHandle shared_data_buf,
                const FgImgROHandle noise_tex, const FgImgRWHandle depth_tex, const FgImgRWHandle velocity_tex) {
        view_state_ = view_state;
        bindless_tex_ = bindless_tex;
        clear_depth_ = clear_depth;

        p_list_ = list;

        vtx_buf1_ = vtx_buf1;
        vtx_buf2_ = vtx_buf2;
        ndx_buf_ = ndx_buf;
        instances_buf_ = instances_buf;
        instance_indices_buf_ = instance_indices_buf;
        shared_data_buf_ = shared_data_buf;
        materials_buf_ = materials_buf;

        noise_tex_ = noise_tex;

        depth_tex_ = depth_tex;
        velocity_tex_ = velocity_tex;
    }

    void Execute(const FgContext &fg) override;
};
} // namespace Eng