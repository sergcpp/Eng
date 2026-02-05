#include "RenderPass.h"

#include "VKCtx.h"

#ifndef NDEBUG
#define VERBOSE_LOGGING
#endif

namespace Ren {
static_assert(int(eImageLayout::Undefined) == VK_IMAGE_LAYOUT_UNDEFINED);
static_assert(int(eImageLayout::General) == VK_IMAGE_LAYOUT_GENERAL);
static_assert(int(eImageLayout::ColorAttachmentOptimal) == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
static_assert(int(eImageLayout::DepthStencilAttachmentOptimal) == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
static_assert(int(eImageLayout::DepthStencilReadOnlyOptimal) == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
static_assert(int(eImageLayout::ShaderReadOnlyOptimal) == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
static_assert(int(eImageLayout::TransferSrcOptimal) == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
static_assert(int(eImageLayout::TransferDstOptimal) == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

extern const VkAttachmentLoadOp vk_load_ops[] = {
    VK_ATTACHMENT_LOAD_OP_LOAD,      // Load
    VK_ATTACHMENT_LOAD_OP_CLEAR,     // Clear
    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // DontCare
    VK_ATTACHMENT_LOAD_OP_NONE_EXT   // None
};
static_assert(std::size(vk_load_ops) == int(eLoadOp::_Count));

extern const VkAttachmentStoreOp vk_store_ops[] = {
    VK_ATTACHMENT_STORE_OP_STORE,     // Store
    VK_ATTACHMENT_STORE_OP_DONT_CARE, // DontCare
    VK_ATTACHMENT_STORE_OP_NONE_EXT   // None
};
static_assert(std::size(vk_store_ops) == int(eStoreOp::_Count));

// make sure we can simply cast these
static_assert(VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT == 1);
static_assert(VkSampleCountFlagBits::VK_SAMPLE_COUNT_2_BIT == 2);
static_assert(VkSampleCountFlagBits::VK_SAMPLE_COUNT_4_BIT == 4);
static_assert(VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT == 8);
} // namespace Ren

bool Ren::RenderPass_Init(const ApiContext &api, RenderPassMain &rp_main, const RenderTargetInfo &depth_rt,
                          Span<const RenderTargetInfo> color_rts, ILog *log) {
    SmallVector<VkAttachmentDescription, 4> pass_attachments;
    SmallVector<VkAttachmentReference, 4> color_attachment_refs(uint32_t(color_rts.size()),
                                                                {VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED});
    VkAttachmentReference depth_attachment_ref = {VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED};

    assert(rp_main.handle == VK_NULL_HANDLE);
    assert(!rp_main.depth_rt);
    assert(rp_main.color_rts.empty());

    rp_main.color_rts.resize(uint32_t(color_rts.size()));

    if (depth_rt) {
        const uint32_t att_index = pass_attachments.size();

        auto &att_desc = pass_attachments.emplace_back();
        att_desc.format = Ren::VKFormatFromFormat(depth_rt.format);
        att_desc.samples = VkSampleCountFlagBits(depth_rt.samples);
        att_desc.loadOp = vk_load_ops[int(depth_rt.load)];
        if (att_desc.loadOp == VK_ATTACHMENT_LOAD_OP_NONE_EXT && !api.renderpass_loadstore_none_supported) {
            att_desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        }
        att_desc.storeOp = vk_store_ops[int(depth_rt.store)];
        if (att_desc.storeOp == VK_ATTACHMENT_STORE_OP_NONE_EXT && !api.renderpass_loadstore_none_supported) {
            att_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }
        att_desc.stencilLoadOp = vk_load_ops[int(depth_rt.stencil_load)];
        if (att_desc.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_NONE_EXT && !api.renderpass_loadstore_none_supported) {
            att_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        }
        att_desc.stencilStoreOp = vk_store_ops[int(depth_rt.stencil_store)];
        if (att_desc.stencilStoreOp == VK_ATTACHMENT_STORE_OP_NONE_EXT && !api.renderpass_loadstore_none_supported) {
            att_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        }
        att_desc.initialLayout = VkImageLayout(depth_rt.layout);
        att_desc.finalLayout = att_desc.initialLayout;

        depth_attachment_ref.attachment = att_index;
        depth_attachment_ref.layout = att_desc.initialLayout;

        rp_main.depth_rt = depth_rt;
    }

    for (int i = 0; i < color_rts.size(); ++i) {
        if (!color_rts[i]) {
            continue;
        }

        const uint32_t att_index = pass_attachments.size();

        auto &att_desc = pass_attachments.emplace_back();
        att_desc.format = VKFormatFromFormat(color_rts[i].format);
        att_desc.samples = VkSampleCountFlagBits(color_rts[i].samples);
        if (VkImageLayout(color_rts[i].layout) == VK_IMAGE_LAYOUT_UNDEFINED) {
            att_desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            att_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        } else {
            att_desc.loadOp = vk_load_ops[int(color_rts[i].load)];
            att_desc.stencilLoadOp = vk_load_ops[int(color_rts[i].load)];
        }
        att_desc.storeOp = vk_store_ops[int(color_rts[i].store)];
        att_desc.stencilStoreOp = vk_store_ops[int(color_rts[i].store)];
        att_desc.initialLayout = VkImageLayout(color_rts[i].layout);
        att_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if (att_desc.loadOp == VK_ATTACHMENT_LOAD_OP_NONE_EXT && !api.renderpass_loadstore_none_supported) {
            att_desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        }
        if (att_desc.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_NONE_EXT && !api.renderpass_loadstore_none_supported) {
            att_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        }
        if (att_desc.storeOp == VK_ATTACHMENT_STORE_OP_NONE && !api.renderpass_loadstore_none_supported) {
            att_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }
        if (att_desc.stencilStoreOp == VK_ATTACHMENT_STORE_OP_NONE && !api.renderpass_loadstore_none_supported) {
            att_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        }

        color_attachment_refs[i].attachment = att_index;
        color_attachment_refs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        rp_main.color_rts[i] = color_rts[i];
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = uint32_t(color_rts.size());
    subpass.pColorAttachments = color_attachment_refs.data();
    if (depth_attachment_ref.attachment != VK_ATTACHMENT_UNUSED) {
        subpass.pDepthStencilAttachment = &depth_attachment_ref;
    }

    VkRenderPassCreateInfo render_pass_create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_create_info.attachmentCount = pass_attachments.size();
    render_pass_create_info.pAttachments = pass_attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    const VkResult res = api.vkCreateRenderPass(api.device, &render_pass_create_info, nullptr, &rp_main.handle);
    if (res != VK_SUCCESS) {
        log->Error("Failed to create render pass!");
        return false;
    }
#ifdef VERBOSE_LOGGING
    log->Info("RenderPass %p created", rp_main.handle);
#endif
    return true;
}

void Ren::RenderPass_Destroy(const ApiContext &api, RenderPassMain &rp_main) {
    if (rp_main.handle != VK_NULL_HANDLE) {
        api.render_passes_to_destroy[api.backend_frame].push_back(rp_main.handle);
    }
    rp_main = {};
}

void Ren::RenderPass_DestroyImmediately(const ApiContext& api, RenderPassMain& rp_main) {
    if (rp_main.handle != VK_NULL_HANDLE) {
        api.vkDestroyRenderPass(api.device, rp_main.handle, nullptr);
    }
    rp_main = {};
}

#undef VERBOSE_LOGGING
