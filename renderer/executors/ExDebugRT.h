#pragma once

#include <Ren/Image.h>
#include <Ren/VertexInput.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

struct view_state_t;

namespace Eng {
class PrimDraw;
class ExDebugRT final : public FgExecutor {
  public:
    struct Args {
        FgBufROHandle shared_data;
        FgBufROHandle geo_data_buf;
        FgBufROHandle materials_buf;
        FgBufROHandle vtx_buf1;
        FgBufROHandle vtx_buf2;
        FgBufROHandle ndx_buf;
        FgResRef env_tex;
        FgBufROHandle lights_buf;
        FgResRef shadow_depth_tex, shadow_color_tex;
        FgResRef ltc_luts_tex;
        FgBufROHandle cells_buf;
        FgBufROHandle items_buf;
        FgBufROHandle tlas_buf;

        FgResRef irradiance_tex;
        FgResRef distance_tex;
        FgResRef offset_tex;

        const Ren::IAccStructure *tlas = nullptr;
        uint32_t cull_mask = 0xffffffff;

        struct {
            uint32_t root_node = 0xffffffff;
            FgBufROHandle rt_blas_buf;
            FgBufROHandle prim_ndx_buf;
            FgBufROHandle mesh_instances_buf;
        } swrt;

        FgResRef output_tex;
    };

    ExDebugRT(ShaderLoader &sh, const view_state_t *view_state, const BindlessTextureData *bindless_tex, const Args *args);

    void Execute(const FgContext &fg) override;

  private:
    Ren::PipelineHandle pi_debug_;

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;
    int depth_w_ = 0, depth_h_ = 0;

    const Args *args_ = nullptr;

    void Execute_HWRT(const FgContext &fg);
    void Execute_SWRT(const FgContext &fg);
};
} // namespace Eng