#pragma once

#include <cstdint>
#include <string_view>

#include "SamplingParams.h"

namespace Ren {
#define DECORATE(X) X,
enum class eTexFormat : uint8_t {
#include "TextureFormats.inl"
    _Count
};
#undef DECORATE

inline bool operator<(const eTexFormat lhs, const eTexFormat rhs) { return uint8_t(lhs) < uint8_t(rhs); }

std::string_view TexFormatName(eTexFormat format);
eTexFormat TexFormat(std::string_view name);

inline bool IsDepthFormat(const eTexFormat format) {
    return format == eTexFormat::D16 || format == eTexFormat::D24_S8 || format == eTexFormat::D32_S8 ||
           format == eTexFormat::D32;
}

inline bool IsDepthStencilFormat(const eTexFormat format) {
    return format == eTexFormat::D24_S8 || format == eTexFormat::D32_S8;
}

inline bool IsUnsignedIntegerFormat(const eTexFormat format) {
    return format == eTexFormat::R32UI || format == eTexFormat::RG32UI || format == eTexFormat::RGBA32UI;
}

bool IsCompressedFormat(const eTexFormat format);

int CalcMipCount(int w, int h, int min_res, eTexFilter filter);

#if defined(__ANDROID__)
const eTexFormat DefaultCompressedRGBA = eTexFormat::ASTC;
#else
const eTexFormat DefaultCompressedRGBA = eTexFormat::BC3;
#endif

enum class eTexBlock : uint8_t {
    _4x4,
    _5x4,
    _5x5,
    _6x5,
    _6x6,
    _8x5,
    _8x6,
    _8x8,
    _10x5,
    _10x6,
    _10x8,
    _10x10,
    _12x10,
    _12x12,
    _None
};

enum class eTexFlagBits : uint16_t {
    NoOwnership = (1u << 0u),
    Signed = (1u << 1u),
    SRGB = (1u << 2u),
    NoRepeat = (1u << 3u),
    NoFilter = (1u << 4u),
    MIPMin = (1u << 5u),
    MIPMax = (1u << 6u),
    NoBias = (1u << 7u),
    ExtendedViews = (1u << 8u)
};
using eTexFlags = eTexFlagBits;
inline eTexFlags operator|(eTexFlags a, eTexFlags b) { return eTexFlags(uint16_t(a) | uint16_t(b)); }
inline eTexFlags &operator|=(eTexFlags &a, eTexFlags b) { return a = eTexFlags(uint16_t(a) | uint16_t(b)); }
inline eTexFlags operator&(eTexFlags a, eTexFlags b) { return eTexFlags(uint16_t(a) & uint16_t(b)); }
inline eTexFlags &operator&=(eTexFlags &a, eTexFlags b) { return a = eTexFlags(uint16_t(a) & uint16_t(b)); }
inline eTexFlags operator~(eTexFlags a) { return eTexFlags(~uint16_t(a)); }

enum class eTexUsageBits : uint8_t {
    Transfer = (1u << 0u),
    Sampled = (1u << 1u),
    Storage = (1u << 2u),
    RenderTarget = (1u << 3u)
};
using eTexUsage = eTexUsageBits;

inline eTexUsage operator|(eTexUsage a, eTexUsage b) { return eTexUsage(uint8_t(a) | uint8_t(b)); }
inline eTexUsage &operator|=(eTexUsage &a, eTexUsage b) { return a = eTexUsage(uint8_t(a) | uint8_t(b)); }
inline eTexUsage operator&(eTexUsage a, eTexUsage b) { return eTexUsage(uint8_t(a) & uint8_t(b)); }
inline eTexUsage &operator&=(eTexUsage &a, eTexUsage b) { return a = eTexUsage(uint8_t(a) & uint8_t(b)); }

struct Texture1DParams {
    uint32_t offset = 0, size = 0;
    uint8_t _padding[3];
    eTexFormat format = eTexFormat::Undefined;
};
static_assert(sizeof(Texture1DParams) == 12, "!");

struct Tex2DParams {
    uint16_t w = 0, h = 0;
    eTexFlags flags = {};
    uint8_t mip_count = 1;
    eTexUsage usage = {};
    uint8_t cube = 0;
    uint8_t samples = 1;
    uint8_t fallback_color[4] = {0, 0, 0, 255};
    eTexFormat format = eTexFormat::Undefined;
    eTexBlock block = eTexBlock::_None;
    SamplingParams sampling;
};
static_assert(sizeof(Tex2DParams) == 22, "!");

inline bool operator==(const Tex2DParams &lhs, const Tex2DParams &rhs) {
    return lhs.w == rhs.w && lhs.h == rhs.h && lhs.flags == rhs.flags && lhs.mip_count == rhs.mip_count &&
           lhs.usage == rhs.usage && lhs.cube == rhs.cube && lhs.samples == rhs.samples &&
           lhs.fallback_color[0] == rhs.fallback_color[0] && lhs.fallback_color[1] == rhs.fallback_color[1] &&
           lhs.fallback_color[2] == rhs.fallback_color[2] && lhs.fallback_color[3] == rhs.fallback_color[3] &&
           lhs.format == rhs.format && lhs.sampling == rhs.sampling;
}
inline bool operator!=(const Tex2DParams &lhs, const Tex2DParams &rhs) { return !operator==(lhs, rhs); }

struct Tex3DParams {
    uint16_t w = 0, h = 0, d = 0;
    eTexFlags flags = {};
    eTexUsage usage = {};
    eTexFormat format = eTexFormat::Undefined;
    eTexBlock block = eTexBlock::_None;
    SamplingParams sampling;
};
static_assert(sizeof(Tex3DParams) == 18, "!");

inline bool operator==(const Tex3DParams &lhs, const Tex3DParams &rhs) {
    return lhs.w == rhs.w && lhs.h == rhs.h && lhs.d == rhs.d && lhs.flags == rhs.flags && lhs.usage == rhs.usage &&
           lhs.format == rhs.format && lhs.sampling == rhs.sampling;
}
inline bool operator!=(const Tex3DParams &lhs, const Tex3DParams &rhs) { return !operator==(lhs, rhs); }

int GetColorChannelCount(eTexFormat format);

enum class eTexLoadStatus { Found, Reinitialized, CreatedDefault, CreatedFromData, Error };

} // namespace Ren