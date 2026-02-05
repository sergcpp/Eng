#pragma once

#include <cstdint>

#include "Fwd.h"
#include "MVec.h"
#include "Span.h"

namespace Ren {
struct ApiContext;
class DescrMultiPoolAlloc;
class ILog;
class Image;
struct BindlessDescriptors;

enum class eBindTarget : uint16_t {
    Tex,
    TexSampled,
    Sampler,
    UBuf,
    UTBuf,
    SBufRO,
    SBufRW,
    STBufRO,
    STBufRW,
    ImageRO,
    ImageRW,
    AccStruct,
    BindlessDescriptors,
    _Count
};

#if defined(REN_GL_BACKEND)
uint32_t GLBindTarget(const Image &img, int view);
uint32_t GLBindTarget(const ImageCold &img, int view);
extern int g_param_buf_binding;
#endif

struct OpaqueHandle {
    union {
        const Image *img;
        const BufferROHandle buf;
        const BindlessDescriptors *bindless;
#if defined(REN_VK_BACKEND)
        const AccStructureVK *acc_struct;
#endif
        void *ptr;
    };
    union {
        const ImageROHandle img_new;
        const Handle<void> handle;
    };
    const SamplerROHandle sampler = {};
    int view_index = 0;
    bool readonly = false;

    OpaqueHandle(const Image &_img, int _view_index = 0) : img(&_img), handle(), view_index(_view_index) {}
    OpaqueHandle(const Image &_img, const SamplerROHandle _sampler, int _view_index = 0)
        : img(&_img), handle(), sampler(_sampler), view_index(_view_index) {}
    OpaqueHandle(const BufferROHandle _buf, int _view_index = 0)
        : buf(_buf), handle(), view_index(_view_index), readonly(true) {}
    OpaqueHandle(const BufferRWHandle _buf, int _view_index = 0) : buf(_buf), handle(), view_index(_view_index) {}

    OpaqueHandle(const ImageROHandle _img, int _view_index = 0)
        : ptr(nullptr), img_new(_img), view_index(_view_index), readonly(true) {}
    OpaqueHandle(const ImageRWHandle _img, int _view_index = 0)
        : ptr(nullptr), img_new(_img), view_index(_view_index) {}
    OpaqueHandle(const ImageROHandle _img, const SamplerROHandle _sampler, int _view_index = 0)
        : ptr(nullptr), img_new(_img), sampler(_sampler), view_index(_view_index), readonly(true) {}
    OpaqueHandle(const ImageRWHandle _img, const SamplerROHandle _sampler, int _view_index = 0)
        : ptr(nullptr), img_new(_img), sampler(_sampler), view_index(_view_index) {}

    OpaqueHandle(const SamplerROHandle _sampler) : ptr(nullptr), handle(), sampler(_sampler) {}
    OpaqueHandle(const BindlessDescriptors &_bindless) : bindless(&_bindless), handle() {}
#if defined(REN_VK_BACKEND)
    OpaqueHandle(const AccStructureVK &_acc_struct) : acc_struct(&_acc_struct), handle() {}
#endif
};

struct Binding {
    eBindTarget trg;
    uint16_t loc = 0;
    uint16_t offset = 0;
    uint16_t size = 0;
    OpaqueHandle handle;

    Binding(const eBindTarget _trg, const int _loc, OpaqueHandle _handle) : trg(_trg), loc(_loc), handle(_handle) {}
    Binding(const eBindTarget _trg, const int _loc, const size_t _offset, OpaqueHandle _handle)
        : trg(_trg), loc(_loc), offset(uint16_t(_offset)), handle(_handle) {}
    Binding(const eBindTarget _trg, const int _loc, const size_t _offset, const size_t _size, OpaqueHandle _handle)
        : trg(_trg), loc(_loc), offset(uint16_t(_offset)), size(uint16_t(_size)), handle(_handle) {}
};
static_assert(sizeof(Binding) == sizeof(void *) + sizeof(void *) + 8 + 8 + 8);

#if defined(REN_VK_BACKEND)
[[nodiscard]] VkDescriptorSet PrepareDescriptorSet(const ApiContext &api, const StoragesRef &storages,
                                                   VkDescriptorSetLayout layout, Span<const Binding> bindings,
                                                   DescrMultiPoolAlloc &descr_alloc, ILog *log);
#endif

void DispatchCompute(CommandBuffer cmd_buf, PipelineHandle pipeline, const StoragesRef &storages, Vec3u grp_count,
                     Span<const Binding> bindings, const void *uniform_data, int uniform_data_len,
                     DescrMultiPoolAlloc &descr_alloc, ILog *log);
void DispatchCompute(PipelineHandle pipeline, const StoragesRef &storages, Vec3u grp_count,
                     Span<const Binding> bindings, const void *uniform_data, int uniform_data_len,
                     DescrMultiPoolAlloc &descr_alloc, ILog *log);

void DispatchComputeIndirect(CommandBuffer cmd_buf, PipelineHandle pipeline, const StoragesRef &storages,
                             BufferROHandle indir_buf, uint32_t indir_buf_offset, Span<const Binding> bindings,
                             const void *uniform_data, int uniform_data_len, DescrMultiPoolAlloc &descr_alloc,
                             ILog *log);
} // namespace Ren