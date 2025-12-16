#pragma once

#include <Ren/Image.h>
#include <Ren/VertexInput.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

struct view_state_t;

namespace Eng {
class ExVolVoxelize final : public FgExecutor {
  public:
    struct Args {
        FgBufHandle shared_data;
        FgResRef stbn_tex;
        FgBufHandle geo_data;
        FgBufHandle materials;
        FgBufHandle tlas_buf;

        Ren::IAccStructure *tlas = nullptr;

        struct {
            uint32_t root_node = 0xffffffff;
            FgBufHandle rt_blas_buf;
            FgBufHandle prim_ndx_buf;
            FgBufHandle mesh_instances_buf;
            FgBufHandle vtx_buf1;
            FgBufHandle ndx_buf;
        } swrt;

        FgResRef out_emission_tex;
        FgResRef out_scatter_tex;
    };

    ExVolVoxelize(const DrawList **p_list, const view_state_t *view_state, const Args *args)
        : p_list_(p_list), view_state_(view_state), args_(args) {}

    void Execute(const FgContext &fg) override;

  private:
    bool initialized_ = false;

    // lazily initialized data
    Ren::PipelineHandle pi_vol_voxelize_;

    // temp data (valid only between Setup and Execute calls)
    const DrawList **p_list_ = nullptr;
    const view_state_t *view_state_ = nullptr;
    const Args *args_ = nullptr;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh);

    void Execute_HWRT(const FgContext &fg);
    void Execute_SWRT(const FgContext &fg);
};
} // namespace Eng