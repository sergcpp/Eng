#include "ExBuildAccStructures.h"

#include <Ren/Context.h>
#include <Ren/VKCtx.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgBuilder.h"

void Eng::ExBuildAccStructures::Execute_HWRT(const FgContext &fg) {
    const Ren::BufferROHandle rt_obj_instances_buf = fg.AccessROBuffer(rt_obj_instances_buf_ro_);
    [[maybe_unused]] const Ren::BufferHandle rt_tlas_buf = fg.AccessRWBuffer(rt_tlas_buf_);
    const Ren::BufferHandle rt_tlas_build_scratch_buf = fg.AccessRWBuffer(rt_tlas_build_scratch_buf_);

    const Ren::ApiContext &api = fg.ren_ctx().api();

    auto *vk_tlas = reinterpret_cast<Ren::AccStructureVK *>(rt_tlas_);

    const Ren::BufferMain &rt_obj_instances_buf_main = fg.storages().buffers.Get(rt_obj_instances_buf).first;
    VkAccelerationStructureGeometryInstancesDataKHR instances_data = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    instances_data.data.deviceAddress = Buffer_GetDeviceAddress(api, rt_obj_instances_buf_main);

    VkAccelerationStructureGeometryKHR tlas_geo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlas_geo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlas_geo.geometry.instances = instances_data;

    VkAccelerationStructureBuildGeometryInfoKHR tlas_build_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    tlas_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    tlas_build_info.geometryCount = 1;
    tlas_build_info.pGeometries = &tlas_geo;
    tlas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    tlas_build_info.srcAccelerationStructure = VK_NULL_HANDLE;
    tlas_build_info.dstAccelerationStructure = vk_tlas->vk_handle();

    const Ren::BufferMain &rt_tlas_build_scratch_buf_main = fg.storages().buffers.Get(rt_tlas_build_scratch_buf).first;
    tlas_build_info.scratchData.deviceAddress = Buffer_GetDeviceAddress(api, rt_tlas_build_scratch_buf_main);

    VkAccelerationStructureBuildRangeInfoKHR range_info = {};
    range_info.primitiveOffset = 0;
    range_info.primitiveCount = p_list_->rt_obj_instances[rt_index_].count;
    range_info.firstVertex = 0;
    range_info.transformOffset = 0;

    VkCommandBuffer cmd_buf = fg.cmd_buf();

    const VkAccelerationStructureBuildRangeInfoKHR *build_range = &range_info;
    api.vkCmdBuildAccelerationStructuresKHR(cmd_buf, 1, &tlas_build_info, &build_range);
}
