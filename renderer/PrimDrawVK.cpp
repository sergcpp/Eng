#include "PrimDraw.h"

#include <Ren/Context.h>
#include <Ren/Framebuffer.h>
#include <Ren/ProbeStorage.h>
#include <Ren/VKCtx.h>

namespace Ren {
extern const VkAttachmentLoadOp vk_load_ops[];
extern const VkAttachmentStoreOp vk_store_ops[];
} // namespace Ren

namespace PrimDrawInternal {
extern const int SphereIndicesCount;
} // namespace PrimDrawInternal

void Eng::PrimDraw::DrawPrim(Ren::CommandBuffer cmd_buf, const ePrim prim, const Ren::ProgramHandle p,
                             Ren::RenderTarget depth_rt, Ren::Span<const Ren::RenderTarget> color_rts,
                             const Ren::RastState &new_rast_state, Ren::RastState &applied_rast_state,
                             Ren::Span<const Ren::Binding> bindings, const void *uniform_data,
                             const int uniform_data_len, const int uniform_data_offset, const int instances) {
    using namespace PrimDrawInternal;

    const Ren::ApiContext &api = ctx_->api();
    const Ren::StoragesRef &storages = ctx_->storages();

    const Ren::ProgramMain &p_main = storages.programs.Get(p).first;

    VkDescriptorSet descr_set = {};
    if (!p_main.descr_set_layouts.empty()) {
        VkDescriptorSetLayout descr_set_layout = p_main.descr_set_layouts[0];
        descr_set =
            PrepareDescriptorSet(api, &storages, descr_set_layout, bindings, ctx_->default_descr_alloc(), ctx_->log());
    }

    { // transition resources if required
        VkPipelineStageFlags src_stages = 0, dst_stages = 0;

        Ren::SmallVector<VkImageMemoryBarrier, 16> img_barriers;
        Ren::SmallVector<VkBufferMemoryBarrier, 4> buf_barriers;

        for (const auto &b : bindings) {
            if (b.trg == Ren::eBindTarget::Tex || b.trg == Ren::eBindTarget::TexSampled) {
                assert(b.handle.img->resource_state == Ren::eResState::ShaderResource ||
                       b.handle.img->resource_state == Ren::eResState::DepthRead ||
                       b.handle.img->resource_state == Ren::eResState::StencilTestDepthFetch);
            }
        }

        for (const auto &rt : color_rts) {
            if (rt.ref->resource_state != Ren::eResState::RenderTarget) {
                auto &new_barrier = img_barriers.emplace_back();
                new_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                new_barrier.srcAccessMask = VKAccessFlagsForState(rt.ref->resource_state);
                new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::RenderTarget);
                new_barrier.oldLayout = VkImageLayout(Ren::VKImageLayoutForState(rt.ref->resource_state));
                new_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                new_barrier.image = rt.ref->handle().img;
                new_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                new_barrier.subresourceRange.baseMipLevel = 0;
                new_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                new_barrier.subresourceRange.baseArrayLayer = 0;
                new_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

                src_stages |= VKPipelineStagesForState(rt.ref->resource_state);
                dst_stages |= VKPipelineStagesForState(Ren::eResState::RenderTarget);

                rt.ref->resource_state = Ren::eResState::RenderTarget;
            }
        }

        if (depth_rt.ref && depth_rt.ref->resource_state != Ren::eResState::DepthWrite &&
            depth_rt.ref->resource_state != Ren::eResState::DepthRead &&
            depth_rt.ref->resource_state != Ren::eResState::StencilTestDepthFetch) {
            auto &new_barrier = img_barriers.emplace_back();
            new_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            new_barrier.srcAccessMask = VKAccessFlagsForState(depth_rt.ref->resource_state);
            new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::DepthWrite);
            new_barrier.oldLayout = VkImageLayout(Ren::VKImageLayoutForState(depth_rt.ref->resource_state));
            new_barrier.newLayout = VkImageLayout(Ren::VKImageLayoutForState(Ren::eResState::DepthWrite));
            new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.image = depth_rt.ref->handle().img;
            new_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            new_barrier.subresourceRange.baseMipLevel = 0;
            new_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            new_barrier.subresourceRange.baseArrayLayer = 0;
            new_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            src_stages |= VKPipelineStagesForState(depth_rt.ref->resource_state);
            dst_stages |= VKPipelineStagesForState(Ren::eResState::DepthWrite);

            depth_rt.ref->resource_state = Ren::eResState::DepthWrite;
        }

        const Ren::BufferMain &default_vertex_buf1 = storages.buffers.Get(ctx_->default_vertex_buf1()).first;
        if (default_vertex_buf1.resource_state != Ren::eResState::VertexBuffer) {
            auto &new_barrier = buf_barriers.emplace_back();
            new_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            new_barrier.srcAccessMask = VKAccessFlagsForState(default_vertex_buf1.resource_state);
            new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::VertexBuffer);
            new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.buffer = default_vertex_buf1.buf;
            new_barrier.offset = VkDeviceSize{0};
            new_barrier.size = VK_WHOLE_SIZE;

            src_stages |= VKPipelineStagesForState(default_vertex_buf1.resource_state);
            dst_stages |= VKPipelineStagesForState(Ren::eResState::VertexBuffer);

            default_vertex_buf1.resource_state = Ren::eResState::VertexBuffer;
        }

        const Ren::BufferMain &default_vertex_buf2 = storages.buffers.Get(ctx_->default_vertex_buf2()).first;
        if (default_vertex_buf2.resource_state != Ren::eResState::VertexBuffer) {
            auto &new_barrier = buf_barriers.emplace_back();
            new_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            new_barrier.srcAccessMask = VKAccessFlagsForState(default_vertex_buf2.resource_state);
            new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::VertexBuffer);
            new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.buffer = default_vertex_buf2.buf;
            new_barrier.offset = VkDeviceSize{0};
            new_barrier.size = VK_WHOLE_SIZE;

            src_stages |= VKPipelineStagesForState(default_vertex_buf2.resource_state);
            dst_stages |= VKPipelineStagesForState(Ren::eResState::VertexBuffer);

            default_vertex_buf2.resource_state = Ren::eResState::VertexBuffer;
        }

        const Ren::BufferMain &indices_buf = storages.buffers.Get(ctx_->default_vertex_buf2()).first;
        if (indices_buf.resource_state != Ren::eResState::IndexBuffer) {
            auto &new_barrier = buf_barriers.emplace_back();
            new_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            new_barrier.srcAccessMask = VKAccessFlagsForState(indices_buf.resource_state);
            new_barrier.dstAccessMask = VKAccessFlagsForState(Ren::eResState::IndexBuffer);
            new_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            new_barrier.buffer = indices_buf.buf;
            new_barrier.offset = VkDeviceSize{0};
            new_barrier.size = VK_WHOLE_SIZE;

            src_stages |= VKPipelineStagesForState(indices_buf.resource_state);
            dst_stages |= VKPipelineStagesForState(Ren::eResState::IndexBuffer);

            indices_buf.resource_state = Ren::eResState::IndexBuffer;
        }

        src_stages &= api.supported_stages_mask;
        dst_stages &= api.supported_stages_mask;

        if (!img_barriers.empty() || !buf_barriers.empty()) {
            api.vkCmdPipelineBarrier(cmd_buf, src_stages ? src_stages : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, dst_stages,
                                     0, 0, nullptr, buf_barriers.size(), buf_barriers.data(), img_barriers.size(),
                                     img_barriers.data());
        }
    }

    const Ren::RenderPassHandle rp = ctx_->FindOrCreateRenderPass(depth_rt, color_rts);
    const Ren::RenderPassMain &rp_main = storages.render_passes.Get(rp).first;
    const Ren::Framebuffer *fb =
        FindOrCreateFramebuffer(&rp_main, new_rast_state.depth.test_enabled ? depth_rt : Ren::RenderTarget{},
                                new_rast_state.stencil.enabled ? depth_rt : Ren::RenderTarget{}, color_rts);

    const Ren::VertexInputHandle vi = (prim == ePrim::Quad) ? fs_quad_vtx_input_ : sphere_vtx_input_;
    const Ren::PipelineHandle pipeline = ctx_->FindOrCreatePipeline(new_rast_state, p, vi, rp, 0);
    const Ren::PipelineMain &pi_main = storages.pipelines.Get(pipeline).first;

    VkRenderPassBeginInfo render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render_pass_begin_info.renderPass = rp_main.handle;
    render_pass_begin_info.framebuffer = fb->vk_handle();
    render_pass_begin_info.renderArea = {{0, 0}, {uint32_t(fb->w), uint32_t(fb->h)}};

    Ren::SmallVector<VkClearValue, 4> clear_values;
    if (depth_rt) {
        clear_values.push_back(VkClearValue{});
    }
    clear_values.resize(clear_values.size() + uint32_t(color_rts.size()), VkClearValue{});
    render_pass_begin_info.pClearValues = clear_values.cdata();
    render_pass_begin_info.clearValueCount = clear_values.size();

    api.vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_main.handle);

    const VkViewport viewport = {0.0f, 0.0f, float(new_rast_state.viewport[2]), float(new_rast_state.viewport[3]),
                                 0.0f, 1.0f};
    api.vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor = {{0, 0}, {uint32_t(new_rast_state.viewport[2]), uint32_t(new_rast_state.viewport[3])}};
    if (new_rast_state.scissor.enabled) {
        scissor = VkRect2D{{new_rast_state.scissor.rect[0], new_rast_state.scissor.rect[1]},
                           {uint32_t(new_rast_state.scissor.rect[2]), uint32_t(new_rast_state.scissor.rect[3])}};
    }
    api.vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    if (descr_set) {
        api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pi_main.layout, 0, 1, &descr_set, 0,
                                    nullptr);
    }

    if (uniform_data) {
        api.vkCmdPushConstants(cmd_buf, pi_main.layout, p_main.pc_ranges[0].stageFlags, uniform_data_offset,
                               uniform_data_len, uniform_data);
    }

    const Ren::VertexInputMain &vtx_input_main = storages.vtx_inputs.Get(pi_main.vtx_input).first;
    if (prim == ePrim::Quad) {
        VertexInput_BindBuffers(api, vtx_input_main, storages.buffers, cmd_buf, quad_ndx_.offset, VK_INDEX_TYPE_UINT16);

        api.vkCmdDrawIndexed(cmd_buf, 6u, // index count
                             instances,   // instance count
                             0,           // first index
                             0,           // vertex offset
                             0);          // first instance
    } else if (prim == ePrim::Sphere) {
        VertexInput_BindBuffers(api, vtx_input_main, storages.buffers, cmd_buf, sphere_ndx_.offset,
                                VK_INDEX_TYPE_UINT16);

        api.vkCmdDrawIndexed(cmd_buf, uint32_t(SphereIndicesCount), // index count
                             instances,                             // instance count
                             0,                                     // first index
                             0,                                     // vertex offset
                             0);                                    // first instance
    }

    api.vkCmdEndRenderPass(cmd_buf);
}

void Eng::PrimDraw::DrawPrim(const ePrim prim, const Ren::ProgramHandle p, Ren::RenderTarget depth_rt,
                             Ren::Span<const Ren::RenderTarget> color_rts, const Ren::RastState &new_rast_state,
                             Ren::RastState &applied_rast_state, Ren::Span<const Ren::Binding> bindings,
                             const void *uniform_data, const int uniform_data_len, const int uniform_data_offset,
                             const int instances) {
    const Ren::ApiContext &api = ctx_->api();
    const VkCommandBuffer cmd_buf = api.draw_cmd_buf[api.backend_frame];
    DrawPrim(cmd_buf, prim, p, depth_rt, color_rts, new_rast_state, applied_rast_state, bindings, uniform_data,
             uniform_data_len, uniform_data_offset, instances);
}

void Eng::PrimDraw::ClearTarget(Ren::CommandBuffer cmd_buf, Ren::RenderTarget depth_rt,
                                Ren::Span<const Ren::RenderTarget> color_rts) {
    const Ren::RenderPassHandle rp = ctx_->FindOrCreateRenderPass(depth_rt, color_rts);
    const Ren::RenderPassMain &rp_main = ctx_->render_passes().Get(rp).first;
    const Ren::Framebuffer *fb = FindOrCreateFramebuffer(&rp_main, depth_rt, depth_rt, color_rts);

    VkRenderPassBeginInfo render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render_pass_begin_info.renderPass = rp_main.handle;
    render_pass_begin_info.framebuffer = fb->vk_handle();
    render_pass_begin_info.renderArea = {{0, 0}, {uint32_t(fb->w), uint32_t(fb->h)}};

    Ren::SmallVector<VkClearValue, 4> clear_values;
    if (depth_rt) {
        clear_values.push_back(VkClearValue{});
    }
    clear_values.resize(clear_values.size() + uint32_t(color_rts.size()), VkClearValue{});
    render_pass_begin_info.pClearValues = clear_values.cdata();
    render_pass_begin_info.clearValueCount = clear_values.size();

    const Ren::ApiContext &api = ctx_->api();
    api.vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // Do nothing

    api.vkCmdEndRenderPass(cmd_buf);
}

void Eng::PrimDraw::ClearTarget(Ren::RenderTarget depth_rt, Ren::Span<const Ren::RenderTarget> color_rts) {
    const Ren::ApiContext &api = ctx_->api();
    VkCommandBuffer cmd_buf = api.draw_cmd_buf[api.backend_frame];
    ClearTarget(cmd_buf, depth_rt, color_rts);
}