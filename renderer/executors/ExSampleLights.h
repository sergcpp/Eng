#pragma once

#include "../framegraph/FgNode.h"

namespace Eng {
struct BindlessTextureData;
class ShaderLoader;
struct view_state_t;

class ExSampleLights final : public FgExecutor {
  public:
    struct Args {
        FgBufROHandle shared_data;
        FgBufROHandle random_seq;
        FgBufROHandle lights_buf;
        FgBufROHandle nodes_buf;

        FgBufROHandle geo_data;
        FgBufROHandle materials;
        FgBufROHandle vtx_buf1;
        FgBufROHandle ndx_buf;
        FgBufROHandle tlas_buf;

        FgImgROHandle albedo_tex;
        FgImgROHandle depth_tex;
        FgImgROHandle norm_tex;
        FgImgROHandle spec_tex;

        Ren::IAccStructure *tlas = nullptr;

        struct {
            uint32_t root_node = 0xffffffff;
            FgBufROHandle rt_blas_buf;
            FgBufROHandle prim_ndx_buf;
            FgBufROHandle mesh_instances_buf;
        } swrt;

        FgImgRWHandle out_diffuse_tex;
        FgImgRWHandle out_specular_tex;
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

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh);

    void Execute_HWRT(const FgContext &fg);
    void Execute_SWRT(const FgContext &fg);
};
} // namespace Eng