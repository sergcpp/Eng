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
    FgBufROHandle instances_;
    FgBufROHandle instance_indices_;
    FgBufROHandle shared_data_;
    FgBufROHandle cells_;
    FgBufROHandle items_;
    FgBufROHandle lights_;
    FgBufROHandle decals_;
    FgBufROHandle materials_;
    FgResRef lm_tex_[4];
    FgImgROHandle brdf_lut_;
    FgImgROHandle noise_;
    FgImgROHandle cone_rt_lut_;
    FgImgROHandle dummy_black_;

    FgImgROHandle shadow_depth_;
    FgImgROHandle ssao_;

    FgImgRWHandle color_;
    FgImgRWHandle normal_;
    FgImgRWHandle spec_;
    FgImgRWHandle depth_;

    void LazyInit(Ren::Context &ctx, ShaderLoader &sh, Ren::ImageRWHandle color, Ren::ImageRWHandle normal,
                  Ren::ImageRWHandle spec, Ren::ImageRWHandle depth);
    void DrawTransparent(const FgContext &fg, Ren::ImageRWHandle color, Ren::ImageRWHandle normal,
                         Ren::ImageRWHandle spec, Ren::ImageRWHandle depth);

    void DrawTransparent_Simple(const FgContext &fg, Ren::BufferROHandle instances,
                                Ren::BufferROHandle instance_indices, Ren::BufferROHandle unif_shared_data,
                                Ren::BufferROHandle materials, Ren::BufferROHandle cells, Ren::BufferROHandle items,
                                Ren::BufferROHandle lights, Ren::BufferROHandle decals, Ren::ImageROHandle shad,
                                Ren::ImageRWHandle color, Ren::ImageRWHandle normal, Ren::ImageRWHandle spec,
                                Ren::ImageRWHandle depth, Ren::ImageROHandle ssao);
    void DrawTransparent_OIT_MomentBased(const FgContext &fg);
    void DrawTransparent_OIT_WeightedBlended(const FgContext &fg);

#if defined(REN_VK_BACKEND)
    void InitDescrSetLayout();
#endif

  public:
    ExTransparent(const Ren::ApiContext &api, const DrawList **p_list, const view_state_t *view_state,
                  const FgBufROHandle vtx_buf1, const FgBufROHandle vtx_buf2, const FgBufROHandle ndx_buf,
                  const FgBufROHandle materials, const BindlessTextureData *bindless_tex, const FgImgROHandle brdf_lut,
                  const FgImgROHandle noise, const FgImgROHandle cone_rt_lut, const FgImgROHandle dummy_black,
                  const FgBufROHandle instances, const FgBufROHandle instance_indices, const FgBufROHandle shared_data,
                  const FgBufROHandle cells, const FgBufROHandle items, const FgBufROHandle lights,
                  const FgBufROHandle decals, const FgImgROHandle shadow_depth, const FgImgROHandle ssao,
                  const FgResRef lm_tex[4], const FgImgRWHandle color, const FgImgRWHandle normal,
                  const FgImgRWHandle spec, const FgImgRWHandle depth)
        : api_(api) {
        view_state_ = view_state;
        bindless_tex_ = bindless_tex;

        p_list_ = p_list;

        vtx_buf1_ = vtx_buf1;
        vtx_buf2_ = vtx_buf2;
        ndx_buf_ = ndx_buf;
        instances_ = instances;
        instance_indices_ = instance_indices;
        shared_data_ = shared_data;
        cells_ = cells;
        items_ = items;
        lights_ = lights;
        decals_ = decals;
        shadow_depth_ = shadow_depth;
        ssao_ = ssao;
        materials_ = materials;

        for (int i = 0; i < 4; ++i) {
            lm_tex_[i] = lm_tex[i];
        }

        brdf_lut_ = brdf_lut;
        noise_ = noise;
        cone_rt_lut_ = cone_rt_lut;

        dummy_black_ = dummy_black;

        color_ = color;
        normal_ = normal;
        spec_ = spec;
        depth_ = depth;
    }
    ~ExTransparent() final;

    void Execute(const FgContext &fg) override;
};
} // namespace Eng