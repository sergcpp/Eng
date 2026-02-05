#include "Renderer.h"

#include "Utils.h"

#include <cassert>
#include <fstream>

#include "../Ren/Context.h"
#include "../Ren/DescriptorPool.h"
#include "../Ren/VKCtx.h"

namespace Gui {
extern const int TexAtlasSlot;
} // namespace Gui

Gui::Renderer::Renderer(Ren::Context &ctx) : ctx_(ctx) { instance_index_ = g_instance_count++; }

Gui::Renderer::~Renderer() {
    const Ren::ApiContext &api = ctx_.api();

    // TODO: Remove this
    api.vkDeviceWaitIdle(api.device);

    const auto &[vtx_stage_main, vtx_stage_cold] = ctx_.buffers().Get(vertex_stage_buf_);
    Buffer_Unmap(api, vtx_stage_main, vtx_stage_cold);
    ctx_.ReleaseBuffer(vertex_stage_buf_, true /* immediately */);

    const auto &[ndx_stage_main, ndx_stage_cold] = ctx_.buffers().Get(index_stage_buf_);
    Buffer_Unmap(api, ndx_stage_main, ndx_stage_cold);
    ctx_.ReleaseBuffer(index_stage_buf_, true /* immediately */);

    ctx_.ReleaseBuffer(vertex_buf_, true /* immediately */);
    ctx_.ReleaseBuffer(index_buf_, true /* immediately */);

    ctx_.ReleaseRenderPass(render_pass_, true /* immediately */);

    for (const Ren::FramebufferHandle fb : framebuffers_) {
        ctx_.ReleaseFramebuffer(fb, true /* immediately */);
    }

    ctx_.ReleasePipeline(pipeline_, true /* immediately */);
    ctx_.ReleaseProgram(program_);
    ctx_.ReleaseShader(vs_);
    ctx_.ReleaseShader(fs_);
    ctx_.ReleaseVertexInput(vtx_input_);
}

void Gui::Renderer::Draw(const int w, const int h) {
    const Ren::ApiContext &api = ctx_.api();
    VkCommandBuffer cmd_buf = api.draw_cmd_buf[api.backend_frame];

    const int stage_frame = ctx_.in_flight_frontend_frame[api.backend_frame];
    if (!ndx_count_[stage_frame]) {
        // nothing to draw
        return;
    }

    { // insert memory barrier
        VkMemoryBarrier mem_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        mem_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        api.vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1,
                                 &mem_barrier, 0, nullptr, 0, nullptr);
    }

    VkDebugUtilsLabelEXT label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    const std::string label_name = name_ + "::Draw";
    label.pLabelName = label_name.c_str();
    label.color[0] = label.color[1] = label.color[2] = label.color[3] = 1;
    api.vkCmdBeginDebugUtilsLabelEXT(cmd_buf, &label);

    //
    // Update buffers
    //
    const uint32_t vtx_data_offset = uint32_t(stage_frame * MaxVerticesPerRange * sizeof(vertex_t));
    const uint32_t vtx_data_size = uint32_t(vtx_count_[stage_frame]) * sizeof(vertex_t);

    const uint32_t ndx_data_offset = uint32_t(stage_frame * MaxIndicesPerRange * sizeof(uint16_t));
    const uint32_t ndx_data_size = uint32_t(ndx_count_[stage_frame]) * sizeof(uint16_t);

    const auto &[vtx_main, vtx_cold] = ctx_.buffers().Get(vertex_buf_);
    const auto &[ndx_main, ndx_cold] = ctx_.buffers().Get(index_buf_);

    const auto &[vtx_stage_main, vtx_stage_cold] = ctx_.buffers().Get(vertex_stage_buf_);
    const auto &[ndx_stage_main, ndx_stage_cold] = ctx_.buffers().Get(index_stage_buf_);

    { // insert needed barriers before copying
        VkPipelineStageFlags src_stages = 0, dst_stages = 0;
        SmallVector<VkBufferMemoryBarrier, 2> buf_barriers;

        vtx_stage_main.resource_state = Ren::eResState::CopySrc;
        ndx_stage_main.resource_state = Ren::eResState::CopySrc;

        if (vtx_main.resource_state != Ren::eResState::Undefined &&
            vtx_main.resource_state != Ren::eResState::CopyDst) {
            auto &new_barrier = buf_barriers.emplace_back();
            new_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            new_barrier.srcAccessMask = VKAccessFlagsForState(vtx_main.resource_state);
            new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::CopyDst);
            new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.buffer = vtx_main.buf;
            new_barrier.offset = 0;
            new_barrier.size = VK_WHOLE_SIZE;

            src_stages |= VKPipelineStagesForState(vtx_main.resource_state);
            dst_stages |= VKPipelineStagesForState(Ren::eResState::CopyDst);
        }

        if (ndx_main.resource_state != Ren::eResState::Undefined &&
            ndx_main.resource_state != Ren::eResState::CopyDst) {
            auto &new_barrier = buf_barriers.emplace_back();
            new_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            new_barrier.srcAccessMask = VKAccessFlagsForState(ndx_main.resource_state);
            new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::CopyDst);
            new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.buffer = ndx_main.buf;
            new_barrier.offset = 0;
            new_barrier.size = VK_WHOLE_SIZE;

            src_stages |= VKPipelineStagesForState(ndx_main.resource_state);
            dst_stages |= VKPipelineStagesForState(Ren::eResState::CopyDst);
        }

        src_stages &= api.supported_stages_mask;
        dst_stages &= api.supported_stages_mask;

        if (!buf_barriers.empty()) {
            api.vkCmdPipelineBarrier(cmd_buf, src_stages, dst_stages, 0, 0, nullptr, buf_barriers.size(),
                                     buf_barriers.cdata(), 0, nullptr);
        }

        vtx_main.resource_state = Ren::eResState::CopyDst;
        ndx_main.resource_state = Ren::eResState::CopyDst;
    }

    { // copy vertex data
        assert(vtx_stage_main.resource_state == Ren::eResState::CopySrc);
        assert(vtx_main.resource_state == Ren::eResState::CopyDst);

        VkBufferCopy region_to_copy = {};
        region_to_copy.srcOffset = vtx_data_offset;
        region_to_copy.dstOffset = 0;
        region_to_copy.size = vtx_data_size;

        api.vkCmdCopyBuffer(cmd_buf, vtx_stage_main.buf, vtx_main.buf, 1, &region_to_copy);
    }

    { // copy index data
        assert(ndx_stage_main.resource_state == Ren::eResState::CopySrc);
        assert(ndx_main.resource_state == Ren::eResState::CopyDst);

        VkBufferCopy region_to_copy = {};
        region_to_copy.srcOffset = ndx_data_offset;
        region_to_copy.dstOffset = 0;
        region_to_copy.size = ndx_data_size;

        api.vkCmdCopyBuffer(cmd_buf, ndx_stage_main.buf, ndx_main.buf, 1, &region_to_copy);
    }

    auto &atlas = ctx_.image_atlas();

    //
    // Insert needed barriers before drawing
    //

    VkPipelineStageFlags src_stages = 0, dst_stages = 0;
    SmallVector<VkBufferMemoryBarrier, 4> buf_barriers;
    SmallVector<VkImageMemoryBarrier, 4> img_barriers;

    { // vertex buffer barrier [CopyDst -> VertexBuffer]
        auto &new_barrier = buf_barriers.emplace_back();
        new_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        new_barrier.srcAccessMask = VKAccessFlagsForState(Ren::eResState::CopyDst);
        new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::VertexBuffer);
        new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        new_barrier.buffer = vtx_main.buf;
        new_barrier.offset = 0;
        new_barrier.size = vtx_data_size;

        src_stages |= VKPipelineStagesForState(Ren::eResState::CopyDst);
        dst_stages |= VKPipelineStagesForState(Ren::eResState::VertexBuffer);
    }

    { // index buffer barrier [CopyDst -> IndexBuffer]
        auto &new_barrier = buf_barriers.emplace_back();
        new_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        new_barrier.srcAccessMask = VKAccessFlagsForState(Ren::eResState::CopyDst);
        new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::IndexBuffer);
        new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        new_barrier.buffer = ndx_main.buf;
        new_barrier.offset = 0;
        new_barrier.size = ndx_data_size;

        src_stages |= VKPipelineStagesForState(Ren::eResState::CopyDst);
        dst_stages |= VKPipelineStagesForState(Ren::eResState::IndexBuffer);
    }

    if (atlas.resource_state != Ren::eResState::ShaderResource) {
        auto &new_barrier = img_barriers.emplace_back();
        new_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        new_barrier.srcAccessMask = VKAccessFlagsForState(atlas.resource_state);
        new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::ShaderResource);
        new_barrier.oldLayout = VkImageLayout(VKImageLayoutForState(atlas.resource_state));
        new_barrier.newLayout = VkImageLayout(VKImageLayoutForState(Ren::eResState::ShaderResource));
        new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        new_barrier.image = atlas.img();
        new_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        new_barrier.subresourceRange.baseMipLevel = 0;
        new_barrier.subresourceRange.levelCount = atlas.mip_count();
        new_barrier.subresourceRange.baseArrayLayer = 0;
        new_barrier.subresourceRange.layerCount = atlas.layer_count();

        src_stages |= VKPipelineStagesForState(atlas.resource_state);
        dst_stages |= VKPipelineStagesForState(Ren::eResState::ShaderResource);
    }

    src_stages &= api.supported_stages_mask;
    dst_stages &= api.supported_stages_mask;

    if (!buf_barriers.empty() || !img_barriers.empty()) {
        api.vkCmdPipelineBarrier(cmd_buf, src_stages, dst_stages, 0, 0, nullptr, buf_barriers.size(),
                                 buf_barriers.cdata(), img_barriers.size(), img_barriers.cdata());

        vtx_main.resource_state = Ren::eResState::VertexBuffer;
        ndx_main.resource_state = Ren::eResState::IndexBuffer;

        atlas.resource_state = Ren::eResState::ShaderResource;
    }

    //
    // (Re)create framebuffers
    //
    framebuffers_.resize(api.present_images.size());

    const Ren::FramebufferAttachment attachments[] = {ctx_.backbuffer_img()};
    if (!framebuffers_[api.active_present_image]) {
        framebuffers_[api.active_present_image] = ctx_.CreateFramebuffer(render_pass_, {}, {}, attachments);
    } else {
        const auto &[fb_main, fb_cold] = ctx_.framebuffers().Get(framebuffers_[api.active_present_image]);
        if (!Framebuffer_Equals(fb_main, fb_cold, render_pass_, {}, {}, attachments)) {
            Framebuffer_Destroy(ctx_.api(), fb_main, fb_cold);
            Framebuffer_Init(ctx_.api(), fb_main, fb_cold, ctx_.storages(), render_pass_, {}, {}, attachments,
                             ctx_.log());
        }
    }

    //
    // Update descriptor set
    //

    const Ren::PipelineMain &pi_main = ctx_.pipelines().Get(pipeline_).first;
    const Ren::ProgramMain &pr_main = ctx_.programs().Get(pi_main.prog).first;

    VkDescriptorSetLayout descr_set_layout = pr_main.descr_set_layouts[0];
    Ren::DescrSizes descr_sizes;
    descr_sizes.img_sampler_count = 1;
    VkDescriptorSet descr_set = ctx_.default_descr_alloc().Alloc(descr_sizes, descr_set_layout);

    VkDescriptorImageInfo img_info = {};
    img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    img_info.imageView = ctx_.image_atlas().img_view();
    img_info.sampler = ctx_.image_atlas().sampler().first.handle;

    VkWriteDescriptorSet descr_write;
    descr_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descr_write.dstSet = descr_set;
    descr_write.dstBinding = TexAtlasSlot;
    descr_write.dstArrayElement = 0;
    descr_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descr_write.descriptorCount = 1;
    descr_write.pBufferInfo = nullptr;
    descr_write.pImageInfo = &img_info;
    descr_write.pTexelBufferView = nullptr;
    descr_write.pNext = nullptr;

    api.vkUpdateDescriptorSets(api.device, 1, &descr_write, 0, nullptr);

    //
    // Submit draw call
    //

    assert(vtx_main.resource_state == Ren::eResState::VertexBuffer);
    assert(ndx_main.resource_state == Ren::eResState::IndexBuffer);

    VkRenderPassBeginInfo render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render_pass_begin_info.renderPass = ctx_.render_passes().Get(render_pass_).first.handle;
    render_pass_begin_info.framebuffer = ctx_.framebuffers().Get(framebuffers_[api.active_present_image]).first.handle;
    render_pass_begin_info.renderArea = {{0, 0}, {uint32_t(w), uint32_t(h)}};

    api.vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_main.pipeline);

    const VkViewport viewport = {0, 0, float(w), float(h), 0, 1};
    api.vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    const VkRect2D scissor = {{0, 0}, {uint32_t(w), uint32_t(h)}};
    api.vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_main.layout, 0, 1, &descr_set, 0, nullptr);

    VkBuffer vtx_buf = vtx_main.buf;

    VkDeviceSize offset = {};
    api.vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vtx_buf, &offset);
    api.vkCmdBindIndexBuffer(cmd_buf, ndx_main.buf, 0, VK_INDEX_TYPE_UINT16);

    api.vkCmdDrawIndexed(cmd_buf,
                         ndx_count_[stage_frame], // index count
                         1,                       // instance count
                         0,                       // first index
                         0,                       // vertex offset
                         0);                      // first instance

    api.vkCmdEndRenderPass(cmd_buf);

    api.vkCmdEndDebugUtilsLabelEXT(cmd_buf);

    vtx_count_[stage_frame] = ndx_count_[stage_frame] = 0;
}
