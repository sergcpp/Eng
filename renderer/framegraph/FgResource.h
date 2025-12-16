#pragma once

#include <cstdint>

#include <Ren/Resource.h>

namespace Eng {
enum class eFgResType : uint8_t { Undefined, Buffer, Image };

struct FgResource {
    eFgResType type = eFgResType::Undefined;
    Ren::eResState desired_state = Ren::eResState::Undefined;
    union {
        struct {
            uint8_t read_count;
            uint8_t write_count;
        };
        uint16_t _generation = 0;
    };
    uint16_t index = 0xffff;
    Ren::Bitmask<Ren::eStage> stages;
    uint64_t opaque_handle = Ren::InvalidHandle;
    FgResource *next_use = nullptr;

    FgResource() = default;
    FgResource(const eFgResType _type, const uint16_t __generation, const Ren::eResState _desired_state,
               const uint16_t _index, const Ren::Bitmask<Ren::eStage> _stages)
        : type(_type), desired_state(_desired_state), _generation(__generation), index(_index), stages(_stages) {}
    FgResource(const eFgResType _type, const uint16_t __generation, const Ren::eResState _desired_state,
               const uint64_t _opaque_handle, const Ren::Bitmask<Ren::eStage> _stages)
        : type(_type), desired_state(_desired_state), _generation(__generation), stages(_stages),
          opaque_handle(_opaque_handle) {}

    operator bool() const { return type != eFgResType::Undefined; }

    static bool LessThanTypeAndIndex(const FgResource &lhs, const FgResource &rhs) {
        if ((lhs.opaque_handle != Ren::InvalidHandle) < (rhs.opaque_handle != Ren::InvalidHandle)) {
            return true;
        } else if ((rhs.opaque_handle != Ren::InvalidHandle) < (lhs.opaque_handle != Ren::InvalidHandle)) {
            return false;
        }
        if (lhs.opaque_handle != Ren::InvalidHandle) {
            assert(rhs.opaque_handle != Ren::InvalidHandle);
            if (lhs.type != rhs.type) {
                return lhs.type < rhs.type;
            }
            return lhs.opaque_handle < rhs.opaque_handle;
        }
        if (lhs.type != rhs.type) {
            return lhs.type < rhs.type;
        }
        return lhs.index < rhs.index;
    }
};
static_assert(sizeof(FgResource) == 16 + 8);

struct FgResRef {
    eFgResType type = eFgResType::Undefined;
    uint8_t _pad = 0;
    union {
        struct {
            uint8_t read_count;
            uint8_t write_count;
        };
        uint16_t _generation = 0;
    };
    uint16_t index = 0xffff;
    uint64_t opaque_handle = Ren::InvalidHandle;

    operator bool() const { return type != eFgResType::Undefined; }

    FgResRef() = default;
    FgResRef(const FgResource &res)
        : type(res.type), _generation(res._generation), index(res.index), opaque_handle(res.opaque_handle) {}

    bool operator<(const FgResRef rhs) const {
        if (type != rhs.type) {
            return type < rhs.type;
        }
        return index < rhs.index;
    }
};
static_assert(sizeof(FgResRef) == 16);

struct FgBufDesc {
    Ren::eBufType type;
    uint32_t size;
    Ren::SmallVector<Ren::eFormat, 1> views;
};

struct FgImageViewDesc {
    Ren::eFormat format;
    int mip_level;
    int mip_count;
    int base_layer;
    int layer_count;
};

struct FgImgDesc : Ren::ImgParams {
    Ren::SmallVector<FgImageViewDesc, 1> views;
};

inline bool operator==(const FgBufDesc &lhs, const FgBufDesc &rhs) {
    return lhs.size == rhs.size && lhs.type == rhs.type;
}
} // namespace Eng
