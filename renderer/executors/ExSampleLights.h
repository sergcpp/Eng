#pragma once

#include <Ren/Image.h>
#include <Ren/VertexInput.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

struct view_state_t;

namespace Eng {
class ExSampleLights final : public FgExecutor {
  public:
    struct Args {
        FgBufHandle shared_data;
        FgBufHandle random_seq;
        FgBufHandle lights_buf;
        FgBufHandle nodes_buf;

        FgBufHandle geo_data;
        FgBufHandle materials;
        FgBufHandle vtx_buf1;
        FgBufHandle ndx_buf;
        FgBufHandle tlas_buf;

        FgResRef albedo_tex;
        FgResRef depth_tex;
        FgResRef norm_tex;
        FgResRef spec_tex;

        Ren::IAccStructure *tlas = nullptr;

        struct {
            uint32_t root_node = 0xffffffff;
            FgBufHandle rt_blas_buf;
            FgBufHandle prim_ndx_buf;
            FgBufHandle mesh_instances_buf;
        } swrt;

        FgResRef out_diffuse_tex;
        FgResRef out_specular_tex;
    };

    ExSampleLights(const view_state_t *view_state, const BindlessTextureData *bindless_tex, const Args *args)
        : view_state_(view_state), bindless_tex_(bindless_tex), args_(args) {}

    void Execute(const FgContext &fg) override;

  private:
    bool initialized_ = false;

    // lazily initialized data
    Ren::PipelineHandle pi_sample_lights_;

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    const Args *args_ = nullptr;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh);

    void Execute_HWRT(const FgContext &fg);
    void Execute_SWRT(const FgContext &fg);
};
} // namespace Eng