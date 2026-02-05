#pragma once

#include <utility>
#include <vector>

#include "Bitmask.h"
#include "Fence.h"
#include "ImageParams.h"
#include "MemoryAllocator.h"
#include "Resource.h"
#include "SmallVector.h"
#include "Storage.h"
#include "String.h"

namespace Ren {
class ILog;
struct ApiContext;
#define X(_0) _0,
enum class eType : uint8_t {
#include "Types.inl"
    _Count
};
#undef X
enum class eBufType : uint8_t {
    Undefined,
    VertexAttribs,
    VertexIndices,
    Texture,
    Uniform,
    Storage,
    Upload,
    Readback,
    AccStructure,
    ShaderBinding,
    Indirect,
    _Count
};

std::string_view TypeName(eType type);
eType Type(std::string_view name);

struct SubAllocation {
    uint32_t offset = 0xffffffff;
    uint32_t block = 0xffffffff;

    operator bool() const { return offset != 0xffffffff; }
};

struct BufferMain {
#if defined(REN_VK_BACKEND)
    VkBuffer buf = {};
    SmallVector<std::pair<eFormat, VkBufferView>, 1> views;
#elif defined(REN_GL_BACKEND)
    uint32_t buf = 0;
    SmallVector<std::pair<eFormat, uint32_t>, 1> views;
#endif
    mutable eResState resource_state = eResState::Undefined;
};

struct BufferCold {
    String name;
    MemAllocation alloc;
    std::unique_ptr<FreelistAlloc> sub_alloc;
#if defined(REN_VK_BACKEND)
    MemAllocators *mem_allocs = nullptr;
    VkDeviceMemory dedicated_mem = {};
#endif
    eBufType type = eBufType::Undefined;
    uint32_t size = 0, size_alignment = 1;
    mutable uint8_t *mapped_ptr = nullptr;
    mutable uint32_t mapped_offset = 0xffffffff;
};

inline bool operator==(const BufferMain &lhs, const BufferMain &rhs) {
    return lhs.buf == rhs.buf && lhs.views == rhs.views;
}
inline bool operator!=(const BufferMain &lhs, const BufferMain &rhs) {
    return lhs.buf != rhs.buf || lhs.views != rhs.views;
}
inline bool operator<(const BufferMain &lhs, const BufferMain &rhs) {
    if (lhs.buf < rhs.buf) {
        return true;
    } else if (lhs.buf == rhs.buf) {
        return lhs.views < rhs.views;
    }
    return false;
}

bool Buffer_Init(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold, String name, eBufType type,
                 uint32_t initial_size, ILog *log, uint32_t size_alignment = 1, MemAllocators *mem_allocs = nullptr);
bool Buffer_Init(const ApiContext &api, BufferCold &buf_cold, String name, eBufType type, MemAllocation &&alloc,
                 uint32_t initial_size, ILog *log, uint32_t size_alignment = 1);
void Buffer_Destroy(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold);
void Buffer_DestroyImmediately(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold);

bool Buffer_Resize(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold, uint32_t new_size, ILog *log,
                   bool keep_content = true, bool release_immediately = false);
int Buffer_AddView(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold, eFormat format);

uint8_t *Buffer_MapRange(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold, uint32_t offset,
                         uint32_t size, bool persistent = false);
inline uint8_t *Buffer_Map(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold,
                           const bool persistent = false) {
    return Buffer_MapRange(api, buf_main, buf_cold, 0, buf_cold.size, persistent);
}
void Buffer_Unmap(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold);

SubAllocation Buffer_AllocSubRegion(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold,
                                    uint32_t req_size, uint32_t req_alignment, std::string_view tag, ILog *log,
                                    const BufferMain *init_buf = nullptr, CommandBuffer cmd_buf = {},
                                    uint32_t init_off = 0);
void Buffer_UpdateSubRegion(const ApiContext &api, BufferMain &buf_main, BufferCold &buf_cold, uint32_t offset,
                            uint32_t size, const BufferMain &init_buf, uint32_t init_off = 0,
                            CommandBuffer cmd_buf = {});
bool Buffer_FreeSubRegion(BufferCold &buf_cold, SubAllocation alloc);

void Buffer_Fill(const ApiContext &api, BufferMain &buf_main, uint32_t dst_offset, uint32_t size, uint32_t data,
                 CommandBuffer cmd_buf);
void Buffer_UpdateInPlace(const ApiContext &api, BufferMain &buf_main, uint32_t dst_offset, uint32_t size,
                          const void *data, CommandBuffer cmd_buf);

#if defined(REN_VK_BACKEND)
VkDeviceAddress Buffer_GetDeviceAddress(const ApiContext &api, const BufferMain &buf_main);
#endif

void CopyBufferToBuffer(const ApiContext &api, const BufferMain &src, uint32_t src_offset, BufferMain &dst,
                        uint32_t dst_offset, uint32_t size, CommandBuffer cmd_buf);
void CopyBufferToBuffer(const ApiContext &api, const StoragesRef &storages, BufferROHandle src, uint32_t src_offset,
                        BufferHandle dst, uint32_t dst_offset, uint32_t size, CommandBuffer cmd_buf);
// Update buffer using stage buffer
bool UpdateBuffer(const ApiContext &api, const StoragesRef &storages, BufferHandle dst, uint32_t dst_offset,
                  uint32_t data_size, const void *data, BufferHandle stage, uint32_t map_offset, uint32_t map_size,
                  CommandBuffer cmd_buf);

#if defined(REN_GL_BACKEND)
void GLUnbindBufferUnits(int start, int count);
#endif
} // namespace Ren