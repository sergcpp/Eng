#pragma once

#include <Ren/Common.h>
#include <Ren/Framebuffer.h>

#include "../framegraph/FgNode.h"

namespace Eng {
struct BindlessTextureData;
struct DrawList;
struct view_state_t;
class ShaderLoader;

class ExOpaque final : public FgExecutor {
    bool initialized = false;

    // lazily initialized data
    Ren::VertexInputHandle draw_pass_vi_;
    Ren::RenderPassHandle rp_opaque_;

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
    FgBufROHandle materials_buf_;
    FgBufROHandle cells_buf_;
    FgBufROHandle items_buf_;
    FgBufROHandle lights_buf_;
    FgBufROHandle decals_buf_;
    FgImgROHandle shadow_depth_;
    FgResRef lm_tex_[4];
    FgImgROHandle ssao_tex_;
    FgImgROHandle brdf_lut_;
    FgImgROHandle noise_tex_;
    FgImgROHandle cone_rt_lut_;
    FgImgROHandle dummy_black_;

    FgImgRWHandle color_tex_;
    FgImgRWHandle normal_tex_;
    FgImgRWHandle spec_tex_;
    FgImgRWHandle depth_tex_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle color_tex, Ren::ImageRWHandle normal_tex,
                  Ren::ImageRWHandle spec_tex, Ren::ImageRWHandle depth_tex);
    void DrawOpaque(const FgContext &fg, Ren::ImageRWHandle color_tex, Ren::ImageRWHandle normal_tex,
                    Ren::ImageRWHandle spec_tex, Ren::ImageRWHandle depth_tex);

#if defined(REN_VK_BACKEND)
    void InitDescrSetLayout();
#endif
  public:
    ExOpaque(Ren::ApiContext &api, const DrawList **p_list, const view_state_t *view_state,
             const FgBufROHandle vtx_buf1, const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf,
             const FgBufROHandle materials_buf, const BindlessTextureData *bindless_tex, const FgImgROHandle brdf_lut,
             const FgImgROHandle noise_tex, const FgImgROHandle cone_rt_lut, const FgImgROHandle dummy_black,
             const FgBufROHandle instances_buf, const FgBufROHandle instance_indices_buf,
             const FgBufROHandle shared_data_buf, const FgBufROHandle cells_buf, const FgBufROHandle items_buf,
             const FgBufROHandle lights_buf, const FgBufROHandle decals_buf, const FgImgROHandle shadow_depth,
             const FgImgROHandle ssao_tex, const FgResRef lm_tex[], const FgImgRWHandle out_color,
             const FgImgRWHandle out_normals, const FgImgRWHandle out_spec, const FgImgRWHandle out_depth)
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

        materials_buf_ = materials_buf;
        ssao_tex_ = ssao_tex;

        for (int i = 0; i < 4; ++i) {
            lm_tex_[i] = lm_tex[i];
        }

        brdf_lut_ = brdf_lut;
        noise_tex_ = noise_tex;
        cone_rt_lut_ = cone_rt_lut;

        dummy_black_ = dummy_black;

        color_tex_ = out_color;
        normal_tex_ = out_normals;
        spec_tex_ = out_spec;
        depth_tex_ = out_depth;
    }
    ~ExOpaque() final;

    void Execute(const FgContext &fg) override;
};
} // namespace Eng