#include "ExUpdateAccBuffers.h"

#include <Ren/Context.h>

#include "../framegraph/FgBuilder.h"

void Eng::ExUpdateAccBuffers::Execute(const FgContext &fg) {
    if (fg.ren_ctx().capabilities.hwrt) {
#if !defined(REN_GL_BACKEND)
        Execute_HWRT(fg);
#endif
    } else {
        Execute_SWRT(fg);
    }
}

void Eng::ExUpdateAccBuffers::Execute_SWRT(const FgContext &fg) {
    const Ren::BufferHandle rt_geo_instances_buf = fg.AccessRWBuffer(rt_geo_instances_);

    const auto &rt_geo_instances = p_list_->rt_geo_instances[rt_index_];
    const Ren::BufferHandle rt_geo_instances_stage = p_list_->rt_geo_instances_stage[rt_index_];

    const Ren::ApiContext &api = fg.ren_ctx().api();
    const Ren::StoragesRef &storages = fg.storages();

    if (rt_geo_instances.count) {
        const auto &[rt_geo_instances_stage_buf_main, rt_geo_instances_stage_buf_cold] =
            storages.buffers[rt_geo_instances_stage];

        uint8_t *stage_mem =
            Buffer_MapRange(api, rt_geo_instances_stage_buf_main, rt_geo_instances_stage_buf_cold,
                            fg.backend_frame() * RTGeoInstancesBufChunkSize, RTGeoInstancesBufChunkSize);
        const uint32_t rt_geo_instances_mem_size = rt_geo_instances.count * sizeof(rt_geo_instance_t);
        if (stage_mem) {
            memcpy(stage_mem, rt_geo_instances.data, rt_geo_instances_mem_size);
            Buffer_Unmap(api, rt_geo_instances_stage_buf_main, rt_geo_instances_stage_buf_cold);
        }

        Ren::BufferMain &rt_geo_instances_buf_main = fg.storages().buffers[rt_geo_instances_buf].first;
        CopyBufferToBuffer(api, rt_geo_instances_stage_buf_main, fg.backend_frame() * RTGeoInstancesBufChunkSize,
                           rt_geo_instances_buf_main, 0, rt_geo_instances_mem_size, fg.cmd_buf());
    }
}