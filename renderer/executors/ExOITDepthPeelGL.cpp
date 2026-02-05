#include "ExOITDepthPeel.h"

#include <Ren/Context.h>
#include <Ren/DebugMarker.h>
#include <Ren/GL.h>
#include <Ren/RastState.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgBuilder.h"
#include "../shaders/depth_peel_interface.h"

namespace ExSharedInternal {
void _bind_textures_and_samplers(Ren::Context &ctx, const Ren::Material &mat,
                                 Ren::SmallVectorImpl<Ren::SamplerHandle> &temp_samplers);
uint32_t _draw_range_ext2(const Eng::FgContext &fg, const Ren::MaterialStorage &materials,
                          const Ren::ImageMain &white_tex, Ren::Span<const uint32_t> batch_indices,
                          Ren::Span<const Eng::basic_draw_batch_t> batches, uint32_t i, uint64_t mask,
                          uint32_t &cur_mat_id, int *draws_count);
} // namespace ExSharedInternal

void Eng::ExOITDepthPeel::DrawTransparent(const FgContext &fg, const Ren::ImageRWHandle depth_tex) {
    using namespace ExSharedInternal;

    const Ren::BufferROHandle attrib_bufs[] = {fg.AccessROBuffer(vtx_buf1_), fg.AccessROBuffer(vtx_buf2_)};
    const Ren::BufferROHandle ndx_buf = fg.AccessROBuffer(ndx_buf_);

    const Ren::BufferROHandle instances_buf = fg.AccessROBuffer(instances_buf_);
    const Ren::BufferROHandle instance_indices_buf = fg.AccessROBuffer(instance_indices_buf_);
    const Ren::BufferROHandle unif_shared_data_buf = fg.AccessROBuffer(shared_data_buf_);
    const Ren::BufferROHandle materials_buf = fg.AccessROBuffer(materials_buf_);
    const Ren::ImageROHandle dummy_white = fg.AccessROImage(dummy_white_);

    const Ren::BufferHandle out_depth_buf = fg.AccessRWBuffer(out_depth_buf_);

    if ((*p_list_)->alpha_blend_start_index == -1) {
        return;
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

    const Ren::FramebufferHandle fb = fg.FindOrCreateFramebuffer({}, depth_tex, depth_tex, {});

    // Bind main buffer for drawing
    glBindFramebuffer(GL_FRAMEBUFFER, storages.framebuffers.Get(fb).first.id);

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

    const Ren::BufferMain &instances_buf_main = storages.buffers.Get(instances_buf).first;
    ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, BIND_INST_BUF, GLuint(instances_buf_main.views[0].second));
    const Ren::BufferMain &instance_indices_buf_main = storages.buffers.Get(instance_indices_buf).first;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_INST_NDX_BUF, GLuint(instance_indices_buf_main.buf));

    const Ren::BufferMain &out_depth_buf_main = storages.buffers.Get(out_depth_buf).first;
    glBindImageTexture(GLuint(DepthPeel::OUT_IMG_BUF_SLOT), GLuint(out_depth_buf_main.views[0].second), 0, GL_FALSE, 0,
                       GL_READ_WRITE, GL_R32UI);

    const Ren::ImageMain &dummy_white_main = storages.images.Get(dummy_white).first;

    const Ren::Span<const basic_draw_batch_t> batches = {(*p_list_)->basic_batches};
    const Ren::Span<const uint32_t> batch_indices = {(*p_list_)->basic_batch_indices};
    const auto &materials = *(*p_list_)->materials;

    int draws_count = 0;
    uint32_t i = (*p_list_)->alpha_blend_start_index;
    uint32_t cur_mat_id = 0xffffffff;

    using BDB = basic_draw_batch_t;

    { // Simple meshes
        Ren::DebugMarker _m(fg.ren_ctx().api(), fg.cmd_buf(), "SIMPLE");

        const Ren::PipelineMain &pi_simple0_main = storages.pipelines.Get(pi_simple_[0]).first;
        const Ren::PipelineMain &pi_simple1_main = storages.pipelines.Get(pi_simple_[1]).first;
        const Ren::PipelineMain &pi_simple2_main = storages.pipelines.Get(pi_simple_[2]).first;

        const Ren::VertexInputMain &vi = storages.vtx_inputs.Get(pi_simple0_main.vtx_input).first;
        VertexInput_BindBuffers(fg.ren_ctx().api(), vi, storages.buffers, attrib_bufs, ndx_buf);
        glUseProgram(storages.programs.Get(pi_simple0_main.prog).first.id);

        { // solid one-sided
            Ren::DebugMarker _mm(fg.ren_ctx().api(), fg.cmd_buf(), "SOLID-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white_main, batch_indices, batches, i, BDB::BitAlphaBlend,
                                 cur_mat_id, &draws_count);

            rast_state = pi_simple1_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white_main, batch_indices, batches, i,
                                 BDB::BitAlphaBlend | BDB::BitBackSided, cur_mat_id, &draws_count);
        }
        { // solid two-sided
            Ren::DebugMarker _mm(fg.ren_ctx().api(), fg.cmd_buf(), "SOLID-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white_main, batch_indices, batches, i,
                                 BDB::BitAlphaBlend | BDB::BitTwoSided, cur_mat_id, &draws_count);
        }
        { // moving solid one-sided
            Ren::DebugMarker _mm(fg.ren_ctx().api(), fg.cmd_buf(), "MOVING-SOLID-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white_main, batch_indices, batches, i,
                                 BDB::BitAlphaBlend | BDB::BitMoving, cur_mat_id, &draws_count);
        }
        { // moving solid two-sided
            Ren::DebugMarker _mm(fg.ren_ctx().api(), fg.cmd_buf(), "MOVING-SOLID-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitTwoSided;
            i = _draw_range_ext2(fg, materials, dummy_white_main, batch_indices, batches, i, DrawMask, cur_mat_id,
                                 &draws_count);
        }
        { // alpha-tested one-sided
            Ren::DebugMarker _mm(fg.ren_ctx().api(), fg.cmd_buf(), "ALPHA-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, materials, dummy_white_main, batch_indices, batches, i,
                                 BDB::BitAlphaBlend | BDB::BitAlphaTest, cur_mat_id, &draws_count);
        }
        { // alpha-tested two-sided
            Ren::DebugMarker _mm(fg.ren_ctx().api(), fg.cmd_buf(), "ALPHA-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitAlphaTest | BDB::BitTwoSided;
            i = _draw_range_ext2(fg, materials, dummy_white_main, batch_indices, batches, i, DrawMask, cur_mat_id,
                                 &draws_count);
        }
        { // moving alpha-tested one-sided
            Ren::DebugMarker _mm(fg.ren_ctx().api(), fg.cmd_buf(), "MOVING-ALPHA-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitAlphaTest;
            i = _draw_range_ext2(fg, materials, dummy_white_main, batch_indices, batches, i, DrawMask, cur_mat_id,
                                 &draws_count);
        }
        { // moving alpha-tested two-sided
            Ren::DebugMarker _mm(fg.ren_ctx().api(), fg.cmd_buf(), "MOVING-ALPHA-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitAlphaTest | BDB::BitTwoSided;
            i = _draw_range_ext2(fg, materials, dummy_white_main, batch_indices, batches, i, DrawMask, cur_mat_id,
                                 &draws_count);
        }
    }
}
