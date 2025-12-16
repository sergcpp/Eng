#pragma once

#include <Ren/Image.h>
#include <Sys/AsyncFileReader.h>

#if defined(REN_VK_BACKEND)
#include <Ren/VKCtx.h>
#endif

namespace Eng {
class TextureUpdateFileBuf : public Sys::FileReadBufBase {
    const Ren::ApiContext *api_ = nullptr;
    Ren::ILog *log_ = nullptr;
    Ren::BufferMain stage_buf_main_;
    Ren::BufferCold stage_buf_cold_;

  public:
    TextureUpdateFileBuf(const Ren::ApiContext *api, Ren::ILog *log) : api_(api) {
        Realloc(24 * 1024 * 1024);

#if defined(REN_VK_BACKEND)
        VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence new_fence;
        VkResult res = api->vkCreateFence(api->device, &fence_info, nullptr, &new_fence);
        assert(res == VK_SUCCESS);

        fence = Ren::SyncFence{api, new_fence};

        VkCommandBufferAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool = api->command_pool;
        alloc_info.commandBufferCount = 1;

        VkCommandBuffer command_buf = {};
        res = api->vkAllocateCommandBuffers(api->device, &alloc_info, &command_buf);
        assert(res == VK_SUCCESS);

        cmd_buf = command_buf;
#endif
    }
    ~TextureUpdateFileBuf() override {
        Free();

#if defined(REN_VK_BACKEND)
        VkCommandBuffer _cmd_buf = reinterpret_cast<VkCommandBuffer>(cmd_buf);
        api_->vkFreeCommandBuffers(api_->device, api_->command_pool, 1, &_cmd_buf);
#endif
    }

    Ren::BufferMain &stage_buf() { return stage_buf_main_; }

    uint8_t *Alloc(const size_t new_size) override {
        if (!Buffer_Init(*api_, stage_buf_main_, stage_buf_cold_, Ren::String{"Tex Upload Buf"}, Ren::eBufType::Upload,
                         uint32_t(new_size), log_)) {
            log_->Error("Failed to initialize %s", stage_buf_cold_.name.c_str());
            return nullptr;
        }
        return Buffer_Map(*api_, stage_buf_main_, stage_buf_cold_, true /* persistent */);
    }

    void Free() override {
        mem_ = nullptr;
        if (stage_buf_cold_.mapped_ptr) {
            Buffer_Unmap(*api_, stage_buf_main_, stage_buf_cold_);
        }
        Buffer_Destroy(*api_, stage_buf_main_, stage_buf_cold_);
    }

    Ren::SyncFence fence;
    Ren::CommandBuffer cmd_buf = {};
};
} // namespace Eng