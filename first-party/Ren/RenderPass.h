#pragma once

#include "Fwd.h"
#include "Image.h"
#include "Span.h"

namespace Ren {
struct ApiContext;

enum class eImageLayout : uint8_t {
    Undefined,
    General,
    ColorAttachmentOptimal,
    DepthStencilAttachmentOptimal,
    DepthStencilReadOnlyOptimal,
    ShaderReadOnlyOptimal,
    TransferSrcOptimal,
    TransferDstOptimal,
    _Count
};

inline bool operator<(const eImageLayout lhs, const eImageLayout rhs) { return uint8_t(lhs) < uint8_t(rhs); }

enum class eLoadOp : uint8_t { Load, Clear, DontCare, None, _Count };
enum class eStoreOp : uint8_t { Store, DontCare, None, _Count };

std::string_view LoadOpName(eLoadOp op);
eLoadOp LoadOp(std::string_view name);

std::string_view StoreOpName(eStoreOp op);
eStoreOp StoreOp(std::string_view name);

struct RenderTarget {
    WeakImgRef ref;
    uint8_t view_index = 0;
    eLoadOp load = eLoadOp::DontCare;
    eStoreOp store = eStoreOp::DontCare;
    eLoadOp stencil_load = eLoadOp::DontCare;
    eStoreOp stencil_store = eStoreOp::DontCare;

    RenderTarget() = default;
    RenderTarget(WeakImgRef _ref, eLoadOp _load, eStoreOp _store, eLoadOp _stencil_load = eLoadOp::DontCare,
                 eStoreOp _stencil_store = eStoreOp::DontCare)
        : ref(_ref), load(_load), store(_store), stencil_load(_stencil_load), stencil_store(_stencil_store) {}
    RenderTarget(WeakImgRef _ref, uint8_t _view_index, eLoadOp _load, eStoreOp _store,
                 eLoadOp _stencil_load = eLoadOp::DontCare, eStoreOp _stencil_store = eStoreOp::DontCare)
        : ref(_ref), view_index(_view_index), load(_load), store(_store), stencil_load(_stencil_load),
          stencil_store(_stencil_store) {}

    explicit operator bool() const { return bool(ref); }
};

inline bool operator==(const RenderTarget &lhs, const RenderTarget &rhs) {
    return lhs.ref == rhs.ref && lhs.view_index == rhs.view_index && lhs.load == rhs.load && lhs.store == rhs.store &&
           lhs.stencil_load == rhs.stencil_load && lhs.stencil_store == rhs.stencil_store;
}

inline bool operator==(const eFormat lhs, const RenderTarget &rhs) {
    if (!rhs) {
        return lhs == eFormat::Undefined;
    }
    return lhs == rhs.ref->params.format;
}
inline bool operator==(const RenderTarget &lhs, const eFormat rhs) {
    if (!lhs) {
        return eFormat::Undefined == rhs;
    }
    return lhs.ref->params.format == rhs;
}
inline bool operator!=(const eFormat lhs, const RenderTarget &rhs) {
    if (!rhs) {
        return lhs != eFormat::Undefined;
    }
    return lhs != rhs.ref->params.format;
}
inline bool operator!=(const RenderTarget &lhs, const eFormat rhs) {
    if (!lhs) {
        return eFormat::Undefined != rhs;
    }
    return lhs.ref->params.format != rhs;
}
inline bool operator<(const eFormat lhs, const RenderTarget &rhs) {
    if (!rhs) {
        return lhs < eFormat::Undefined;
    }
    return lhs < rhs.ref->params.format;
}
inline bool operator<(const RenderTarget &lhs, const eFormat rhs) {
    if (!lhs) {
        return eFormat::Undefined < rhs;
    }
    return lhs.ref->params.format < rhs;
}

struct RenderTargetInfo {
    eFormat format = eFormat::Undefined;
    uint8_t samples = 1;
    Bitmask<eImgFlags> flags;
    eImageLayout layout = eImageLayout::Undefined;
    eLoadOp load = eLoadOp::DontCare;
    eStoreOp store = eStoreOp::DontCare;
    eLoadOp stencil_load = eLoadOp::DontCare;
    eStoreOp stencil_store = eStoreOp::DontCare;

    RenderTargetInfo() = default;
    RenderTargetInfo(const Image *tex, eLoadOp _load, eStoreOp _store, eLoadOp _stencil_load = eLoadOp::DontCare,
                     eStoreOp _stencil_store = eStoreOp::DontCare)
        : format(tex->params.format), samples(tex->params.samples), flags(tex->params.flags),
#if defined(REN_VK_BACKEND)
          layout(eImageLayout(VKImageLayoutForState(tex->resource_state))),
#endif
          load(_load), store(_store), stencil_load(_stencil_load), stencil_store(_stencil_store) {
    }
    RenderTargetInfo(eFormat _format, uint8_t _samples, eImageLayout _layout, eLoadOp _load, eStoreOp _store,
                     eLoadOp _stencil_load = eLoadOp::DontCare, eStoreOp _stencil_store = eStoreOp::DontCare)
        : format(_format), samples(_samples), layout(_layout), load(_load), store(_store), stencil_load(_stencil_load),
          stencil_store(_stencil_store) {}
    explicit RenderTargetInfo(const RenderTarget &rt) {
        if (rt.ref) {
            format = rt.ref->params.format;
            samples = rt.ref->params.samples;
            flags = Bitmask<eImgFlags>{rt.ref->params.flags};
#if defined(REN_VK_BACKEND)
            layout = eImageLayout(VKImageLayoutForState(rt.ref->resource_state));
#endif
            load = rt.load;
            store = rt.store;
            stencil_load = rt.stencil_load;
            stencil_store = rt.stencil_store;
        }
    }

    bool operator==(const RenderTargetInfo &rhs) const {
        return format == rhs.format && samples == rhs.samples &&
#if defined(REN_VK_BACKEND)
               layout == rhs.layout &&
#endif
               load == rhs.load && store == rhs.store && stencil_load == rhs.stencil_load &&
               stencil_store == rhs.stencil_store;
    }
    bool operator!=(const RenderTargetInfo &rhs) const {
        return format != rhs.format || samples != rhs.samples ||
#if defined(REN_VK_BACKEND)
               layout != rhs.layout ||
#endif
               load != rhs.load || store != rhs.store || stencil_load != rhs.stencil_load ||
               stencil_store != rhs.stencil_store;
    }
    bool operator<(const RenderTargetInfo &rhs) const {
#if defined(REN_VK_BACKEND)
        const eImageLayout rhs_layout = rhs.layout;
#else
        const eImageLayout rhs_layout = layout;
#endif
        return std::tie(format, samples, layout, load, store, stencil_load, stencil_store) <
               std::tie(rhs.format, rhs.samples, rhs_layout, rhs.load, rhs.store, rhs.stencil_load, rhs.stencil_store);
    }

    operator bool() const { return format != eFormat::Undefined; }
};

inline bool operator==(const RenderTargetInfo &lhs, const RenderTarget &rhs) {
    if (!rhs.ref) {
        return lhs.format == eFormat::Undefined;
    }
    const auto &p = rhs.ref->params;
    return lhs.format == p.format && lhs.samples == p.samples &&
#if defined(REN_VK_BACKEND)
           lhs.layout == eImageLayout(VKImageLayoutForState(rhs.ref->resource_state)) &&
#endif
           lhs.load == rhs.load && lhs.store == rhs.store && lhs.stencil_load == rhs.stencil_load &&
           lhs.stencil_store == rhs.stencil_store;
}
inline bool operator!=(const RenderTargetInfo &lhs, const RenderTarget &rhs) { return !operator==(lhs, rhs); }
inline bool operator<(const RenderTargetInfo &lhs, const RenderTarget &rhs) {
    if (!rhs.ref) {
        return lhs.format < eFormat::Undefined;
    }
    const auto &p = rhs.ref->params;
#if defined(REN_VK_BACKEND)
    const eImageLayout rhs_layout = eImageLayout(VKImageLayoutForState(rhs.ref->resource_state));
#else
    const eImageLayout rhs_layout = lhs.layout;
#endif
    return std::tie(lhs.format, lhs.samples, lhs.layout, lhs.load, lhs.store, lhs.stencil_load, lhs.stencil_store) <
           std::tie(p.format, p.samples, rhs_layout, rhs.load, rhs.store, rhs.stencil_load, rhs.stencil_store);
}
inline bool operator<(const RenderTarget &lhs, const RenderTargetInfo &rhs) {
    if (!lhs.ref) {
        return eFormat::Undefined < rhs.format;
    }
    const auto &p = lhs.ref->params;
#if defined(REN_VK_BACKEND)
    const eImageLayout lhs_layout = eImageLayout(VKImageLayoutForState(lhs.ref->resource_state));
#else
    const eImageLayout lhs_layout = rhs.layout;
#endif
    return std::tie(p.format, p.samples, lhs_layout, lhs.load, lhs.store, lhs.stencil_load, lhs.stencil_store) <
           std::tie(rhs.format, rhs.samples, rhs.layout, rhs.load, rhs.store, rhs.stencil_load, rhs.stencil_store);
}

inline bool operator==(const eFormat lhs, const RenderTargetInfo &rhs) { return lhs == rhs.format; }
inline bool operator==(const RenderTargetInfo &lhs, const eFormat rhs) { return lhs.format == rhs; }
inline bool operator!=(const eFormat lhs, const RenderTargetInfo &rhs) { return lhs != rhs.format; }
inline bool operator!=(const RenderTargetInfo &lhs, const eFormat rhs) { return lhs.format != rhs; }
inline bool operator<(const eFormat lhs, const RenderTargetInfo &rhs) { return lhs < rhs.format; }
inline bool operator<(const RenderTargetInfo &lhs, const eFormat rhs) { return lhs.format < rhs; }

struct RenderPassMain {
#if defined(REN_VK_BACKEND)
    VkRenderPass handle = {};
#endif
    RenderTargetInfo depth_rt;
    SmallVector<RenderTargetInfo, 4> color_rts;

    bool operator==(const RenderPassMain &rhs) const { return Equals(rhs.depth_rt, rhs.color_rts); }
    bool operator!=(const RenderPassMain &rhs) const { return depth_rt != rhs.depth_rt || color_rts != rhs.color_rts; }
    bool operator<(const RenderPassMain &rhs) const { return LessThan(rhs.depth_rt, rhs.color_rts); }

    bool IsCompatibleWith(const RenderTarget &_depth_rt, Span<const RenderTarget> _color_rts) const {
        return depth_rt == _depth_rt && Span<const RenderTargetInfo>(color_rts) == _color_rts;
    }

    bool Equals(const RenderTargetInfo &_depth_rt, Span<const RenderTargetInfo> _color_rts) const {
        return depth_rt == _depth_rt && Span<const RenderTargetInfo>(color_rts) == _color_rts;
    }

    bool LessThan(const RenderTargetInfo &_depth_rt, Span<const RenderTargetInfo> _color_rts) const {
        if (depth_rt < _depth_rt) {
            return true;
        } else if (depth_rt == _depth_rt) {
            return Span<const RenderTargetInfo>(color_rts) < _color_rts;
        }
        return false;
    }
};

struct RenderPassCold {
    // TODO:
};

bool RenderPass_Init(const ApiContext &api, RenderPassMain &rp_main, const RenderTargetInfo &depth_rt,
                     Span<const RenderTargetInfo> color_rts, ILog *log);
void RenderPass_Destroy(const ApiContext &api, RenderPassMain &rp_main);
} // namespace Ren
