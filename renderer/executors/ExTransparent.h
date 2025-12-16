#pragma once

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

#include <Ren/VertexInput.h>

namespace Eng {
class PrimDraw;
class ExTransparent final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data
    Ren::VertexInputHandle draw_pass_vi_;
    Ren::RenderPassHandle rp_transparent_;
    Ren::Framebuffer transparent_draw_fb_[Ren::MaxFramesInFlight][2], color_only_fb_[2], resolved_fb_, moments_fb_;
    int fb_to_use_ = 0;
#if defined(REN_VK_BACKEND)
    VkDescriptorSetLayout descr_set_layout_ = VK_NULL_HANDLE;
#endif

    // temp data (valid only between Setup and Execute calls)
    const Ren::ApiContext &api_;
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    const DrawList **p_list_ = nullptr;

    FgBufHandle vtx_buf1_;
    FgBufHandle vtx_buf2_;
    FgBufHandle ndx_buf_;
    FgBufHandle instances_buf_;
    FgBufHandle instance_indices_buf_;
    FgBufHandle shared_data_buf_;
    FgBufHandle cells_buf_;
    FgBufHandle items_buf_;
    FgBufHandle lights_buf_;
    FgBufHandle decals_buf_;
    FgBufHandle materials_buf_;
    FgResRef lm_tex_[4];
    FgResRef brdf_lut_;
    FgResRef noise_tex_;
    FgResRef cone_rt_lut_;
    FgResRef dummy_black_;

    FgResRef shad_tex_;
    FgResRef ssao_tex_;

    FgResRef color_tex_;
    FgResRef normal_tex_;
    FgResRef spec_tex_;
    FgResRef depth_tex_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh, Ren::BufferHandle vtx_buf1, Ren::BufferHandle vtx_buf2,
                  Ren::BufferHandle ndx_buf, const Ren::WeakImgRef &color_tex, const Ren::WeakImgRef &normal_tex,
                  const Ren::WeakImgRef &spec_tex, const Ren::WeakImgRef &depth_tex);
    void DrawTransparent(const FgContext &fg, const Ren::WeakImgRef &color_tex);

    void DrawTransparent_Simple(const FgContext &fg, Ren::BufferHandle instances_buf,
                                Ren::BufferHandle instance_indices_buf, Ren::BufferHandle unif_shared_data_buf,
                                Ren::BufferHandle materials_buf, Ren::BufferHandle cells_buf,
                                Ren::BufferHandle items_buf, Ren::BufferHandle lights_buf, Ren::BufferHandle decals_buf,
                                const Ren::Image &shad_tex, const Ren::WeakImgRef &color_tex,
                                const Ren::Image &ssao_tex);
    void DrawTransparent_OIT_MomentBased(const FgContext &fg);
    void DrawTransparent_OIT_WeightedBlended(const FgContext &fg);

#if defined(REN_VK_BACKEND)
    void InitDescrSetLayout();
#endif

  public:
    ExTransparent(const Ren::ApiContext &api, const DrawList **p_list, const view_state_t *view_state,
                  const FgBufHandle vtx_buf1, const FgBufHandle vtx_buf2, const FgBufHandle ndx_buf,
                  const FgBufHandle materials_buf, const BindlessTextureData *bindless_tex, const FgResRef brdf_lut,
                  const FgResRef noise_tex, const FgResRef cone_rt_lut, const FgResRef dummy_black,
                  const FgBufHandle instances_buf, const FgBufHandle instance_indices_buf,
                  const FgBufHandle shared_data_buf, const FgBufHandle cells_buf, const FgBufHandle items_buf,
                  const FgBufHandle lights_buf, const FgBufHandle decals_buf, const FgResRef shad_tex,
                  const FgResRef ssao_tex, const FgResRef lm_tex[4], const FgResRef color_tex,
                  const FgResRef normal_tex, const FgResRef spec_tex, const FgResRef depth_tex)
        : api_(api) {
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
        lights_buf_ = lights_buf;
        decals_buf_ = decals_buf;
        shad_tex_ = shad_tex;
        ssao_tex_ = ssao_tex;
        materials_buf_ = materials_buf;

        for (int i = 0; i < 4; ++i) {
            lm_tex_[i] = lm_tex[i];
        }

        brdf_lut_ = brdf_lut;
        noise_tex_ = noise_tex;
        cone_rt_lut_ = cone_rt_lut;

        dummy_black_ = dummy_black;

        color_tex_ = color_tex;
        normal_tex_ = normal_tex;
        spec_tex_ = spec_tex;
        depth_tex_ = depth_tex;
    }
    ~ExTransparent() final;

    void Execute(const FgContext &fg) override;
};
} // namespace Eng