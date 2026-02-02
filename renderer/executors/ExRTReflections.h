#pragma once

#include <Ren/Image.h>
#include <Ren/VertexInput.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

struct view_state_t;

namespace Eng {
class PrimDraw;
class ExRTReflections final : public FgExecutor {
  public:
    struct Args {
        FgResRef noise_tex;
        FgBufROHandle geo_data;
        FgBufROHandle materials;
        FgBufROHandle vtx_buf1;
        FgBufROHandle vtx_buf2;
        FgBufROHandle ndx_buf;
        FgBufROHandle shared_data;
        FgResRef depth_tex;
        FgResRef normal_tex;
        FgResRef env_tex;
        FgBufROHandle lights_buf;
        FgResRef shadow_depth_tex, shadow_color_tex;
        FgResRef ltc_luts_tex;
        FgBufROHandle cells_buf;
        FgBufROHandle items_buf;
        FgBufROHandle ray_counter;
        FgBufROHandle ray_list;
        FgBufROHandle indir_args;
        FgBufROHandle tlas_buf;

        FgResRef irradiance_tex;
        FgResRef distance_tex;
        FgResRef offset_tex;

        FgBufROHandle stoch_lights_buf;
        FgBufROHandle light_nodes_buf;

        FgBufROHandle oit_depth_buf;

        const Ren::IAccStructure *tlas = nullptr;
        const probe_volume_t *probe_volume = nullptr;

        struct {
            uint32_t root_node = 0xffffffff;
            FgBufROHandle rt_blas_buf;
            FgBufROHandle prim_ndx_buf;
            FgBufROHandle mesh_instances_buf;
        } swrt;

        bool layered = false;
        bool four_bounces = false;

        FgResRef out_refl_tex[OIT_REFLECTION_LAYERS];
    };

    explicit ExRTReflections(const view_state_t *view_state, const BindlessTextureData *bindless_tex, const Args *args,
                             bool use_rt_pipeline)
        : use_rt_pipeline_(use_rt_pipeline), view_state_(view_state), bindless_tex_(bindless_tex), args_(args) {}

    void Execute(const FgContext &fg) override;

  private:
    bool initialized_ = false;
    bool use_rt_pipeline_ = false;

    // lazily initialized data
    Ren::PipelineHandle pi_rt_reflections_[2];

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    const Args *args_ = nullptr;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh);

    void Execute_HWRT(const FgContext &fg);
    void Execute_SWRT(const FgContext &fg);
};
} // namespace Eng