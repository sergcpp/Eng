#include "ExOITScheduleRays.h"

#include <Ren/Context.h>
#include <Ren/DebugMarker.h>
#include <Ren/Gl/GL.h>
#include <Ren/RastState.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgBuilder.h"
#include "../shaders/blit_oit_depth_interface.h"
#include "../shaders/oit_schedule_rays_interface.h"

namespace ExSharedInternal {
uint32_t _draw_range_ext2(const Eng::FgContext &fg, const Ren::ImageMain &white_tex,
                          Ren::Span<const uint32_t> batch_indices, Ren::Span<const Eng::basic_draw_batch_t> batches,
                          uint32_t i, uint64_t mask, uint32_t &cur_mat_id, int *draws_count);
} // namespace ExSharedInternal

void Eng::ExOITScheduleRays::DrawTransparent(const FgContext &fg, const Ren::ImageRWHandle depth_tex) {
    using namespace ExSharedInternal;

    const Ren::BufferROHandle attrib_bufs[] = {fg.AccessROBuffer(vtx_buf1_), fg.AccessROBuffer(vtx_buf2_)};
    const Ren::BufferROHandle ndx_buf = fg.AccessROBuffer(ndx_buf_);

    const Ren::ImageROHandle noise = fg.AccessROImage(noise_);
    const Ren::ImageROHandle dummy_white = fg.AccessROImage(dummy_white_);
    const Ren::BufferROHandle instances = fg.AccessROBuffer(instances_);
    const Ren::BufferROHandle instance_indices = fg.AccessROBuffer(instance_indices_);
    const Ren::BufferROHandle unif_shared_data = fg.AccessROBuffer(shared_data_);
    const Ren::BufferROHandle materials = fg.AccessROBuffer(materials_);
    const Ren::BufferROHandle oit_depth = fg.AccessROBuffer(oit_depth_);
    const Ren::BufferHandle ray_counter = fg.AccessRWBuffer(ray_counter_);
    const Ren::BufferHandle ray_list = fg.AccessRWBuffer(ray_list_);
    const Ren::BufferHandle ray_bitmask = fg.AccessRWBuffer(ray_bitmask_);

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
    glBindFramebuffer(GL_FRAMEBUFFER, storages.framebuffers[fb].first.id);

    const Ren::BufferMain &materials_main = storages.buffers[materials].first;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_MATERIALS_BUF, GLuint(materials_main.buf));
    if (fg.ren_ctx().capabilities.bindless_texture) {
        const Ren::BufferMain &buf_main = storages.buffers[bindless_tex_->rt_inline_textures.buf].first;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_BINDLESS_TEX, GLuint(buf_main.buf));
    }

    const Ren::BufferMain &unif_shared_data_main = storages.buffers[unif_shared_data].first;
    glBindBufferBase(GL_UNIFORM_BUFFER, BIND_UB_SHARED_DATA_BUF, unif_shared_data_main.buf);

    const Ren::ImageMain &noise_main = storages.images[noise].first;
    ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, BIND_NOISE_TEX, noise_main.img);

    const Ren::BufferMain &instances_main = storages.buffers[instances].first;
    ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, BIND_INST_BUF, GLuint(instances_main.views[0].second));
    const Ren::BufferMain &instance_indices_main = storages.buffers[instance_indices].first;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_INST_NDX_BUF, GLuint(instance_indices_main.buf));

    const Ren::BufferMain &oit_depth_main = storages.buffers[oit_depth].first;
    ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, OITScheduleRays::OIT_DEPTH_BUF_SLOT, oit_depth_main.views[0].second);

    const Ren::BufferMain &ray_counter_main = storages.buffers[ray_counter].first;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, OITScheduleRays::RAY_COUNTER_SLOT, GLuint(ray_counter_main.buf));
    const Ren::BufferMain &ray_list_main = storages.buffers[ray_list].first;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, OITScheduleRays::RAY_LIST_SLOT, GLuint(ray_list_main.buf));
    const Ren::BufferMain &ray_bitmask_main = storages.buffers[ray_bitmask].first;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, OITScheduleRays::RAY_BITMASK_SLOT, GLuint(ray_bitmask_main.buf));

    const Ren::ImageMain &dummy_white_main = storages.images[dummy_white].first;

    const Ren::Span<const basic_draw_batch_t> batches = {(*p_list_)->basic_batches};
    const Ren::Span<const uint32_t> batch_indices = {(*p_list_)->basic_batch_indices};

    int draws_count = 0;
    uint32_t i = (*p_list_)->alpha_blend_start_index;
    uint32_t cur_mat_id = 0xffffffff;

    using BDB = basic_draw_batch_t;

    const Ren::ApiContext &api = fg.ren_ctx().api();

    { // Simple meshes
        Ren::DebugMarker _m(api, fg.cmd_buf(), "SIMPLE");

        const Ren::PipelineMain &pi_simple0_main = storages.pipelines[pi_simple_[0]].first;
        const Ren::PipelineMain &pi_simple1_main = storages.pipelines[pi_simple_[1]].first;
        const Ren::PipelineMain &pi_simple2_main = storages.pipelines[pi_simple_[2]].first;

        const Ren::VertexInput &vi = storages.vtx_inputs[pi_simple0_main.vtx_input];
        VertexInput_BindBuffers(api, vi, storages.buffers, attrib_bufs, ndx_buf);
        glUseProgram(storages.programs[pi_simple0_main.prog].first.id);

        { // solid one-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "SOLID-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, dummy_white_main, batch_indices, batches, i, BDB::BitAlphaBlend, cur_mat_id,
                                 &draws_count);

            rast_state = pi_simple1_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, dummy_white_main, batch_indices, batches, i,
                                 BDB::BitAlphaBlend | BDB::BitBackSided, cur_mat_id, &draws_count);
        }
        { // solid two-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "SOLID-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, dummy_white_main, batch_indices, batches, i, BDB::BitAlphaBlend | BDB::BitTwoSided,
                                 cur_mat_id, &draws_count);
        }
        { // moving solid one-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "MOVING-SOLID-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, dummy_white_main, batch_indices, batches, i, BDB::BitAlphaBlend | BDB::BitMoving,
                                 cur_mat_id, &draws_count);
        }
        { // moving solid two-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "MOVING-SOLID-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitTwoSided;
            i = _draw_range_ext2(fg, dummy_white_main, batch_indices, batches, i, DrawMask, cur_mat_id, &draws_count);
        }
        { // alpha-tested one-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "ALPHA-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            i = _draw_range_ext2(fg, dummy_white_main, batch_indices, batches, i,
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
            i = _draw_range_ext2(fg, dummy_white_main, batch_indices, batches, i, DrawMask, cur_mat_id, &draws_count);
        }
        { // moving alpha-tested one-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "MOVING-ALPHA-ONE-SIDED");

            Ren::RastState rast_state = pi_simple0_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitAlphaTest;
            i = _draw_range_ext2(fg, dummy_white_main, batch_indices, batches, i, DrawMask, cur_mat_id, &draws_count);
        }
        { // moving alpha-tested two-sided
            Ren::DebugMarker _mm(api, fg.cmd_buf(), "MOVING-ALPHA-TWO-SIDED");

            Ren::RastState rast_state = pi_simple2_main.rast_state;
            rast_state.viewport[2] = view_state_->ren_res[0];
            rast_state.viewport[3] = view_state_->ren_res[1];
            rast_state.ApplyChanged(fg.rast_state());
            fg.rast_state() = rast_state;

            const uint64_t DrawMask = BDB::BitAlphaBlend | BDB::BitMoving | BDB::BitAlphaTest | BDB::BitTwoSided;
            i = _draw_range_ext2(fg, dummy_white_main, batch_indices, batches, i, DrawMask, cur_mat_id, &draws_count);
        }
    }
}
