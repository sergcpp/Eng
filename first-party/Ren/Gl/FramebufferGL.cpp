#include "../Framebuffer.h"

#include "GL.h"

#ifndef NDEBUG
#define VERBOSE_LOGGING
#endif

bool Ren::Framebuffer_Init(const ApiContext &api, FramebufferMain &fb_main, FramebufferCold &fb_cold,
                           const StoragesRef &storages, const RenderPassROHandle render_pass,
                           const FramebufferAttachment &depth, const FramebufferAttachment &stencil,
                           Span<const FramebufferAttachment> color_attachments, ILog *log) {
    GLenum target = GL_TEXTURE_2D;
    bool cube = false;

    ImageRWHandle first_img = depth.img;
    if (!first_img) {
        first_img = stencil.img;
    }
    if (!first_img) {
        assert(!color_attachments.empty());
        first_img = color_attachments[0].img;
    }
    if (first_img) {
        const auto &[img_main, img_cold] = storages.images.Get(first_img);
        if (img_cold.params.samples > 1) {
            target = GL_TEXTURE_2D_MULTISAMPLE;
        }
        if (img_cold.params.flags & eImgFlags::CubeMap) {
            cube = true;
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        }
    }

    if (color_attachments.size() == 1 &&
        (!color_attachments[0] || storages.images.Get(color_attachments[0].img).first.img == 0)) {
        // default backbuffer
        return true;
    }

    GLuint fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    int w = -1, h = -1;

    SmallVector<GLenum, 4> draw_buffers;
    for (int i = 0; i < color_attachments.size(); i++) {
        if (color_attachments[i]) {
            const auto &[img_main, img_cold] = storages.images.Get(color_attachments[i].img);
            if (cube) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GLenum(i),
                                       target + color_attachments[i].view_index - 1, GLuint(img_main.img), 0);
            } else {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, GLuint(img_main.img), 0);
            }
            draw_buffers.push_back(GL_COLOR_ATTACHMENT0 + i);

            if (w == -1) {
                w = img_cold.params.w;
                h = img_cold.params.h;
            }
            assert(w == img_cold.params.w);
            assert(h == img_cold.params.h);
        } else {
            draw_buffers.push_back(GL_NONE);
        }
    }

    glDrawBuffers(GLsizei(draw_buffers.size()), draw_buffers.data());

    if (depth) {
        const auto &[depth_main, depth_cold] = storages.images.Get(depth.img);
        if (depth == stencil) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, target, GLuint(depth_main.img), 0);
        } else {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, GLuint(depth_main.img), 0);
        }

        if (w == -1) {
            w = depth_cold.params.w;
            h = depth_cold.params.h;
        }
        assert(w == depth_cold.params.w);
        assert(h == depth_cold.params.h);
    }

    if (stencil && depth != stencil) {
        const auto &[stencil_main, stencil_cold] = storages.images.Get(stencil.img);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, target, GLuint(stencil_main.img), 0);
    }

    const GLenum s = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (s != GL_FRAMEBUFFER_COMPLETE) {
        log->Error("Framebuffer creation failed (error %i)", int(s));
#ifdef VERBOSE_LOGGING
    } else {
        log->Info("Framebuffer %u created", fb);
#endif
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    fb_main.id = uint32_t(fb);
    fb_main.renderpass = render_pass;
    fb_main.w = uint16_t(w);
    fb_main.h = uint16_t(h);

    fb_cold.color_attachments.assign(color_attachments.begin(), color_attachments.end());
    fb_cold.depth_attachment = depth;
    fb_cold.stencil_attachment = stencil;

    return (s == GL_FRAMEBUFFER_COMPLETE);
}

void Ren::Framebuffer_Destroy(const ApiContext &api, FramebufferMain &fb_main, FramebufferCold &fb_cold) {
    if (fb_main.id) {
        auto fb = GLuint(fb_main.id);
        glDeleteFramebuffers(1, &fb);
    }
    fb_main = {};
    fb_cold = {};
}

void Ren::Framebuffer_DestroyImmediately(const ApiContext &api, FramebufferMain &fb_main, FramebufferCold &fb_cold) {
    Framebuffer_Destroy(api, fb_main, fb_cold);
}

bool Ren::Framebuffer_LessThan(const FramebufferMain &fb_main, const FramebufferCold &fb_cold,
                               const RenderPassROHandle render_pass, const FramebufferAttachment &depth_attachment,
                               const FramebufferAttachment &stencil_attachment,
                               Span<const FramebufferAttachment> color_attachments) {
    if (fb_main.renderpass < render_pass) {
        return true;
    } else if (fb_main.renderpass == render_pass) {
        if (fb_cold.depth_attachment < depth_attachment) {
            return true;
        } else if (fb_cold.depth_attachment == depth_attachment) {
            if (fb_cold.stencil_attachment < stencil_attachment) {
                return true;
            } else if (fb_cold.stencil_attachment == stencil_attachment) {
                return Span<const FramebufferAttachment>(fb_cold.color_attachments) < color_attachments;
            }
        }
    }
    return false;
}

bool Ren::Framebuffer_LessThan(const FramebufferMain &fb_main, const FramebufferCold &fb_cold,
                               const RenderPassROHandle render_pass, const RenderTarget &depth_attachment,
                               const RenderTarget &stencil_attachment, Span<const RenderTarget> color_attachments) {
    if (fb_main.renderpass < render_pass) {
        return true;
    } else if (fb_main.renderpass == render_pass) {
        if (fb_cold.depth_attachment < depth_attachment) {
            return true;
        } else if (fb_cold.depth_attachment == depth_attachment) {
            if (fb_cold.stencil_attachment < stencil_attachment) {
                return true;
            } else if (fb_cold.stencil_attachment == stencil_attachment) {
                return Span<const FramebufferAttachment>(fb_cold.color_attachments) < color_attachments;
            }
        }
    }
    return false;
}

#undef VERBOSE_LOGGING
