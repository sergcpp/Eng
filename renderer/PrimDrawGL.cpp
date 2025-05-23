#include "PrimDraw.h"

#include <Ren/Context.h>
#include <Ren/Framebuffer.h>
#include <Ren/GL.h>
#include <Ren/ProbeStorage.h>

#include "Renderer_Structs.h"

namespace PrimDrawInternal {
extern const int SphereIndicesCount;
} // namespace PrimDrawInternal

Eng::PrimDraw::~PrimDraw() = default;

void Eng::PrimDraw::DrawPrim(Ren::CommandBuffer cmd_buf, ePrim prim, const Ren::ProgramRef &p,
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

    glBindFramebuffer(GL_FRAMEBUFFER, fb->id());

    for (const auto &b : bindings) {
        if (b.trg == Ren::eBindTarget::Tex || b.trg == Ren::eBindTarget::TexSampled) {
            auto texture_id = GLuint(b.handle.tex->id());
            if (b.handle.view_index) {
                texture_id = GLuint(b.handle.tex->handle().views[b.handle.view_index - 1]);
            }
            ren_glBindTextureUnit_Comp(GLBindTarget(*b.handle.tex, b.handle.view_index), GLuint(b.loc + b.offset),
                                       texture_id);
            if (b.handle.sampler) {
                ren_glBindSampler(GLuint(b.loc + b.offset), b.handle.sampler->id());
            } else {
                ren_glBindSampler(GLuint(b.loc + b.offset), 0);
            }
        } else if (b.trg == Ren::eBindTarget::UBuf) {
            glBindBufferRange(GL_UNIFORM_BUFFER, b.loc, b.handle.buf->id(), b.offset,
                              b.size ? b.size : b.handle.buf->size());
        } else if (b.trg == Ren::eBindTarget::SBufRO || b.trg == Ren::eBindTarget::SBufRW) {
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER, b.loc, b.handle.buf->id(), b.offset,
                              b.size ? b.size : b.handle.buf->size());
        } else if (b.trg == Ren::eBindTarget::UTBuf) {
            ren_glBindTextureUnit_Comp(GL_TEXTURE_BUFFER, GLuint(b.loc),
                                       GLuint(b.handle.buf->view(b.handle.view_index).second));
        } else if (b.trg == Ren::eBindTarget::STBufRO) {
            glBindImageTexture(GLuint(b.loc + b.offset), GLuint(b.handle.buf->view(b.handle.view_index).second), 0,
                               GL_FALSE, 0, GL_READ_ONLY,
                               GLInternalFormatFromTexFormat(b.handle.buf->view(b.handle.view_index).first));
        } else if (b.trg == Ren::eBindTarget::STBufRW) {
            glBindImageTexture(GLuint(b.loc + b.offset), GLuint(b.handle.buf->view(b.handle.view_index).second), 0,
                               GL_FALSE, 0, GL_READ_WRITE,
                               GLInternalFormatFromTexFormat(b.handle.buf->view(b.handle.view_index).first));
        } else if (b.trg == Ren::eBindTarget::Sampler) {
            ren_glBindSampler(GLuint(b.loc + b.offset), b.handle.sampler->id());
        } else if (b.trg == Ren::eBindTarget::ImageRO || b.trg == Ren::eBindTarget::ImageRW) {
            auto texture_id = GLuint(b.handle.tex->id());
            if (b.handle.view_index) {
                texture_id = GLuint(b.handle.tex->handle().views[b.handle.view_index - 1]);
            }
            glBindImageTexture(GLuint(b.loc + b.offset), texture_id, 0, GL_FALSE, 0,
                               b.trg == Ren::eBindTarget::ImageRO ? GL_READ_ONLY : GL_READ_WRITE,
                               GLInternalFormatFromTexFormat(b.handle.tex->params.format));
        }
    }

    glUseProgram(p->id());

    Ren::Buffer temp_stage_buffer, temp_unif_buffer;
    if (uniform_data) {
        temp_stage_buffer = Ren::Buffer("Temp upload buf", ctx_->api_ctx(), Ren::eBufType::Upload, uniform_data_len);
        {
            uint8_t *stage_data = temp_stage_buffer.Map();
            memcpy(stage_data, uniform_data, uniform_data_len);
            temp_stage_buffer.Unmap();
        }
        temp_unif_buffer = Ren::Buffer("Temp uniform buf", ctx_->api_ctx(), Ren::eBufType::Uniform, uniform_data_len);
        CopyBufferToBuffer(temp_stage_buffer, 0, temp_unif_buffer, 0, uniform_data_len, nullptr);

        glBindBufferBase(GL_UNIFORM_BUFFER, Eng::BIND_PUSH_CONSTANT_BUF, temp_unif_buffer.id());
    }

    if (prim == ePrim::Quad) {
        glBindVertexArray(fs_quad_vtx_input_->GetVAO());
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const GLvoid *)uintptr_t(quad_ndx_.offset),
                                instance_count);
    } else if (prim == ePrim::Sphere) {
        glBindVertexArray(sphere_vtx_input_->GetVAO());
        glDrawElementsInstanced(GL_TRIANGLES, GLsizei(SphereIndicesCount), GL_UNSIGNED_SHORT,
                                (void *)uintptr_t(sphere_ndx_.offset), instance_count);
    }

#ifndef NDEBUG
    Ren::ResetGLState();
#endif
}

void Eng::PrimDraw::DrawPrim(ePrim prim, const Ren::ProgramRef &p, const Ren::RenderTarget depth_rt,
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