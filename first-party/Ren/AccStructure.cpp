#include "AccStructure.h"

bool Ren::AccStruct_Init(AccStructMain &acc_main, AccStructCold &acc_cold, String name, const uint32_t mesh_index,
                         const SubAllocation nodes_alloc, const SubAllocation prim_alloc) {
    acc_main.type = eAccStructType::SWRT;
    acc_main.sw.mesh_index = mesh_index;
    acc_main.sw.nodes_alloc = nodes_alloc;
    acc_main.sw.prim_alloc = prim_alloc;

    acc_cold.name = std::move(name);
    return true;
}

#if defined(REN_GL_BACKEND)
void Ren::AccStruct_Destroy(const ApiContext &api, AccStructMain &acc_main, AccStructCold &acc_cold) {
    acc_main = {};
    acc_cold = {};
}
void Ren::AccStruct_DestroyImmediately(const ApiContext& api, AccStructMain& acc_main, AccStructCold& acc_cold) {
    AccStruct_Destroy(api, acc_main, acc_cold);
}
#endif