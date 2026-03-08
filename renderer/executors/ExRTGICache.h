#pragma once

#include "../framegraph/FgNode.h"

namespace Eng {
struct BindlessTextureData;
struct view_state_t;
struct probe_volume_t;
class ShaderLoader;

class ExRTGICache final : public FgExecutor {
  public:
    struct Args {
        FgBufROHandle geo_data;
        FgBufROHandle materials;
        FgBufROHandle vtx_buf1;
        FgBufROHandle ndx_buf;
        FgBufROHandle shared_data;
        FgImgROHandle env_tex;
        FgBufROHandle lights_buf;
        FgImgROHandle shadow_depth, shadow_color;
        FgImgROHandle ltc_luts;
        FgBufROHandle cells_buf;
        FgBufROHandle items_buf;
        FgBufROHandle tlas_buf; // fake read for now

        Ren::AccStructROHandle tlas;

        struct {
            uint32_t root_node = 0xffffffff;
            FgBufROHandle rt_blas_buf;
            FgBufROHandle prim_ndx_buf;
            FgBufROHandle mesh_instances_buf;
        } swrt;

        FgImgROHandle irradiance_tex;
        FgImgROHandle distance_tex;
        FgImgROHandle offset_tex;

        FgBufROHandle random_seq;
        FgBufROHandle stoch_lights_buf;
        FgBufROHandle light_nodes_buf;

        FgImgRWHandle out_ray_data_tex;

        const view_state_t *view_state = nullptr;
        bool partial_update = false;
        Ren::Span<const probe_volume_t> probe_volumes;
    };

    ExRTGICache(const view_state_t *view_state, const BindlessTextureData *bindless_tex, const Args *args)
        : view_state_(view_state), bindless_tex_(bindless_tex), args_(args) {}

    void Execute(const FgContext &fg) override;

  private:
    bool initialized_ = false;

    // lazily initialized data
    Ren::PipelineHandle pi_rt_gi_cache_[2][2];

    // temp data (valid only between Setup and Execute calls)
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    const Args *args_ = nullptr;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh);

    void Execute_HWRT(const FgContext &fg);
    void Execute_SWRT(const FgContext &fg);
};
} // namespace Eng