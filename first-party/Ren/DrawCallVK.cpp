#include "DrawCall.h"

#include "AccStructure.h"
#include "Bindless.h"
#include "DescriptorPool.h"
#include "Log.h"
#include "Pipeline.h"
#include "ProbeStorage.h"
#include "Sampler.h"
#include "VKCtx.h"

VkDescriptorSet Ren::PrepareDescriptorSet(const ApiContext &api, const StoragesRef *storages,
                                          VkDescriptorSetLayout layout, Span<const Binding> bindings,
                                          DescrMultiPoolAlloc &descr_alloc, ILog *log) {
    VkDescriptorImageInfo sampler_infos[16] = {};
    VkDescriptorImageInfo img_sampler_infos[24];
    VkDescriptorImageInfo img_storage_infos[24];
    VkDescriptorBufferInfo ubuf_infos[24];
    VkDescriptorBufferInfo sbuf_infos[16];
    VkWriteDescriptorSetAccelerationStructureKHR desc_tlas_infos[16];
    DescrSizes descr_sizes;

    SmallVector<VkWriteDescriptorSet, 48> descr_writes;
    uint64_t used_bindings = 0;

    for (const auto &b : bindings) {
        if (b.trg == eBindTarget::Tex || b.trg == eBindTarget::TexSampled) {
            auto &info = img_sampler_infos[descr_sizes.img_sampler_count++];
            if (b.trg == eBindTarget::TexSampled) {
                if (b.handle.sampler) {
                    info.sampler = storages->samplers.Get(b.handle.sampler).first.handle;
                } else {
                    info.sampler = b.handle.img->handle().sampler;
                }
            }
            info.imageView = b.handle.img->handle().views[b.handle.view_index];
            info.imageLayout = VkImageLayout(VKImageLayoutForState(b.handle.img->resource_state));

            auto &new_write = descr_writes.emplace_back();
            new_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            new_write.dstBinding = b.loc;
            new_write.dstArrayElement = b.offset;
            new_write.descriptorType = (b.trg == eBindTarget::TexSampled) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                                                                          : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            new_write.descriptorCount = 1;
            new_write.pImageInfo = &info;

            assert((used_bindings & (1ull << (b.loc + b.offset))) == 0 && "Bindings overlap detected!");
            used_bindings |= (1ull << (b.loc + b.offset));
        } else if (b.trg == eBindTarget::UBuf) {
            const auto &[buf_main, buf_cold] = storages->buffers.Get(b.handle.buf);

            auto &ubuf = ubuf_infos[descr_sizes.ubuf_count++];
            ubuf.buffer = buf_main.buf;
            ubuf.offset = b.offset;
            ubuf.range = b.offset ? b.size : VK_WHOLE_SIZE;

            auto &new_write = descr_writes.emplace_back();
            new_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            new_write.dstBinding = b.loc;
            new_write.dstArrayElement = 0;
            new_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            new_write.descriptorCount = 1;
            new_write.pBufferInfo = &ubuf;

            assert((used_bindings & (1ull << b.loc)) == 0 && "Bindings overlap detected!");
            used_bindings |= (1ull << b.loc);
        } else if (b.trg == eBindTarget::UTBuf) {
            ++descr_sizes.utbuf_count;

            const auto &[buf_main, buf_cold] = storages->buffers.Get(b.handle.buf);

            auto &new_write = descr_writes.emplace_back();
            new_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            new_write.dstBinding = b.loc;
            new_write.dstArrayElement = 0;
            new_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            new_write.descriptorCount = 1;
            new_write.pTexelBufferView = &buf_main.views[b.handle.view_index].second;

            assert((used_bindings & (1ull << b.loc)) == 0 && "Bindings overlap detected!");
            used_bindings |= (1ull << b.loc);
        } else if (b.trg == eBindTarget::SBufRO || b.trg == eBindTarget::SBufRW) {
            const auto &[buf_main, buf_cold] = storages->buffers.Get(b.handle.buf);

            auto &sbuf = sbuf_infos[descr_sizes.sbuf_count++];
            sbuf.buffer = buf_main.buf;
            sbuf.offset = b.offset;
            sbuf.range = (b.offset || b.size) ? (b.size ? b.size : (buf_cold.size - b.offset)) : VK_WHOLE_SIZE;

            auto &new_write = descr_writes.emplace_back();
            new_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            new_write.dstBinding = b.loc;
            new_write.dstArrayElement = 0;
            new_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            new_write.descriptorCount = 1;
            new_write.pBufferInfo = &sbuf;

            assert((used_bindings & (1ull << b.loc)) == 0 && "Bindings overlap detected!");
            used_bindings |= (1ull << b.loc);
        } else if (b.trg == eBindTarget::STBufRO || b.trg == eBindTarget::STBufRW) {
            ++descr_sizes.stbuf_count;

            const auto &[buf_main, buf_cold] = storages->buffers.Get(b.handle.buf);

            auto &new_write = descr_writes.emplace_back();
            new_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            new_write.dstBinding = b.loc;
            new_write.dstArrayElement = 0;
            new_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            new_write.descriptorCount = 1;
            new_write.pTexelBufferView = &buf_main.views[b.handle.view_index].second;

            assert((used_bindings & (1ull << b.loc)) == 0 && "Bindings overlap detected!");
            used_bindings |= (1ull << b.loc);
        } else if (b.trg == eBindTarget::ImageRO || b.trg == eBindTarget::ImageRW) {
            auto &info = img_storage_infos[descr_sizes.store_img_count++];
            info.sampler = b.handle.img->handle().sampler;
            info.imageView = b.handle.img->handle().views[b.handle.view_index];
            info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            auto &new_write = descr_writes.emplace_back();
            new_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            new_write.dstBinding = b.loc;
            new_write.dstArrayElement = b.offset;
            new_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            new_write.descriptorCount = b.size ? b.size : 1;
            new_write.pImageInfo = &info;

            assert((used_bindings & (1ull << (b.loc + b.offset))) == 0 && "Bindings overlap detected!");
            used_bindings |= (1ull << (b.loc + b.offset));
        } else if (b.trg == eBindTarget::Sampler) {
            auto &info = sampler_infos[descr_sizes.sampler_count++];
            info.sampler = storages->samplers.Get(b.handle.sampler).first.handle;

            auto &new_write = descr_writes.emplace_back();
            new_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            new_write.dstBinding = b.loc;
            new_write.dstArrayElement = b.offset;
            new_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            new_write.descriptorCount = 1;
            new_write.pImageInfo = &info;
        } else if (b.trg == eBindTarget::AccStruct) {
            auto &info = desc_tlas_infos[descr_sizes.acc_count++];
            info = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
            info.pAccelerationStructures = &b.handle.acc_struct->vk_handle();
            info.accelerationStructureCount = 1;

            auto &new_write = descr_writes.emplace_back();
            new_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            new_write.dstBinding = b.loc;
            new_write.dstArrayElement = 0;
            new_write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            new_write.descriptorCount = 1;
            new_write.pNext = &info;

            assert((used_bindings & (1ull << b.loc)) == 0 && "Bindings overlap detected!");
            used_bindings |= (1ull << b.loc);
        }
    }

    VkDescriptorSet descr_set = descr_alloc.Alloc(descr_sizes, layout);
    if (!descr_set) {
        log->Error("Failed to allocate descriptor set!");
        return VK_NULL_HANDLE;
    }

    for (auto &d : descr_writes) {
        d.dstSet = descr_set;
    }

    api.vkUpdateDescriptorSets(api.device, descr_writes.size(), descr_writes.data(), 0, nullptr);

    return descr_set;
}

void Ren::DispatchCompute(CommandBuffer cmd_buf, const PipelineHandle pipeline, const StoragesRef &storages,
                          const Vec3u grp_count, Span<const Binding> bindings, const void *uniform_data,
                          const int uniform_data_len, DescrMultiPoolAlloc &descr_alloc, ILog *log) {
    const ApiContext &api = descr_alloc.api();

    const auto &[pi_main, pi_cold] = storages.pipelines.Get(pipeline);
    const auto &[prog_main, prog_cold] = storages.programs.Get(pi_main.prog);

    SmallVector<VkDescriptorSet, 2> descr_sets;
    descr_sets.push_back(
        PrepareDescriptorSet(api, &storages, prog_main.descr_set_layouts[0], bindings, descr_alloc, log));
    if (!descr_sets.back()) {
        log->Error("Failed to allocate descriptor set, skipping draw call!");
        return;
    }
    for (const Binding &b : bindings) {
        if (b.trg == eBindTarget::BindlessDescriptors) {
            descr_sets.push_back(b.handle.bindless->descr_set);
        } else if (b.trg == eBindTarget::SBufRO) {
            assert(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly);
        } else if (b.trg == eBindTarget::SBufRW) {
            assert(!(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly));
        } else if (b.trg == eBindTarget::STBufRO) {
            assert(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly);
        } else if (b.trg == eBindTarget::STBufRW) {
            assert(!(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly));
        } else if (b.trg == eBindTarget::ImageRO) {
            assert(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly);
        } else if (b.trg == eBindTarget::ImageRW) {
            assert(!(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly));
        }
    }

    api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi_main.handle);
    api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi_main.layout, 0, descr_sets.size(),
                                descr_sets.data(), 0, nullptr);

    if (uniform_data) {
        api.vkCmdPushConstants(cmd_buf, pi_main.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, uniform_data_len, uniform_data);
    }

    api.vkCmdDispatch(cmd_buf, grp_count[0], grp_count[1], grp_count[2]);
}

void Ren::DispatchCompute(const PipelineHandle pipeline, const StoragesRef &storages, const Vec3u grp_count,
                          Span<const Binding> bindings, const void *uniform_data, const int uniform_data_len,
                          DescrMultiPoolAlloc &descr_alloc, ILog *log) {
    const ApiContext &api = descr_alloc.api();
    const VkCommandBuffer cmd_buf = api.draw_cmd_buf[api.backend_frame];
    DispatchCompute(cmd_buf, pipeline, storages, grp_count, bindings, uniform_data, uniform_data_len, descr_alloc, log);
}

void Ren::DispatchComputeIndirect(CommandBuffer cmd_buf, const PipelineHandle pipeline, const StoragesRef &storages,
                                  const BufferROHandle indir_buf, const uint32_t indir_buf_offset,
                                  Span<const Binding> bindings, const void *uniform_data, int uniform_data_len,
                                  DescrMultiPoolAlloc &descr_alloc, ILog *log) {
    const ApiContext &api = descr_alloc.api();

    const auto &[pi_main, pi_cold] = storages.pipelines.Get(pipeline);
    const auto &[prog_main, prog_cold] = storages.programs.Get(pi_main.prog);

    SmallVector<VkDescriptorSet, 2> descr_sets;
    descr_sets.push_back(
        PrepareDescriptorSet(api, &storages, prog_main.descr_set_layouts[0], bindings, descr_alloc, log));
    if (!descr_sets.back()) {
        log->Error("Failed to allocate descriptor set, skipping draw call!");
        return;
    }
    for (const Binding &b : bindings) {
        if (b.trg == eBindTarget::BindlessDescriptors) {
            descr_sets.push_back(b.handle.bindless->descr_set);
        } else if (b.trg == eBindTarget::SBufRO) {
            assert(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly);
        } else if (b.trg == eBindTarget::SBufRW) {
            assert(!(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly));
        } else if (b.trg == eBindTarget::STBufRO) {
            assert(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly);
        } else if (b.trg == eBindTarget::STBufRW) {
            assert(!(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly));
        } else if (b.trg == eBindTarget::ImageRO) {
            assert(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly);
        } else if (b.trg == eBindTarget::ImageRW) {
            assert(!(prog_cold.uniform_at(b.loc).flags & eDescrFlags::ReadOnly));
        }
    }

    api.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi_main.handle);
    api.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pi_main.layout, 0, descr_sets.size(),
                                descr_sets.data(), 0, nullptr);

    if (uniform_data) {
        api.vkCmdPushConstants(cmd_buf, pi_main.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, uniform_data_len, uniform_data);
    }

    const BufferMain indir_buf_main = storages.buffers.Get(indir_buf).first;
    api.vkCmdDispatchIndirect(cmd_buf, indir_buf_main.buf, VkDeviceSize(indir_buf_offset));
}
