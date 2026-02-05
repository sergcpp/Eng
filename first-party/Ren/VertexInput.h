#pragma once

#include "Buffer.h"
#include "Span.h"

namespace Ren {
struct VtxAttribDesc {
    uint8_t buf : 4;
    uint8_t loc : 4;
    uint8_t size = 0;
    eType type = eType::Undefined;
    uint8_t stride = 0;
    uint32_t base_offset : 27;
    uint32_t rel_offset : 5;

    VtxAttribDesc() : buf(0), loc(0), base_offset(0), rel_offset(0) {}
    VtxAttribDesc(const int _buf, const int _loc, const uint8_t _size, const eType _type, const int _stride,
                  const uint32_t _base_offset, const uint32_t _rel_offset)
        : buf(_buf), loc(_loc), size(_size), type(_type), stride(_stride), base_offset(_base_offset),
          rel_offset(_rel_offset) {
        assert(_rel_offset < 32);
    }
};
static_assert(sizeof(VtxAttribDesc) == 8);

inline bool operator==(const VtxAttribDesc &lhs, const VtxAttribDesc &rhs) {
    return lhs.buf == rhs.buf && lhs.loc == rhs.loc && lhs.size == rhs.size && lhs.type == rhs.type &&
           lhs.stride == rhs.stride && lhs.base_offset == rhs.base_offset && lhs.rel_offset == rhs.rel_offset;
}
inline bool operator!=(const VtxAttribDesc &lhs, const VtxAttribDesc &rhs) {
    return lhs.buf != rhs.buf || lhs.loc != rhs.loc || lhs.size != rhs.size || lhs.type != rhs.type ||
           lhs.stride != rhs.stride || lhs.base_offset != rhs.base_offset || lhs.rel_offset != rhs.rel_offset;
}
inline bool operator<(const VtxAttribDesc &lhs, const VtxAttribDesc &rhs) {
    return std::tie(lhs.buf, lhs.loc, lhs.size, lhs.type, lhs.stride, lhs.base_offset, lhs.rel_offset) <
           std::tie(rhs.buf, rhs.loc, rhs.size, rhs.type, rhs.stride, rhs.base_offset, rhs.rel_offset);
}

struct VertexInputMain {
    SmallVector<VtxAttribDesc, 4> attribs;
#if defined(REN_GL_BACKEND)
    uint32_t gl_vao = 0;
#endif

    bool operator==(const VertexInputMain &rhs) const { return attribs == rhs.attribs; }
    bool operator!=(const VertexInputMain &rhs) const { return attribs != rhs.attribs; }
    bool operator<(const VertexInputMain &rhs) const { return attribs < rhs.attribs; }
};

struct VertexInputCold {
    // TODO:
};

bool VertexInput_Init(VertexInputMain &vtx_input, Span<const VtxAttribDesc> attribs);
void VertexInput_Destroy(VertexInputMain &vtx_input);

#if defined(REN_VK_BACKEND)
void VertexInput_BindBuffers(const ApiContext &api, const VertexInputMain &vtx_input,
                             const DualStorage<BufferMain, BufferCold> &buffers, Span<const BufferROHandle> attrib_bufs,
                             BufferROHandle elem_buf, VkCommandBuffer cmd_buf, uint32_t index_offset, int index_type);
void VertexInput_FillVKDescriptions(
    const VertexInputMain &vtx_input,
    SmallVectorImpl<VkVertexInputBindingDescription, aligned_allocator<VkVertexInputBindingDescription, 4>>
        &out_bindings,
    SmallVectorImpl<VkVertexInputAttributeDescription, aligned_allocator<VkVertexInputAttributeDescription, 4>>
        &out_attribs);
#elif defined(REN_GL_BACKEND)
void VertexInput_BindBuffers(const ApiContext &api, const VertexInputMain &vtx_input,
                             const DualStorage<BufferMain, BufferCold> &buffers, Span<const BufferROHandle> attrib_bufs,
                             BufferROHandle elem_buf);
#endif
} // namespace Ren