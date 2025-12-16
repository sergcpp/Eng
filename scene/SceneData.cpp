#include "SceneData.h"

#include <Ren/Context.h>
#if defined(REN_VK_BACKEND)
#include <Ren/DescriptorPool.h>
#include <Ren/VKCtx.h>
#endif

Eng::PersistentGpuData::PersistentGpuData(Ren::Context &_ctx) : ctx(_ctx) {}

Eng::PersistentGpuData::~PersistentGpuData() {
    Release();

    ctx.ReleaseBuffer(vertex_buf1);
    ctx.ReleaseBuffer(vertex_buf2);
    ctx.ReleaseBuffer(indices_buf);
    ctx.ReleaseBuffer(skin_vertex_buf);
    ctx.ReleaseBuffer(delta_buf);
}

void Eng::PersistentGpuData::Release() {
    const Ren::ApiContext &api = ctx.api();
#if defined(REN_VK_BACKEND)
    if (textures_descr_pool) {
        api.vkDestroyDescriptorSetLayout(api.device, textures_descr_layout, nullptr);
        textures_descr_layout = VK_NULL_HANDLE;
        api.vkDestroyDescriptorSetLayout(api.device, rt_textures_descr_layout, nullptr);
        rt_textures_descr_layout = VK_NULL_HANDLE;
        api.vkDestroyDescriptorSetLayout(api.device, rt_inline_textures_descr_layout, nullptr);
        rt_inline_textures_descr_layout = VK_NULL_HANDLE;
        for (auto &descr_set : textures_descr_sets) {
            descr_set.clear();
        }
        for (auto &descr_set : rt_textures_descr_sets) {
            descr_set = VK_NULL_HANDLE;
        }
        api.allocators_to_release[api.backend_frame].push_back(std::move(mem_allocs));
    }
    textures_descr_pool = {};
    rt_textures_descr_pool = {};
    rt_inline_textures_descr_pool = {};
#elif defined(REN_GL_BACKEND)
    if (textures_buf) {
        ctx.ReleaseBuffer(textures_buf);
    }
    textures_buf = {};
#endif
    if (instance_buf) {
        ctx.ReleaseBuffer(instance_buf);
    }
    instance_buf = {};
    if (materials_buf) {
        ctx.ReleaseBuffer(materials_buf);
    }
    materials_buf = {};
    if (vertex_buf1) {
        const auto &[vertex_buf1_main, vertex_buf1_cold] = ctx.buffers().Get(vertex_buf1);
        Buffer_Resize(api, vertex_buf1_main, vertex_buf1_cold, 128, ctx.log(), false, true /* destroy_immediately */);
    }
    if (vertex_buf2) {
        const auto &[vertex_buf2_main, vertex_buf2_cold] = ctx.buffers().Get(vertex_buf2);
        Buffer_Resize(api, vertex_buf2_main, vertex_buf2_cold, 128, ctx.log(), false, true /* destroy_immediately */);
    }
    if (skin_vertex_buf) {
        const auto &[skin_vertex_buf_main, skin_vertex_buf_cold] = ctx.buffers().Get(skin_vertex_buf);
        Buffer_Resize(api, skin_vertex_buf_main, skin_vertex_buf_cold, 128, ctx.log(), false,
                      true /* destroy_immediately */);
    }
    if (delta_buf) {
        const auto &[delta_buf_main, delta_buf_cold] = ctx.buffers().Get(delta_buf);
        Buffer_Resize(api, delta_buf_main, delta_buf_cold, 128, ctx.log(), false, true /* destroy_immediately */);
    }
    if (indices_buf) {
        const auto &[indices_buf_main, indices_buf_cold] = ctx.buffers().Get(indices_buf);
        Buffer_Resize(api, indices_buf_main, indices_buf_cold, 128, ctx.log(), false, true /* destroy_immediately */);
    }
    if (stoch_lights_buf) {
        ctx.ReleaseBuffer(stoch_lights_buf);
    }
    stoch_lights_buf = {};
    if (stoch_lights_nodes_buf) {
        ctx.ReleaseBuffer(stoch_lights_nodes_buf);
    }
    stoch_lights_nodes_buf = {};
    for (Ren::BufferHandle &buf : rt_tlas_buf) {
        if (buf) {
            ctx.ReleaseBuffer(buf);
        }
        buf = {};
    }
    for (const Ren::BufferHandle b : hwrt.rt_blas_buffers) {
        ctx.ReleaseBuffer(b);
    }
    hwrt = {};
    if (swrt.rt_prim_indices_buf) {
        ctx.ReleaseBuffer(swrt.rt_prim_indices_buf);
    }
    if (swrt.rt_blas_buf) {
        ctx.ReleaseBuffer(swrt.rt_blas_buf);
    }
    swrt = {};

    std::fill(std::begin(rt_tlas), std::end(rt_tlas), nullptr);

    probe_irradiance = {};
    probe_distance = {};
    probe_offset = {};
    probe_volumes.clear();
}