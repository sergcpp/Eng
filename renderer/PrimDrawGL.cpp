#include "PrimDraw.h"

#include <Ren/Context.h>
#include <Ren/Framebuffer.h>
#include <Ren/GL.h>
#include <Ren/ProbeStorage.h>

#include "Renderer_Structs.h"

namespace PrimDrawInternal {
extern const int SphereIndicesCount;
} // namespace PrimDrawInternal

void Eng::PrimDraw::DrawPrim(Ren::CommandBuffer cmd_buf, ePrim prim, const Ren::ProgramHandle p,
                             const Ren::RenderTarget depth_rt, Ren::Span<const Ren::RenderTarget> color_rts,
                             const Ren::RastState &new_rast_state, Ren::RastState &applied_rast_state,
                             Ren::Span<const Ren::Binding> bindings, const void *uniform_data,
                             const int uniform_data_len, const int uniform_data_offset, const int instance_count) {
    using namespace PrimDrawInternal;

    const Ren::Framebuffer *fb =
        FindOrCreateFramebuffer(nullptr, new_rast_state.depth.test_enabled ? depth_rt : Ren::RenderTarget{},
                                new_rast_state.stencil.enabled ? depth_rt : Ren::RenderTarget{}, color_rts);

    new_rast_state.ApplyChanged(applied_rast_state);
    applied_rast_state = new_rast_state;

    const Ren::StoragesRef &storages = ctx_->storages();

    glBindFramebuffer(GL_FRAMEBUFFER, fb->id());

    for (const auto &b : bindings) {
        if (b.trg == Ren::eBindTarget::Tex || b.trg == Ren::eBindTarget::TexSampled) {
            auto texture_id = GLuint(b.handle.img->id());
            if (b.handle.view_index) {
                texture_id = GLuint(b.handle.img->handle().views[b.handle.view_index - 1]);
            }
            ren_glBindTextureUnit_Comp(GLBindTarget(*b.handle.img, b.handle.view_index), GLuint(b.loc + b.offset),
                                       texture_id);
            if (b.handle.sampler) {
                ren_glBindSampler(GLuint(b.loc + b.offset), storages.samplers.Get(b.handle.sampler).first.id);
            } else {
                ren_glBindSampler(GLuint(b.loc + b.offset), 0);
            }
        } else if (b.trg == Ren::eBindTarget::UBuf) {
            const auto &[buf_main, buf_cold] = storages.buffers.Get(b.handle.buf);
            if (b.offset || b.size) {
                glBindBufferRange(GL_UNIFORM_BUFFER, b.loc, buf_main.buf, b.offset,
                                  b.size ? b.size : (buf_cold.size - b.offset));
            } else {
                glBindBufferBase(GL_UNIFORM_BUFFER, b.loc, buf_main.buf);
            }
        } else if (b.trg == Ren::eBindTarget::SBufRO || b.trg == Ren::eBindTarget::SBufRW) {
            const auto &[buf_main, buf_cold] = storages.buffers.Get(b.handle.buf);
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER, b.loc, buf_main.buf, b.offset,
                              b.size ? b.size : (buf_cold.size - b.offset));
        } else if (b.trg == Ren::eBindTarget::UTBuf) {
            const auto &[buf_main, buf_cold] = storages.buffers.Get(b.handle.buf);
            ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, GLuint(b.loc),
                                       GLuint(buf_main.views[b.handle.view_index].second));
        } else if (b.trg == Ren::eBindTarget::STBufRO) {
            const auto &[buf_main, buf_cold] = storages.buffers.Get(b.handle.buf);
            glBindImageTexture(GLuint(b.loc + b.offset), GLuint(buf_main.views[b.handle.view_index].second), 0,
                               GL_FALSE, 0, GL_READ_ONLY,
                               GLInternalFormatFromFormat(buf_main.views[b.handle.view_index].first));
        } else if (b.trg == Ren::eBindTarget::STBufRW) {
            const auto &[buf_main, buf_cold] = storages.buffers.Get(b.handle.buf);
            glBindImageTexture(GLuint(b.loc + b.offset), GLuint(buf_main.views[b.handle.view_index].second), 0,
                               GL_FALSE, 0, GL_READ_WRITE,
                               GLInternalFormatFromFormat(buf_main.views[b.handle.view_index].first));
        } else if (b.trg == Ren::eBindTarget::Sampler) {
            ren_glBindSampler(GLuint(b.loc + b.offset), storages.samplers.Get(b.handle.sampler).first.id);
        } else if (b.trg == Ren::eBindTarget::ImageRO || b.trg == Ren::eBindTarget::ImageRW) {
            auto texture_id = GLuint(b.handle.img->id());
            if (b.handle.view_index) {
                texture_id = GLuint(b.handle.img->handle().views[b.handle.view_index - 1]);
            }
            glBindImageTexture(GLuint(b.loc + b.offset), texture_id, 0, GL_FALSE, 0,
                               b.trg == Ren::eBindTarget::ImageRO ? GL_READ_ONLY : GL_READ_WRITE,
                               GLInternalFormatFromFormat(b.handle.img->params.format));
        }
    }

    const Ren::ProgramMain &p_main = ctx_->programs().Get(p).first;
    glUseProgram(p_main.id);

    const Ren::ApiContext &api = ctx_->api();

    Ren::BufferMain temp_unif_buffer_main = {};
    Ren::BufferCold temp_unif_buffer_cold = {};
    if (uniform_data && uniform_data_len) {
        if (!Buffer_Init(api, temp_unif_buffer_main, temp_unif_buffer_cold, Ren::String{"Temp uniform buf"},
                         Ren::eBufType::Uniform, uniform_data_len, ctx_->log())) {
            ctx_->log()->Error("Failed to initialize temp uniform buffer");
            return;
        }
        Ren::BufferMain temp_stage_buffer_main = {};
        Ren::BufferCold temp_stage_buffer_cold = {};
        if (!Buffer_Init(api, temp_stage_buffer_main, temp_stage_buffer_cold, Ren::String{"Temp upload buf"},
                         Ren::eBufType::Upload, uniform_data_len, ctx_->log())) {
            ctx_->log()->Error("Failed to initialize temp upload buffer");
            Buffer_DestroyImmediately(api, temp_unif_buffer_main, temp_unif_buffer_cold);
            return;
        }
        {
            uint8_t *stage_data = Buffer_Map(api, temp_stage_buffer_main, temp_stage_buffer_cold);
            memcpy(stage_data, uniform_data, uniform_data_len);
            Buffer_Unmap(api, temp_stage_buffer_main, temp_stage_buffer_cold);
        }
        CopyBufferToBuffer(api, temp_stage_buffer_main, 0, temp_unif_buffer_main, 0, uniform_data_len, nullptr);
        Buffer_Destroy(api, temp_stage_buffer_main, temp_stage_buffer_cold);
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, Eng::BIND_PUSH_CONSTANT_BUF, temp_unif_buffer_main.buf);

    if (prim == ePrim::Quad) {
        const Ren::VertexInputMain &vi = storages.vtx_inputs.Get(fs_quad_vtx_input_).first;

        glBindVertexArray(VertexInput_GetVAO(vi, storages.buffers));
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const GLvoid *)uintptr_t(quad_ndx_.offset),
                                instance_count);
    } else if (prim == ePrim::Sphere) {
        const Ren::VertexInputMain &vi = storages.vtx_inputs.Get(sphere_vtx_input_).first;

        glBindVertexArray(VertexInput_GetVAO(vi, storages.buffers));
        glDrawElementsInstanced(GL_TRIANGLES, GLsizei(SphereIndicesCount), GL_UNSIGNED_SHORT,
                                (void *)uintptr_t(sphere_ndx_.offset), instance_count);
    }

    Buffer_Destroy(api, temp_unif_buffer_main, temp_unif_buffer_cold);

#ifndef NDEBUG
    Ren::ResetGLState();
#endif
}

void Eng::PrimDraw::DrawPrim(ePrim prim, const Ren::ProgramHandle p, const Ren::RenderTarget depth_rt,
                             Ren::Span<const Ren::RenderTarget> color_rts, const Ren::RastState &new_rast_state,
                             Ren::RastState &applied_rast_state, Ren::Span<const Ren::Binding> bindings,
                             const void *uniform_data, const int uniform_data_len, const int uniform_data_offset,
                             const int instance_count) {
    DrawPrim({}, prim, p, depth_rt, color_rts, new_rast_state, applied_rast_state, bindings, uniform_data,
             uniform_data_len, uniform_data_offset, instance_count);
}

void Eng::PrimDraw::ClearTarget(Ren::CommandBuffer cmd_buf, Ren::RenderTarget depth_rt,
                                Ren::Span<const Ren::RenderTarget> color_rts) {
    const Ren::Framebuffer *fb = FindOrCreateFramebuffer(nullptr, depth_rt, depth_rt, color_rts);

    glBindFramebuffer(GL_FRAMEBUFFER, fb->id());

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Eng::PrimDraw::ClearTarget(Ren::RenderTarget depth_rt, Ren::Span<const Ren::RenderTarget> color_rts) {
    ClearTarget({}, depth_rt, color_rts);
}