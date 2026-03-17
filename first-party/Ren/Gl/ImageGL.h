#pragma once

#include <cstdint>
#include <cstring>

#include "../Buffer.h"
#include "../ImageParams.h"
#include "../math/Vec.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace Ren {
class ILog;

struct ImageMain {
    uint32_t img = 0;
    SmallVector<uint32_t, 1> views;
    uint32_t generation = 0;
    mutable eResState resource_state = eResState::Undefined;
};

struct ImageCold {
    String name;
    MemAllocation alloc;
    ImgParams params;
    uint32_t first_user = 0xffffffff;
};

bool Image_Init(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, String name, const ImgParams &p,
                const BufferMain *sbuf_main, const BufferCold *sbuf_cold, uint32_t data_off, CommandBuffer cmd_buf,
                MemAllocators *mem_allocs, ILog *log);
bool Image_Init(const ApiContext &api, ImageCold &img_cold, String name, const ImgParams &p, MemAllocation &&alloc,
                ILog *log);

void Image_Destroy(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold);
void Image_DestroyImmediately(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold);

bool Image_SetSampling(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, SamplingParams s, ILog *log);

void Image_SetSubImage(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, int layer, int level,
                       const Vec3i &offset, const Vec3i &size, eFormat format, const BufferMain &sbuf_main,
                       CommandBuffer cmd_buf, int data_off, int data_len);

int Image_AddView(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, eFormat format, int mip_level,
                  int mip_count, int base_layer, int layer_count);

void Image_CmdClear(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, const ClearColor &col,
                    CommandBuffer cmd_buf);
void Image_CmdCopyToBuffer(const ApiContext &api, const ImageMain &img_main, const ImageCold &img_cold,
                           BufferMain &buf_main, BufferCold &buf_cold, CommandBuffer cmd_buf, uint32_t data_off);
void Image_CmdCopyToImage(const ApiContext &api, CommandBuffer cmd_buf, const ImageMain &src_main,
                          const ImageCold &src_cold, uint32_t src_level, const Vec3i &src_offset,
                          const ImageMain &dst_main, const ImageCold &dst_cold, uint32_t dst_level,
                          const Vec3i &dst_offset, uint32_t dst_face, const Vec3i &size);

uint32_t GLFormatFromFormat(eFormat format);
uint32_t GLInternalFormatFromFormat(eFormat format);
uint32_t GLTypeFromFormat(eFormat format);

void GLUnbindTextureUnits(int start, int count);
} // namespace Ren

#ifdef _MSC_VER
#pragma warning(pop)
#endif
