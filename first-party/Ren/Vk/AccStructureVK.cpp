#include "../AccStructure.h"

#include "VKCtx.h"

bool Ren::AccStruct_Init(AccStructMain &acc_main, AccStructCold &acc_cold, String name,
                         const VkAccelerationStructureKHR handle, const FreelistAlloc::Allocation mem_alloc) {
    acc_main.type = eAccStructType::HWRT;
    acc_main.hw.handle = handle;
    acc_main.hw.mem_alloc = mem_alloc;

    acc_cold.name = std::move(name);
    return true;
}

VkDeviceAddress Ren::AccStruct_GetDeviceAddress(const ApiContext &api, const AccStructMain &acc_main) {
    VkAccelerationStructureDeviceAddressInfoKHR info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    info.accelerationStructure = acc_main.hw.handle;
    return api.vkGetAccelerationStructureDeviceAddressKHR(api.device, &info);
}

void Ren::AccStruct_Destroy(const ApiContext &api, AccStructMain &acc_main, AccStructCold &acc_cold) {
    if (acc_main.type == eAccStructType::HWRT) {
        if (acc_main.hw.handle) {
            api.acc_structs_to_destroy[api.backend_frame].push_back(acc_main.hw.handle);
        }
    }
    acc_main = {};
    acc_cold = {};
}

void Ren::AccStruct_DestroyImmediately(const ApiContext &api, AccStructMain &acc_main, AccStructCold &acc_cold) {
    if (acc_main.type == eAccStructType::HWRT) {
        if (acc_main.hw.handle) {
            api.vkDestroyAccelerationStructureKHR(api.device, acc_main.hw.handle, nullptr);
        }
    }
    acc_main = {};
    acc_cold = {};
}

/*void Ren::AccStructureVK::Free() {
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
}*/