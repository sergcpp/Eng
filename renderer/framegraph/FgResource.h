#pragma once

#include <cstdint>

#include <Ren/Resource.h>

namespace Eng {
enum class eFgResType : uint8_t { Undefined, Buffer, Image };
enum class eFgQueueType : uint8_t { Graphics, Compute, Transfer };

struct FgAllocBufMain;
using FgBufROHandle = Ren::Handle<FgAllocBufMain, Ren::ROTag>;
using FgBufRWHandle = Ren::Handle<FgAllocBufMain, Ren::RWTag>;

struct FgAllocImgMain;
using FgImgROHandle = Ren::Handle<FgAllocImgMain, Ren::ROTag>;
using FgImgRWHandle = Ren::Handle<FgAllocImgMain, Ren::RWTag>;

struct FgBufDesc;
struct FgImgDesc;

struct FgResource {
    eFgResType type = eFgResType::Undefined;
    Ren::eResState desired_state = Ren::eResState::Undefined;
    Ren::Bitmask<Ren::eStage> stages;
    uint64_t opaque_handle = Ren::InvalidHandle;
    FgResource *next_use = nullptr;

    FgResource() = default;
    FgResource(const eFgResType _type, const Ren::eResState _desired_state, const uint64_t _opaque_handle,
               const Ren::Bitmask<Ren::eStage> _stages)
        : type(_type), desired_state(_desired_state), stages(_stages), opaque_handle(_opaque_handle) {}

    operator bool() const { return type != eFgResType::Undefined; }

    static bool LessThanTypeAndIndex(const FgResource &lhs, const FgResource &rhs) {
        if (lhs.type < rhs.type) {
            return true;
        } else if (lhs.type == rhs.type) {
            return (lhs.opaque_handle >> 32) < (rhs.opaque_handle >> 32);
        }
        return false;
    }
};
static_assert(sizeof(FgResource) == 24);

struct FgResRef {
    eFgResType type = eFgResType::Undefined;
    uint8_t _pad[3] = {};
    uint32_t index = 0xffffffff;

    operator bool() const { return type != eFgResType::Undefined; }

    FgResRef() = default;
    FgResRef(const FgResource &res) : type(res.type), index(res.opaque_handle >> 32) {}

    bool operator<(const FgResRef rhs) const {
        if (type < rhs.type) {
            return true;
        } else if (type == rhs.type) {
            return index < rhs.index;
        }
        return false;
    }
};
static_assert(sizeof(FgResRef) == 8);
} // namespace Eng
