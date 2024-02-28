#include "RpShadowMaps.h"

#include <Ren/Context.h>
#include <Ren/GL.h>
#include <Ren/RastState.h>

#include "../Renderer_Structs.h"
#include "../shaders/shadow_interface.h"

namespace RpSharedInternal {
void _bind_texture3_and_sampler3(Ren::Context &ctx, const Ren::Material &mat,
                                 Ren::SmallVectorImpl<Ren::SamplerRef> &temp_samplers);
}
namespace RpShadowMapsInternal {
using namespace RpSharedInternal;

void _adjust_bias_and_viewport(Ren::RastState &rast_state, const Eng::ShadowList &sh_list) {
    Ren::RastState new_rast_state = rast_state;

    new_rast_state.depth_bias.slope_factor = sh_list.bias[0];
    new_rast_state.depth_bias.constant_offset = sh_list.bias[1];

    new_rast_state.viewport[0] = sh_list.shadow_map_pos[0];
    new_rast_state.viewport[1] = sh_list.shadow_map_pos[1];
    new_rast_state.viewport[2] = sh_list.shadow_map_size[0];
    new_rast_state.viewport[3] = sh_list.shadow_map_size[1];

    new_rast_state.scissor.rect[0] = sh_list.scissor_test_pos[0];
    new_rast_state.scissor.rect[1] = sh_list.scissor_test_pos[1];
    new_rast_state.scissor.rect[2] = sh_list.scissor_test_size[0];
    new_rast_state.scissor.rect[3] = sh_list.scissor_test_size[1];

    new_rast_state.ApplyChanged(rast_state);
    rast_state = new_rast_state;
}
} // namespace RpShadowMapsInternal

void Eng::RpShadowMaps::DrawShadowMaps(RpBuilder &builder, RpAllocTex &shadowmap_tex) {
    using namespace RpShadowMapsInternal;

    Ren::RastState rast_state;
    rast_state.poly.cull = uint8_t(Ren::eCullFace::None);
    rast_state.poly.depth_bias_mode = uint8_t(Ren::eDepthBiasMode::Dynamic);

    rast_state.depth.test_enabled = true;
    rast_state.depth.compare_op = unsigned(Ren::eCompareOp::Less);
    rast_state.scissor.enabled = true;
    rast_state.blend.enabled = false;

    rast_state.ApplyChanged(builder.rast_state());
    builder.rast_state() = rast_state;

    Ren::Context &ctx = builder.ctx();
    Ren::ApiContext *api_ctx = ctx.api_ctx();

    RpAllocBuf &unif_shared_data_buf = builder.GetReadBuffer(shared_data_buf_);
    RpAllocBuf &instances_buf = builder.GetReadBuffer(instances_buf_);
    RpAllocBuf &instance_indices_buf = builder.GetReadBuffer(instance_indices_buf_);
    RpAllocBuf &materials_buf = builder.GetReadBuffer(materials_buf_);
    RpAllocBuf &textures_buf = builder.GetReadBuffer(textures_buf_);

    RpAllocTex &noise_tex = builder.GetReadTexture(noise_tex_);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_MATERIALS_BUF, GLuint(materials_buf.ref->id()));
    if (ctx.capabilities.bindless_texture) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_BINDLESS_TEX, GLuint(textures_buf.ref->id()));
    }

    ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, BIND_INST_BUF, GLuint(instances_buf.tbos[0]->id()));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BIND_INST_NDX_BUF, GLuint(instance_indices_buf.ref->id()));

    glBindBufferBase(GL_UNIFORM_BUFFER, BIND_UB_SHARED_DATA_BUF, GLuint(unif_shared_data_buf.ref->id()));

    ren_glBindTextureUnit_Comp(GL_TEXTURE_2D, BIND_NOISE_TEX, noise_tex.ref->id());

    glBindFramebuffer(GL_FRAMEBUFFER, shadow_fb_.id());

    bool region_cleared[MAX_SHADOWMAPS_TOTAL] = {};

    // draw opaque objects
    glBindVertexArray(vi_depth_pass_solid_.gl_vao());
    glUseProgram(pi_solid_.prog()->id());

    [[maybe_unused]] int draw_calls_count = 0;

    for (int i = 0; i < int((*p_list_)->shadow_lists.count); i++) {
        const ShadowList &sh_list = (*p_list_)->shadow_lists.data[i];
        if (!sh_list.shadow_batch_count) {
            continue;
        }

        _adjust_bias_and_viewport(rast_state, sh_list);

        if (!region_cleared[i]) {
            glClear(GL_DEPTH_BUFFER_BIT);
            region_cleared[i] = true;
        }

        glUniformMatrix4fv(Shadow::U_M_MATRIX_LOC, 1, GL_FALSE,
                           Ren::ValuePtr((*p_list_)->shadow_regions.data[i].clip_from_world));

        for (uint32_t j = sh_list.shadow_batch_start; j < sh_list.shadow_batch_start + sh_list.shadow_batch_count;
             ++j) {
            const auto &batch = (*p_list_)->shadow_batches[(*p_list_)->shadow_batch_indices[j]];
            if (!batch.instance_count || batch.alpha_test_bit || batch.type_bits == BasicDrawBatch::TypeVege) {
                continue;
            }

            glUniform1ui(REN_U_BASE_INSTANCE_LOC, batch.instance_start);

            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, batch.indices_count, GL_UNSIGNED_INT,
                                              (const GLvoid *)uintptr_t(batch.indices_offset * sizeof(uint32_t)),
                                              (GLsizei)batch.instance_count, (GLint)batch.base_vertex);
            ++draw_calls_count;
        }
    }

    // draw opaque vegetation
    glBindVertexArray(vi_depth_pass_vege_solid_.gl_vao());
    glUseProgram(pi_vege_solid_.prog()->id());

    for (int i = 0; i < int((*p_list_)->shadow_lists.count); i++) {
        const ShadowList &sh_list = (*p_list_)->shadow_lists.data[i];
        if (!sh_list.shadow_batch_count) {
            continue;
        }

        _adjust_bias_and_viewport(rast_state, sh_list);

        if (!region_cleared[i]) {
            glClear(GL_DEPTH_BUFFER_BIT);
            region_cleared[i] = true;
        }

        glUniformMatrix4fv(Shadow::U_M_MATRIX_LOC, 1, GL_FALSE,
                           Ren::ValuePtr((*p_list_)->shadow_regions.data[i].clip_from_world));

        for (uint32_t j = sh_list.shadow_batch_start; j < sh_list.shadow_batch_start + sh_list.shadow_batch_count;
             ++j) {
            const auto &batch = (*p_list_)->shadow_batches[(*p_list_)->shadow_batch_indices[j]];
            if (!batch.instance_count || batch.alpha_test_bit || batch.type_bits != BasicDrawBatch::TypeVege) {
                continue;
            }

            glUniform1ui(REN_U_BASE_INSTANCE_LOC, batch.instance_start);

            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, batch.indices_count, GL_UNSIGNED_INT,
                                              (const GLvoid *)uintptr_t(batch.indices_offset * sizeof(uint32_t)),
                                              (GLsizei)batch.instance_count, (GLint)batch.base_vertex);
            ++draw_calls_count;
        }
    }

    // draw transparent (alpha-tested) objects
    glBindVertexArray(vi_depth_pass_transp_.gl_vao());
    glUseProgram(pi_transp_.prog()->id());

    for (int i = 0; i < int((*p_list_)->shadow_lists.count); i++) {
        const ShadowList &sh_list = (*p_list_)->shadow_lists.data[i];
        if (!sh_list.shadow_batch_count) {
            continue;
        }

        _adjust_bias_and_viewport(rast_state, sh_list);

        if (!region_cleared[i]) {
            glClear(GL_DEPTH_BUFFER_BIT);
            region_cleared[i] = true;
        }

        glUniformMatrix4fv(Shadow::U_M_MATRIX_LOC, 1, GL_FALSE,
                           Ren::ValuePtr((*p_list_)->shadow_regions.data[i].clip_from_world));

        uint32_t cur_mat_id = 0xffffffff;
        for (uint32_t j = sh_list.shadow_batch_start; j < sh_list.shadow_batch_start + sh_list.shadow_batch_count;
             ++j) {
            const auto &batch = (*p_list_)->shadow_batches[(*p_list_)->shadow_batch_indices[j]];
            if (!batch.instance_count || !batch.alpha_test_bit || batch.type_bits == BasicDrawBatch::TypeVege) {
                continue;
            }

            if (!ctx.capabilities.bindless_texture && batch.material_index != cur_mat_id) {
                const Ren::Material &mat = (*p_list_)->materials->at(batch.material_index);
                _bind_texture3_and_sampler3(builder.ctx(), mat, builder.temp_samplers);
                cur_mat_id = batch.material_index;
            }

            glUniform1ui(REN_U_BASE_INSTANCE_LOC, batch.instance_start);

            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, batch.indices_count, GL_UNSIGNED_INT,
                                              (const GLvoid *)uintptr_t(batch.indices_offset * sizeof(uint32_t)),
                                              GLsizei(batch.instance_count), GLint(batch.base_vertex));
            ++draw_calls_count;
        }
    }

    // draw transparent (alpha-tested) vegetation
    glBindVertexArray(vi_depth_pass_vege_transp_.gl_vao());
    glUseProgram(pi_vege_transp_.prog()->id());

    for (int i = 0; i < int((*p_list_)->shadow_lists.count); i++) {
        const ShadowList &sh_list = (*p_list_)->shadow_lists.data[i];
        if (!sh_list.shadow_batch_count) {
            continue;
        }

        _adjust_bias_and_viewport(rast_state, sh_list);

        if (!region_cleared[i]) {
            glClear(GL_DEPTH_BUFFER_BIT);
            region_cleared[i] = true;
        }

        glUniformMatrix4fv(Shadow::U_M_MATRIX_LOC, 1, GL_FALSE,
                           Ren::ValuePtr((*p_list_)->shadow_regions.data[i].clip_from_world));

        uint32_t cur_mat_id = 0xffffffff;
        for (uint32_t j = sh_list.shadow_batch_start; j < sh_list.shadow_batch_start + sh_list.shadow_batch_count;
             ++j) {
            const auto &batch = (*p_list_)->shadow_batches[(*p_list_)->shadow_batch_indices[j]];
            if (!batch.instance_count || !batch.alpha_test_bit || batch.type_bits != BasicDrawBatch::TypeVege) {
                continue;
            }

            if (!ctx.capabilities.bindless_texture && batch.material_index != cur_mat_id) {
                const Ren::Material &mat = (*p_list_)->materials->at(batch.material_index);
                _bind_texture3_and_sampler3(builder.ctx(), mat, builder.temp_samplers);
                cur_mat_id = batch.material_index;
            }

            glUniform1ui(REN_U_BASE_INSTANCE_LOC, batch.instance_start);

            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, batch.indices_count, GL_UNSIGNED_INT,
                                              (const GLvoid *)uintptr_t(batch.indices_offset * sizeof(uint32_t)),
                                              GLsizei(batch.instance_count), GLint(batch.base_vertex));
            ++draw_calls_count;
        }
    }

    glDisable(GL_SCISSOR_TEST);
    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBindVertexArray(0);
    Ren::GLUnbindSamplers(0, 1);
}
