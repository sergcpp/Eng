#pragma once

#include <Ren/Common.h>
#include <Ren/Framebuffer.h>

#include "../framegraph/FgNode.h"

namespace Eng {
struct BindlessTextureData;
struct DrawList;
class PrimDraw;
class ShaderLoader;
struct view_state_t;

class ExTransparent final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data
    Ren::VertexInputHandle draw_pass_vi_;
    Ren::RenderPassHandle rp_transparent_;

#if defined(REN_VK_BACKEND)
    VkDescriptorSetLayout descr_set_layout_ = VK_NULL_HANDLE;
#endif

    // temp data (valid only between Setup and Execute calls)
    const Ren::ApiContext &api_;
    const view_state_t *view_state_ = nullptr;
    const BindlessTextureData *bindless_tex_ = nullptr;

    const DrawList **p_list_ = nullptr;

    FgBufROHandle vtx_buf1_;
    FgBufROHandle vtx_buf2_;
    FgBufROHandle ndx_buf_;
    FgBufROHandle instances_buf_;
    FgBufROHandle instance_indices_buf_;
    FgBufROHandle shared_data_buf_;
    FgBufROHandle cells_buf_;
    FgBufROHandle items_buf_;
    FgBufROHandle lights_buf_;
    FgBufROHandle decals_buf_;
    FgBufROHandle materials_buf_;
    FgResRef lm_tex_[4];
    FgImgROHandle brdf_lut_;
    FgImgROHandle noise_tex_;
    FgImgROHandle cone_rt_lut_;
    FgImgROHandle dummy_black_;

    FgImgROHandle shadow_depth_;
    FgImgROHandle ssao_tex_;

    FgImgRWHandle color_tex_;
    FgImgRWHandle normal_tex_;
    FgImgRWHandle spec_tex_;
    FgImgRWHandle depth_tex_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle color_tex, Ren::ImageRWHandle normal_tex,
                  Ren::ImageRWHandle spec_tex, Ren::ImageRWHandle depth_tex);
    void DrawTransparent(const FgContext &fg, Ren::ImageRWHandle color_tex, Ren::ImageRWHandle normal_tex,
                         Ren::ImageRWHandle spec_tex, Ren::ImageRWHandle depth_tex);

    void DrawTransparent_Simple(const FgContext &fg, Ren::BufferROHandle instances_buf,
                                Ren::BufferROHandle instance_indices_buf, Ren::BufferROHandle unif_shared_data_buf,
                                Ren::BufferROHandle materials_buf, Ren::BufferROHandle cells_buf,
                                Ren::BufferROHandle items_buf, Ren::BufferROHandle lights_buf,
                                Ren::BufferROHandle decals_buf, Ren::ImageROHandle shad_tex,
                                Ren::ImageRWHandle color_tex, Ren::ImageRWHandle normal_tex,
                                Ren::ImageRWHandle spec_tex, Ren::ImageRWHandle depth_tex, Ren::ImageROHandle ssao_tex);
    void DrawTransparent_OIT_MomentBased(const FgContext &fg);
    void DrawTransparent_OIT_WeightedBlended(const FgContext &fg);

#if defined(REN_VK_BACKEND)
    void InitDescrSetLayout();
#endif

  public:
    ExTransparent(const Ren::ApiContext &api, const DrawList **p_list, const view_state_t *view_state,
                  const FgBufROHandle vtx_buf1, const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf,
                  const FgBufROHandle materials_buf, const BindlessTextureData *bindless_tex,
                  const FgImgROHandle brdf_lut, const FgImgROHandle noise_tex, const FgImgROHandle cone_rt_lut,
                  const FgImgROHandle dummy_black, const FgBufROHandle instances_buf,
                  const FgBufROHandle instance_indices_buf, const FgBufROHandle shared_data_buf,
                  const FgBufROHandle cells_buf, const FgBufROHandle items_buf, const FgBufROHandle lights_buf,
                  const FgBufROHandle decals_buf, const FgImgROHandle shadow_depth, const FgImgROHandle ssao_tex,
                  const FgResRef lm_tex[4], const FgImgRWHandle color_tex, const FgImgRWHandle normal_tex,
                  const FgImgRWHandle spec_tex, const FgImgRWHandle depth_tex)
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
        shadow_depth_ = shadow_depth;
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