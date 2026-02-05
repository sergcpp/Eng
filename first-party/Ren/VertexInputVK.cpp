#include "VertexInput.h"

#include "VKCtx.h"

namespace Ren {
const VkFormat g_attrib_formats_vk[][4] = {
    {}, // Undefined
    {VK_FORMAT_R16_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT,
     VK_FORMAT_R16G16B16A16_SFLOAT}, // Float16
    {VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT,
     VK_FORMAT_R32G32B32A32_SFLOAT},                                                                    // Float32
    {VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32A32_UINT}, // Uint32
    {VK_FORMAT_R16_UINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R16G16B16A16_UINT}, // Uint16
    {VK_FORMAT_R16_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16B16_UNORM,
     VK_FORMAT_R16G16B16A16_UNORM}, // Uint16UNorm
    {VK_FORMAT_R16_SNORM, VK_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16B16_SNORM,
     VK_FORMAT_R16G16B16A16_SNORM},                                                                     // Int16SNorm
    {VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8A8_UNORM},       // Uint8UNorm
    {VK_FORMAT_R32_SINT, VK_FORMAT_R32G32_SINT, VK_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32A32_SINT}, // Int32
};
static_assert(std::size(g_attrib_formats_vk) == int(eType::_Count));

extern const int g_type_sizes[];
} // namespace Ren

bool Ren::VertexInput_Init(VertexInputMain &vtx_input, Span<const VtxAttribDesc> _attribs) {
    vtx_input.attribs.assign(_attribs.begin(), _attribs.end());
    return true;
}

void Ren::VertexInput_Destroy(VertexInputMain &vtx_input) { vtx_input = {}; }

void Ren::VertexInput_BindBuffers(const ApiContext &api, const VertexInputMain &vtx_input,
                                  const DualStorage<BufferMain, BufferCold> &buffers,
                                  Span<const BufferROHandle> attrib_bufs, const BufferROHandle elem_buf,
                                  VkCommandBuffer cmd_buf, uint32_t index_offset, int index_type) {
    SmallVector<VkBuffer, 8> buffers_to_bind;
    SmallVector<VkDeviceSize, 8> buffer_offsets;
    for (const VtxAttribDesc &attr : vtx_input.attribs) {
        int bound_index = -1;
        for (int i = 0; i < int(buffers_to_bind.size()); ++i) {
            if (buffers_to_bind[i] == buffers.Get(attrib_bufs[attr.buf]).first.buf &&
                buffer_offsets[i] == attr.base_offset) {
                bound_index = i;
                break;
            }
        }
        if (bound_index == -1) {
            buffers_to_bind.push_back(buffers.Get(attrib_bufs[attr.buf]).first.buf);
            buffer_offsets.push_back(attr.base_offset);
        }
    }
    api.vkCmdBindVertexBuffers(cmd_buf, 0, buffers_to_bind.size(), buffers_to_bind.cdata(), buffer_offsets.cdata());
    if (elem_buf) {
        api.vkCmdBindIndexBuffer(cmd_buf, buffers.Get(elem_buf).first.buf, VkDeviceSize(index_offset),
                                 VkIndexType(index_type));
    }
}

void Ren::VertexInput_FillVKDescriptions(
    const VertexInputMain &vtx_input,
    SmallVectorImpl<VkVertexInputBindingDescription, aligned_allocator<VkVertexInputBindingDescription, 4>>
        &out_bindings,
    SmallVectorImpl<VkVertexInputAttributeDescription, aligned_allocator<VkVertexInputAttributeDescription, 4>>
        &out_attribs) {
    SmallVector<std::pair<int, VkDeviceSize>, 8> bound_buffers;
    for (const VtxAttribDesc &attr : vtx_input.attribs) {
        auto &vk_attr = out_attribs.emplace_back();
        vk_attr.location = uint32_t(attr.loc);
        vk_attr.format = g_attrib_formats_vk[int(attr.type)][attr.size - 1];
        vk_attr.offset = attr.rel_offset;
        vk_attr.binding = 0xffffffff;

        for (uint32_t i = 0; i < uint32_t(bound_buffers.size()); ++i) {
            if (bound_buffers[i].first == attr.buf && bound_buffers[i].second == attr.base_offset) {
                vk_attr.binding = i;
                break;
            }
        }
        if (vk_attr.binding == 0xffffffff) {
            vk_attr.binding = uint32_t(out_bindings.size());

            auto &vk_binding = out_bindings.emplace_back();
            vk_binding.binding = uint32_t(bound_buffers.size());
            if (attr.stride) {
                vk_binding.stride = uint32_t(attr.stride);
            } else {
                vk_binding.stride = uint32_t(g_type_sizes[int(attr.type)]) * attr.size;
            }
            vk_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            bound_buffers.emplace_back(attr.buf, attr.base_offset);
        }
    }
}
