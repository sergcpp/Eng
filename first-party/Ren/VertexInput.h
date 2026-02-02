#pragma once

#include "Buffer.h"
#include "Span.h"

namespace Ren {
struct VtxAttribDesc {
    BufferROHandle buf;
    uint8_t loc = 0;
    uint8_t size = 0;
    eType type = eType::Undefined;
    uint8_t stride = 0;
    uint32_t offset = 0;

    VtxAttribDesc() = default;
    VtxAttribDesc(const BufferROHandle _buf, const int _loc, const uint8_t _size, const eType _type, const int _stride,
                  const uint32_t _offset)
        : buf(_buf), loc(_loc), size(_size), type(_type), stride(_stride), offset(_offset) {}
};
static_assert(sizeof(VtxAttribDesc) == 16);

inline bool operator==(const VtxAttribDesc &lhs, const VtxAttribDesc &rhs) {
    return lhs.buf == rhs.buf && lhs.loc == rhs.loc && lhs.size == rhs.size && lhs.type == rhs.type &&
           lhs.stride == rhs.stride && lhs.offset == rhs.offset;
}
inline bool operator!=(const VtxAttribDesc &lhs, const VtxAttribDesc &rhs) {
    return lhs.buf != rhs.buf || lhs.loc != rhs.loc || lhs.size != rhs.size || lhs.type != rhs.type ||
           lhs.stride != rhs.stride || lhs.offset != rhs.offset;
}
inline bool operator<(const VtxAttribDesc &lhs, const VtxAttribDesc &rhs) {
    return std::tie(lhs.buf, lhs.loc, lhs.size, lhs.type, lhs.stride, lhs.offset) <
           std::tie(rhs.buf, rhs.loc, rhs.size, rhs.type, rhs.stride, rhs.offset);
}

struct VertexInputMain {
#if defined(REN_GL_BACKEND)
    mutable uint32_t gl_vao = 0;
    mutable SmallVector<std::pair<uint32_t, uint32_t>, 4> cached_attribs_buf;
    mutable std::pair<uint32_t, uint32_t> cached_elem_buf;
#endif
    SmallVector<VtxAttribDesc, 4> attribs;
    BufferROHandle elem_buf;

    bool operator==(const VertexInputMain &rhs) const { return elem_buf == rhs.elem_buf && attribs == rhs.attribs; }
    bool operator!=(const VertexInputMain &rhs) const { return elem_buf != rhs.elem_buf || attribs != rhs.attribs; }
    bool operator<(const VertexInputMain &rhs) const {
        if (elem_buf < rhs.elem_buf) {
            return true;
        } else if (elem_buf == rhs.elem_buf) {
            return attribs < rhs.attribs;
        }
        return false;
    }
};

struct VertexInputCold {
    // TODO:
};

bool VertexInput_Init(VertexInputMain &vtx_input, Span<const VtxAttribDesc> attribs, BufferROHandle elem_buf);
void VertexInput_Destroy(VertexInputMain &vtx_input);

#if defined(REN_GL_BACKEND)
uint32_t VertexInput_GetVAO(const VertexInputMain &vtx_input, const DualStorage<BufferMain, BufferCold> &buffers);
#elif defined(REN_VK_BACKEND)
void VertexInput_BindBuffers(const ApiContext &api, const VertexInputMain &vtx_input,
                             const DualStorage<BufferMain, BufferCold> &buffers, VkCommandBuffer cmd_buf,
                             uint32_t index_offset, int index_type);
void VertexInput_FillVKDescriptions(
    const VertexInputMain &vtx_input, const DualStorage<BufferMain, BufferCold> &buffers,
    SmallVectorImpl<VkVertexInputBindingDescription, aligned_allocator<VkVertexInputBindingDescription, 4>>
        &out_bindings,
    SmallVectorImpl<VkVertexInputAttributeDescription, aligned_allocator<VkVertexInputAttributeDescription, 4>>
        &out_attribs);
#endif
} // namespace Ren