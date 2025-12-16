#pragma once

#include <Ren/Image.h>
#include <Ren/VertexInput.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

struct view_state_t;

namespace Eng {
class ExRTGI final : public FgExecutor {
  public:
    struct Args {
        FgResRef noise_tex;
        FgBufHandle geo_data;
        FgBufHandle materials;
        FgBufHandle vtx_buf1;
        FgBufHandle ndx_buf;
        FgBufHandle shared_data;
        FgResRef depth_tex;
        FgResRef normal_tex;
        FgBufHandle ray_counter;
        FgBufHandle ray_list;
        FgBufHandle indir_args;
        FgBufHandle tlas_buf; // fake read for now

        Ren::IAccStructure *tlas = nullptr;

        struct {
            uint32_t root_node = 0xffffff;
            FgBufHandle rt_blas_buf;
            FgBufHandle prim_ndx_buf;
            FgBufHandle mesh_instances_buf;
        } swrt;

        bool second_bounce = false;

        FgBufHandle out_ray_hits_buf;
    };

    ExRTGI(const view_state_t *view_state, const BindlessTextureData *bindless_tex, const Args *args)
        : view_state_(view_state), bindless_tex_(bindless_tex), args_(args) {}

    void Execute(const FgContext &fg) override;

  private:
    bool initialized_ = false;

    // lazily initialized data
    Ren::PipelineHandle pi_rt_gi_;

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    const Args *args_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh);

    void Execute_HWRT(const FgContext &fg);
    void Execute_SWRT(const FgContext &fg);
};
} // namespace Eng