#pragma once

#include "Fwd.h"
#include "Image.h"
#include "utils/Span.h"

namespace Ren {
struct ApiContext;

enum class eLoadOp : uint8_t { Load, Clear, DontCare, None, _Count };
enum class eStoreOp : uint8_t { Store, DontCare, None, _Count };

std::string_view LoadOpName(eLoadOp op);
eLoadOp LoadOp(std::string_view name);

std::string_view StoreOpName(eStoreOp op);
eStoreOp StoreOp(std::string_view name);

struct RenderTarget {
    ImageRWHandle img;
    uint8_t view_index = 0;
    eLoadOp load = eLoadOp::DontCare;
    eStoreOp store = eStoreOp::DontCare;
    eLoadOp stencil_load = eLoadOp::DontCare;
    eStoreOp stencil_store = eStoreOp::DontCare;

    RenderTarget() = default;
    RenderTarget(ImageRWHandle _img, eLoadOp _load, eStoreOp _store, eLoadOp _stencil_load = eLoadOp::DontCare,
                 eStoreOp _stencil_store = eStoreOp::DontCare)
        : img(_img), load(_load), store(_store), stencil_load(_stencil_load), stencil_store(_stencil_store) {}
    RenderTarget(ImageRWHandle _img, uint8_t _view_index, eLoadOp _load, eStoreOp _store,
                 eLoadOp _stencil_load = eLoadOp::DontCare, eStoreOp _stencil_store = eStoreOp::DontCare)
        : img(_img), view_index(_view_index), load(_load), store(_store), stencil_load(_stencil_load),
          stencil_store(_stencil_store) {}

    explicit operator bool() const { return bool(img); }
};

inline bool operator==(const RenderTarget &lhs, const RenderTarget &rhs) {
    return lhs.img == rhs.img && lhs.view_index == rhs.view_index && lhs.load == rhs.load && lhs.store == rhs.store &&
           lhs.stencil_load == rhs.stencil_load && lhs.stencil_store == rhs.stencil_store;
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
    RenderTargetInfo(eFormat _format, uint8_t _samples, eImageLayout _layout, eLoadOp _load, eStoreOp _store,
                     eLoadOp _stencil_load = eLoadOp::DontCare, eStoreOp _stencil_store = eStoreOp::DontCare)
        : format(_format), samples(_samples), layout(_layout), load(_load), store(_store), stencil_load(_stencil_load),
          stencil_store(_stencil_store) {}

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
void RenderPass_DestroyImmediately(const ApiContext &api, RenderPassMain &rp_main);
} // namespace Ren
