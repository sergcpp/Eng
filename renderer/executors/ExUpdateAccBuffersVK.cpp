#include "ExUpdateAccBuffers.h"

#include <Ren/Context.h>

#include "../framegraph/FgBuilder.h"

void Eng::ExUpdateAccBuffers::Execute_HWRT(const FgContext &fg) {
    const Ren::BufferHandle rt_geo_instances_buf = fg.AccessRWBuffer(rt_geo_instances_buf_);
    const Ren::BufferHandle rt_obj_instances_buf = fg.AccessRWBuffer(rt_obj_instances_buf_);

    const auto &rt_geo_instances = p_list_->rt_geo_instances[rt_index_];
    const Ren::BufferHandle rt_geo_instances_stage_buf = p_list_->rt_geo_instances_stage_buf[rt_index_];

    const Ren::ApiContext &api = fg.ren_ctx().api();
    const Ren::StoragesRef &storages = fg.storages();

    if (rt_geo_instances.count) {
        const auto &[rt_geo_instances_stage_buf_main, rt_geo_instances_stage_buf_cold] =
            storages.buffers.Get(rt_geo_instances_stage_buf);

        uint8_t *stage_mem =
            Buffer_MapRange(api, rt_geo_instances_stage_buf_main, rt_geo_instances_stage_buf_cold,
                            fg.backend_frame() * RTGeoInstancesBufChunkSize, RTGeoInstancesBufChunkSize);

        const uint32_t rt_geo_instances_mem_size = rt_geo_instances.count * sizeof(rt_geo_instance_t);
        assert(rt_geo_instances_mem_size < RTGeoInstancesBufChunkSize);
        if (stage_mem) {
            memcpy(stage_mem, rt_geo_instances.data, rt_geo_instances_mem_size);
            Buffer_Unmap(api, rt_geo_instances_stage_buf_main, rt_geo_instances_stage_buf_cold);
        }

        Ren::BufferMain &rt_geo_instances_buf_main = fg.storages().buffers.Get(rt_geo_instances_buf).first;
        CopyBufferToBuffer(api, rt_geo_instances_stage_buf_main, fg.backend_frame() * RTGeoInstancesBufChunkSize,
                           rt_geo_instances_buf_main, 0, rt_geo_instances_mem_size, fg.cmd_buf());
    }

    const auto &rt_obj_instances = p_list_->rt_obj_instances[rt_index_];
    auto &rt_obj_instances_stage_buf = p_list_->rt_obj_instances_stage_buf[rt_index_];

    if (rt_obj_instances.count) {
        const auto &[rt_obj_instances_stage_buf_main, rt_obj_instances_stage_buf_cold] =
            storages.buffers.Get(rt_obj_instances_stage_buf);

        uint8_t *stage_mem =
            Buffer_MapRange(api, rt_obj_instances_stage_buf_main, rt_obj_instances_stage_buf_cold,
                            fg.backend_frame() * HWRTObjInstancesBufChunkSize, HWRTObjInstancesBufChunkSize);

        const uint32_t rt_obj_instances_mem_size = rt_obj_instances.count * sizeof(VkAccelerationStructureInstanceKHR);
        assert(rt_obj_instances_mem_size <= HWRTObjInstancesBufChunkSize);
        if (stage_mem) {
            auto *out_instances = reinterpret_cast<VkAccelerationStructureInstanceKHR *>(stage_mem);
            for (uint32_t i = 0; i < rt_obj_instances.count; ++i) {
                auto &new_instance = out_instances[i];
                memcpy(new_instance.transform.matrix, rt_obj_instances.data[i].xform, 12 * sizeof(float));
                new_instance.instanceCustomIndex = rt_obj_instances.data[i].geo_index;
                new_instance.mask = rt_obj_instances.data[i].mask;
                new_instance.flags = 0;
                new_instance.accelerationStructureReference =
                    reinterpret_cast<const Ren::AccStructureVK *>(rt_obj_instances.data[i].blas_ref)
                        ->vk_device_address();
            }

            Buffer_Unmap(api, rt_obj_instances_stage_buf_main, rt_obj_instances_stage_buf_cold);
        } else {
            fg.log()->Error("ExUpdateAccBuffers: Failed to map rt obj instance buffer!");
        }

        Ren::BufferMain &rt_obj_instances_buf_main = fg.storages().buffers.Get(rt_obj_instances_buf).first;
        CopyBufferToBuffer(api, rt_obj_instances_stage_buf_main, fg.backend_frame() * HWRTObjInstancesBufChunkSize,
                           rt_obj_instances_buf_main, 0, rt_obj_instances_mem_size, fg.cmd_buf());
    }
}
