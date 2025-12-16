#include "AccStructure.h"

#include "VKCtx.h"

void Ren::AccStructureVK::Free() {
    if (handle_) {
        api_->acc_structs_to_destroy[api_->backend_frame].push_back(handle_);
        handle_ = {};
    }
}

void Ren::AccStructureVK::FreeImmediate() {
    if (handle_) {
        api_->vkDestroyAccelerationStructureKHR(api_->device, handle_, nullptr);
        handle_ = {};
    }
}

VkDeviceAddress Ren::AccStructureVK::vk_device_address() const {
    VkAccelerationStructureDeviceAddressInfoKHR info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    info.accelerationStructure = handle_;
    return api_->vkGetAccelerationStructureDeviceAddressKHR(api_->device, &info);
}

bool Ren::AccStructureVK::Init(const ApiContext *api, VkAccelerationStructureKHR handle) {
    Free();

    api_ = api;
    handle_ = handle;
    return true;
}