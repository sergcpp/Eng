#pragma once

#include <cstdint>

#include "Image.h"
#include "ImageParams.h"
#include "RenderPass.h"
#include "SmallVector.h"
#include "Span.h"

namespace Ren {
struct FramebufferAttachment {
    ImageRWHandle img;
    uint8_t view_index = 0;

    FramebufferAttachment() = default;
    FramebufferAttachment(const ImageRWHandle _img, const uint8_t _view_index = 0)
        : img(_img), view_index(_view_index) {}
    FramebufferAttachment(const RenderTarget &rt) : img(rt.img), view_index(rt.view_index) {}

    operator bool() const { return bool(img); }
};

inline bool operator==(const FramebufferAttachment &lhs, const FramebufferAttachment &rhs) {
    return lhs.img == rhs.img && lhs.view_index == rhs.view_index;
}

inline bool operator!=(const FramebufferAttachment &lhs, const FramebufferAttachment &rhs) {
    return lhs.img != rhs.img || lhs.view_index != rhs.view_index;
}

inline bool operator<(const FramebufferAttachment &lhs, const FramebufferAttachment &rhs) {
    if (lhs.img < rhs.img) {
        return true;
    } else if (lhs.img == rhs.img) {
        return lhs.view_index < rhs.view_index;
    }
    return false;
}

inline bool operator==(const FramebufferAttachment &lhs, const RenderTarget &rhs) {
    return lhs.img == rhs.img && lhs.view_index == rhs.view_index;
}

inline bool operator!=(const FramebufferAttachment &lhs, const RenderTarget &rhs) {
    return lhs.img != rhs.img || lhs.view_index != rhs.view_index;
}

inline bool operator<(const FramebufferAttachment &lhs, const RenderTarget &rhs) {
    if (lhs.img < rhs.img) {
        return true;
    } else if (lhs.img == rhs.img) {
        return lhs.view_index < rhs.view_index;
    }
    return false;
}

inline bool operator<(const RenderTarget &lhs, const FramebufferAttachment &rhs) {
    if (lhs.img < rhs.img) {
        return true;
    } else if (lhs.img == rhs.img) {
        return lhs.view_index < rhs.view_index;
    }
    return false;
}

struct FramebufferMain {
#if defined(REN_VK_BACKEND)
    VkFramebuffer handle = {};
#elif defined(REN_GL_BACKEND)
    uint32_t id = 0;
#endif
    RenderPassROHandle renderpass;
    uint16_t w = 0xffffu, h = 0xffffu;
};

struct FramebufferCold {
    SmallVector<FramebufferAttachment, 4> color_attachments;
    FramebufferAttachment depth_attachment, stencil_attachment;
};

bool Framebuffer_Init(const ApiContext &api, FramebufferMain &fb_main, FramebufferCold &fb_cold,
                      const StoragesRef &storages, RenderPassROHandle render_pass, const FramebufferAttachment &depth,
                      const FramebufferAttachment &stencil, Span<const FramebufferAttachment> color_attachments,
                      ILog *log);
void Framebuffer_Destroy(const ApiContext &api, FramebufferMain &fb_main, FramebufferCold &fb_cold);
void Framebuffer_DestroyImmediately(const ApiContext &api, FramebufferMain &fb_main, FramebufferCold &fb_cold);

inline bool Framebuffer_Equals(const FramebufferMain &fb_main, const FramebufferCold &fb_cold,
                               RenderPassROHandle render_pass, const FramebufferAttachment &depth_attachment,
                               const FramebufferAttachment &stencil_attachment,
                               Span<const FramebufferAttachment> color_attachments) {
    return fb_main.renderpass == render_pass && fb_cold.depth_attachment == depth_attachment &&
           fb_cold.stencil_attachment == stencil_attachment &&
           Span<const FramebufferAttachment>(fb_cold.color_attachments) == color_attachments;
}
inline bool Framebuffer_Equals(const FramebufferMain &fb_main, const FramebufferCold &fb_cold,
                               RenderPassROHandle render_pass, const RenderTarget &depth_attachment,
                               const RenderTarget &stencil_attachment, Span<const RenderTarget> color_attachments) {
    return fb_main.renderpass == render_pass && fb_cold.depth_attachment == depth_attachment &&
           fb_cold.stencil_attachment == stencil_attachment &&
           Span<const FramebufferAttachment>(fb_cold.color_attachments) == color_attachments;
}

bool Framebuffer_LessThan(const FramebufferMain &fb_main, const FramebufferCold &fb_cold,
                          RenderPassROHandle render_pass, const FramebufferAttachment &depth_attachment,
                          const FramebufferAttachment &stencil_attachment,
                          Span<const FramebufferAttachment> color_attachments);
bool Framebuffer_LessThan(const FramebufferMain &fb_main, const FramebufferCold &fb_cold,
                          RenderPassROHandle render_pass, const RenderTarget &depth_attachment,
                          const RenderTarget &stencil_attachment, Span<const RenderTarget> color_attachments);

} // namespace Ren