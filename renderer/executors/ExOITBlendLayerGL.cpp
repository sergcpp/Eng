#include "ExOITBlendLayer.h"

#include "../PrimDraw.h"
#include "../Renderer_Structs.h"

#include <Ren/Context.h>
#include <Ren/DebugMarker.h>
#include <Ren/GL.h>
#include <Ren/RastState.h>

#include "../shaders/blit_oit_depth_interface.h"
#include "../shaders/oit_blend_layer_interface.h"

namespace ExSharedInternal {
void _bind_textures_and_samplers(Ren::Context &ctx, const Ren::Material &mat,
                                 Ren::SmallVectorImpl<Ren::SamplerHandle> &temp_samplers);
uint32_t _draw_range_ext2(const Eng::FgContext &fg, const Ren::MaterialStorage &materials, const Ren::Image &white_tex,
                          Ren::Span<const uint32_t> batch_indices, Ren::Span<const Eng::basic_draw_batch_t> batches,
                          uint32_t i, uint64_t mask, uint32_t &cur_mat_id, int *draws_count);
} // namespace ExSharedInternal

void Eng::ExOITBlendLayer::DrawTransparent(const FgContext &fg, const Ren::WeakImgRef &depth_tex) {
    using namespace ExSharedInternal;

    const Ren::Image &noise_tex = fg.AccessROImage(noise_tex_);
    const Ren::Image &dummy_white = fg.AccessROImage(dummy_white_);
    const Ren::Image &shadow_map_tex = fg.AccessROImage(shadow_map_);
    const Ren::Image &ltc_luts_tex = fg.AccessROImage(ltc_luts_tex_);
    const Ren::Image &env_tex = fg.AccessROImage(env_tex_);
    const Ren::BufferROHandle instances_buf = fg.AccessROBuffer(instances_buf_);
    const Ren::BufferROHandle instance_indices_buf = fg.AccessROBuffer(instance_indices_buf_);
    const Ren::BufferROHandle unif_shared_data_buf = fg.AccessROBuffer(shared_data_buf_);
    const Ren::BufferROHandle materials_buf = fg.AccessROBuffer(materials_buf_);
    const Ren::BufferROHandle cells_buf = fg.AccessROBuffer(cells_buf_);
    const Ren::BufferROHandle items_buf = fg.AccessROBuffer(items_buf_);
    const Ren::BufferROHandle lights_buf = fg.AccessROBuffer(lights_buf_);
    const Ren::BufferROHandle decals_buf = fg.AccessROBuffer(decals_buf_);
    const Ren::BufferROHandle oit_depth_buf = fg.AccessROBuffer(oit_depth_buf_);

    const Ren::Image &back_color_tex = fg.AccessROImage(back_color_tex_);
    const Ren::Image &back_depth_tex = fg.AccessROImage(back_depth_tex_);

    const Ren::Image *irr_tex = nullptr, *dist_tex = nullptr, *off_tex = nullptr;
    if (irradiance_tex_) {
        irr_tex = &fg.AccessROImage(irradiance_tex_);
        dist_tex = &fg.AccessROImage(distance_tex_);
        off_tex = &fg.AccessROImage(offset_tex_);
    }

    const Ren::Image *specular_tex = nullptr;
    if (oit_specular_tex_) {
        specular_tex = &fg.AccessROImage(oit_specular_tex_);
    }

    if ((*p_list_)->alpha_blend_start_index == -1) {
        return;
    }

    { // blit depth layer
        Ren::RastState rast_state;
        rast_state.poly.cull = uint8_t(Ren::eCullFace::Back);
        rast_state.viewport[2] = view_state_->ren_res[0];
        rast_state.viewport[3] = view_state_->ren_res[1];

        rast_state.depth.test_enabled = true;
        rast_state.depth.compare_op = unsigned(Ren::eCompareOp::Greater);

        const Ren::Binding bindings[] = {{Ren::eBindTarget::UTBuf, BlitOITDepth::OIT_DEPTH_BUF_SLOT, oit_depth_buf}};

        BlitOITDepth::Params uniform_params = {};
        uniform_params.img_size = view_state_->ren_res;
        uniform_params.layer_index = depth_layer_index_;

        const Ren::RenderTarget depth_target = {depth_tex, Ren::eLoadOp::Load, Ren::eStoreOp::Store};

        prim_draw_.DrawPrim(PrimDraw::ePrim::Quad, prog_oit_blit_depth_, depth_target, {}, rast_state, fg.rast_state(),
                            bindings, &uniform_params, sizeof(uniform_params), 0);
    }

    Ren::RastState _rast_state;
    _rast_state.poly.cull = uint8_t(Ren::eCullFace::Front);

    if ((*p_list_)->render_settings.debug_wireframe) {
        _rast_state.poly.mode = uint8_t(Ren::ePolygonMode::Line);
    } else {
        _rast_state.poly.mode = uint8_t(Ren::ePolygonMode::Fill);
    }

    _rast_state.depth.test_enabled = true;
    _rast_state.depth.write_enabled = false;
    _rast_state.depth.compare_op = unsigned(Ren::eCompareOp::Greater);

    const Ren::StoragesRef &storages = fg.storages();

    // Bind main buffer for drawing
    glBindFramebuffer(GL_FRAMEBUFFER, main_draw_fb_[0][fb_to_use_].id());

    const Ren::BufferMain &materials_buf_main = storages.buffers.Get(materials_buf).first;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_MATERIALS_BUF, GLuint(materials_buf_main.buf));
    if (fg.ren_ctx().capabilities.bindless_texture) {
        const Ren::BufferMain &buf_main = storages.buffers.Get(bindless_tex_->rt_inline_textures.buf).first;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_BINDLESS_TEX, GLuint(buf_main.buf));
    }

    const Ren::BufferMain &unif_shared_data_buf_main = storages.buffers.Get(unif_shared_data_buf).first;
    glBindBufferBase(GL_UNIFORM_BUFFER, BIND_UB_SHARED_DATA_BUF, unif_shared_data_buf_main.buf);

    if ((*p_list_)->decals_atlas) {
        ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, BIND_DECAL_TEX, (*p_list_)->decals_atlas->tex_id(0));
    }

    ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, BIND_NOISE_TEX, noise_tex.id());

    const Ren::BufferMain &instances_buf_main = storages.buffers.Get(instances_buf).first;
    ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, BIND_INST_BUF, GLuint(instances_buf_main.views[0].second));
    const Ren::BufferMain &instance_indices_buf_main = storages.buffers.Get(instance_indices_buf).first;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_INST_NDX_BUF, GLuint(instance_indices_buf_main.buf));

    ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, OITBlendLayer::CELLS_BUF_SLOT,
                               storages.buffers.Get(cells_buf).first.views[0].second);
    ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, OITBlendLayer::ITEMS_BUF_SLOT,
                               storages.buffers.Get(items_buf).first.views[0].second);
    ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, OITBlendLayer::LIGHT_BUF_SLOT,
                               storages.buffers.Get(lights_buf).first.views[0].second);
    ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, OITBlendLayer::DECAL_BUF_SLOT,
                               storages.buffers.Get(decals_buf).first.views[0].second);

    ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, OITBlendLayer::SHADOW_TEX_SLOT, shadow_map_tex.id());
    ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, OITBlendLayer::LTC_LUTS_TEX_SLOT, ltc_luts_tex.id());
    ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, OITBlendLayer::ENV_TEX_SLOT, env_tex.id());

    ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, OITBlendLayer::BACK_COLOR_TEX_SLOT, back_color_tex.id());
    ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, OITBlendLayer::BACK_DEPTH_TEX_SLOT, back_depth_tex.id());

    if (irr_tex) {
        ren_glBindTextureUnit_Comp(GL_TEXTURE_2D_ARRAY, OITBlendLayer::IRRADIANCE_TEX_SLOT, irr_tex->id());
        ren_glBindTextureUnit_Comp(GL_TEXTURE_2D_ARRAY, OITBlendLayer::DISTANCE_TEX_SLOT, dist_tex->id());
        ren_glBindTextureUnit_Comp(GL_TEXTURE_2D_ARRAY, OITBlendLayer::OFFSET_TEX_SLOT, off_tex->id());
    }

    if (specular_tex) {
        ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, OITBlendLayer::SPEC_TEX_SLOT, specular_tex->id());
    }

    const Ren::Span<const basic_draw_batch_t> batches = {(*p_list_)->basic_batches};
    const Ren::Span<const uint32_t> batch_indices = {(*p_list_)->basic_batch_indices};
    const auto &materials = *(*p_list_)->materials;

    int draws_count = 0;
    uint32_t i = (*p_list_)->alpha_blend_start_index;
    uint32_t cur_mat_id = 0xffffffff;

    using BDB = basic_draw_batch_t;

    const Ren::ApiContext &api = fg.ren_ctx().api();

    { // Simple meshes
        Ren::DebugMarker _m(api, fg.cmd_buf(), "SIMPLE");

        const Ren::PipelineMain &pi_simple0_main = storages.pipelines.Get(pi_simple_[0]).first;
        const Ren::PipelineMain &pi_simple1_main = storages.pipelines.Get(pi_simple_[1]).first;
        const Ren::PipelineMain &pi_simple2_main = storages.pipelines.Get(pi_simple_[2]).first;

        const Ren::VertexInputMain &vi = storages.vtx_inputs.Get(pi_simple0_main.vtx_input).first;
        glBindVertexArray(VertexInput_GetVAO(vi, storages.buffers));
        glUseProgram(storages.programs.Get(pi_simple0_main.prog).first.id);

        { // solid one-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "SOLID-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white, batch_indices, batches, i, BDB::BitAlphaBlend, cur_mat_id,
                                 &draws_count);

            rast_state = pi_simple1_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white, batch_indices, batches, i,
                                 BDB::BitAlphaBlend | BDB::BitBackSided, cur_mat_id, &draws_count);
        }
        { // solid two-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "SOLID-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white, batch_indices, batches, i,
                                 BDB::BitAlphaBlend | BDB::BitTwoSided, cur_mat_id, &draws_count);
        }
        { // moving solid one-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "MOVING-SOLID-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white, batch_indices, batches, i,
                                 BDB::BitAlphaBlend | BDB::BitMoving, cur_mat_id, &draws_count);
        }
        { // moving solid two-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "MOVING-SOLID-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitTwoSided;
            i = _draw_range_ext2(fg, materials, dummy_white, batch_indices, batches, i, DrawMask, cur_mat_id,
                                 &draws_count);
        }
        { // alpha-tested one-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "ALPHA-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white, batch_indices, batches, i,
                                 BDB::BitAlphaBlend | BDB::BitAlphaTest, cur_mat_id, &draws_count);
        }
        { // alpha-tested two-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "ALPHA-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitAlphaTest | BDB::BitTwoSided;
            i = _draw_range_ext2(fg, materials, dummy_white, batch_indices, batches, i, DrawMask, cur_mat_id,
                                 &draws_count);
        }
        { // moving alpha-tested one-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "MOVING-ALPHA-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitAlphaTest;
            i = _draw_range_ext2(fg, materials, dummy_white, batch_indices, batches, i, DrawMask, cur_mat_id,
                                 &draws_count);
        }
        { // moving alpha-tested two-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "MOVING-ALPHA-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitAlphaTest | BDB::BitTwoSided;
            i = _draw_range_ext2(fg, materials, dummy_white, batch_indices, batches, i, DrawMask, cur_mat_id,
                                 &draws_count);
        }
    }
}
