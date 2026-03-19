#include "../Framebuffer.h"

#include "VKCtx.h"

#ifndef NDEBUG
#define VERBOSE_LOGGING
#endif

bool Ren::Framebuffer_Init(const ApiContext &api, FramebufferMain &fb_main, FramebufferCold &fb_cold,
                           const StoragesRef &storages, const RenderPassROHandle render_pass,
                           const FramebufferAttachment &depth, const FramebufferAttachment &stencil,
                           Span<const FramebufferAttachment> color_attachments, ILog *log) {
    SmallVector<VkImageView, 4> image_views;

    int w = -1, h = -1;

    if (depth) {
        const auto &[img_main, img_cold] = storages.images.Get(depth.img);
        image_views.push_back(img_main.views[0]);
        fb_cold.depth_attachment = depth;
        w = img_cold.params.w;
        h = img_cold.params.h;
    }

    if (stencil) {
        const auto &[stencil_main, stencil_cold] = storages.images.Get(stencil.img);
        fb_cold.stencil_attachment = stencil;
        if (depth) {
            const auto &[depth_main, depth_cold] = storages.images.Get(depth.img);
            if (stencil_main.views[0] != depth_main.views[0]) {
                image_views.push_back(stencil_main.views[0]);
            }
        } else {
            image_views.push_back(stencil_main.views[0]);
        }
    }

    for (int i = 0; i < color_attachments.size(); i++) {
        if (color_attachments[i]) {
            const auto &[img_main, img_cold] = storages.images.Get(color_attachments[i].img);
            image_views.push_back(img_main.views[color_attachments[i].view_index]);
            fb_cold.color_attachments.push_back(color_attachments[i]);
            if (w == -1) {
                w = img_cold.params.w;
                h = img_cold.params.h;
            }
            assert(w == img_cold.params.w);
            assert(h == img_cold.params.h);
        } else {
            fb_cold.color_attachments.emplace_back();
        }
    }

    fb_main.renderpass = render_pass;
    fb_main.w = uint16_t(w);
    fb_main.h = uint16_t(h);

    VkFramebufferCreateInfo framebuf_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebuf_create_info.renderPass = storages.render_passes.Get(render_pass).handle;
    framebuf_create_info.attachmentCount = image_views.size();
    framebuf_create_info.pAttachments = image_views.data();
    framebuf_create_info.width = w;
    framebuf_create_info.height = h;
    framebuf_create_info.layers = 1;

    const VkResult res = api.vkCreateFramebuffer(api.device, &framebuf_create_info, nullptr, &fb_main.handle);
    if (res != VK_SUCCESS) {
        log->Error("Framebuffer creation failed (error %i)", int(res));
#ifdef VERBOSE_LOGGING
    } else {
        log->Info("Framebuffer %p created (%i attachments)", fb_main.handle, int(image_views.size()));
#endif
    }
    return res == VK_SUCCESS;
}

void Ren::Framebuffer_Destroy(const ApiContext &api, FramebufferMain &fb_main, FramebufferCold &fb_cold) {
    if (fb_main.handle != VK_NULL_HANDLE) {
        api.framebuffers_to_destroy[api.backend_frame].push_back(fb_main.handle);
    }
    fb_main = {};
    fb_cold = {};
}

void Ren::Framebuffer_DestroyImmediately(const ApiContext &api, FramebufferMain &fb_main, FramebufferCold &fb_cold) {
    if (fb_main.handle != VK_NULL_HANDLE) {
        api.vkDestroyFramebuffer(api.device, fb_main.handle, nullptr);
    }
    fb_main = {};
    fb_cold = {};
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
