#pragma once

#include <cstdint>

#include "Buffer.h"
#include "FreelistAlloc.h"
#include "Fwd.h"
#include "Resource.h"

namespace Ren {
enum class eAccStructType { SWRT, HWRT, _Count };

struct AccStructMain {
    eAccStructType type = eAccStructType::_Count;
    eResState resource_state = eResState::Undefined;
    union {
        struct {
            uint32_t mesh_index;
            SubAllocation nodes_alloc, prim_alloc;
        } sw;
        struct {
#if defined(REN_VK_BACKEND)
            VkAccelerationStructureKHR handle;
            FreelistAlloc::Allocation mem_alloc;
#endif
        } hw;
    };
    AccStructMain() {}
};

struct AccStructCold {
    String name;
};

bool AccStruct_Init(AccStructMain &acc_main, AccStructCold &acc_cold, String name, uint32_t mesh_index,
                    SubAllocation nodes_alloc, SubAllocation prim_alloc);
#if defined(REN_VK_BACKEND)
bool AccStruct_Init(AccStructMain &acc_main, AccStructCold &acc_cold, String name, VkAccelerationStructureKHR handle,
                    FreelistAlloc::Allocation mem_alloc);
VkDeviceAddress AccStruct_GetDeviceAddress(const ApiContext &api, const AccStructMain &acc_main);
#endif

void AccStruct_Destroy(const ApiContext &api, AccStructMain &acc_main, AccStructCold &acc_cold);
void AccStruct_DestroyImmediately(const ApiContext &api, AccStructMain &acc_main, AccStructCold &acc_cold);

} // namespace Ren