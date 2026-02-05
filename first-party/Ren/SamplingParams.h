#pragma once

#include <cassert>
#include <cstdint>
#undef Always

#include "Fixed.h"

namespace Ren {
using Fixed8 = Fixed<int8_t, 3>;

#define X(_0, _1, _2, _3, _4) _0,
enum class eFilter : uint8_t {
#include "Filter.inl"
};
#undef X

#define X(_0, _1, _2) _0,
enum class eWrap : uint8_t {
#include "Wrap.inl"
};
#undef X

#define X(_0, _1, _2) _0,
enum class eCompareOp : uint8_t {
#include "CompareOp.inl"
    _Count
};
#undef X

struct SamplingParams {
    eFilter filter = eFilter::Nearest;
    eWrap wrap = eWrap::Repeat;
    eCompareOp compare = eCompareOp::None;
    Fixed8 lod_bias;

    SamplingParams() = default;
    SamplingParams(const eFilter _filter, const eWrap _wrap, const eCompareOp _compare, const Fixed8 _lod_bias)
        : filter(_filter), wrap(_wrap), compare(_compare), lod_bias(_lod_bias) {}
};
static_assert(sizeof(SamplingParams) == 4);

inline bool operator==(const SamplingParams lhs, const SamplingParams rhs) {
    return lhs.filter == rhs.filter && lhs.wrap == rhs.wrap && lhs.compare == rhs.compare &&
           lhs.lod_bias == rhs.lod_bias;
}
inline bool operator!=(const SamplingParams lhs, const SamplingParams rhs) { return !operator==(lhs, rhs); }

inline bool operator<(const SamplingParams lhs, const SamplingParams rhs) {
    if (lhs.filter < rhs.filter) {
        return true;
    } else if (lhs.filter == rhs.filter) {
        if (lhs.wrap < rhs.wrap) {
            return true;
        } else if (lhs.wrap == rhs.wrap) {
            if (lhs.compare < rhs.compare) {
                return true;
            } else if (lhs.compare == rhs.compare) {
                return lhs.lod_bias.value() < rhs.lod_bias.value();
            }
        }
    }
    return false;
}

enum class eSamplerLoadStatus { Found, Created };
} // namespace Ren