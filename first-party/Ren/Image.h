#pragma once

#include <cstdint>
#undef Always

#include "ImageParams.h"
#include "SamplingParams.h"
#include "Utils.h"

namespace Ren {
class Context;

int GetBlockLenBytes(eFormat format);
int GetBlockCount(int w, int h, eFormat format);
int GetDataLenBytes(int w, int h, int d, eFormat format);
inline int GetDataLenBytes(const int w, const int h, const eFormat format) { return GetDataLenBytes(w, h, 1, format); }
uint32_t GetDataLenBytes(const ImgParams &params);

eFormat FormatFromGLInternalFormat(uint32_t gl_internal_format, bool *is_srgb);
int BlockLenFromGLInternalFormat(uint32_t gl_internal_format);

void ParseDDSHeader(const DDSHeader &hdr, ImgParams *params);
} // namespace Ren

#if defined(REN_GL_BACKEND)
#include "ImageGL.h"
#elif defined(REN_VK_BACKEND)
#include "ImageVK.h"
#endif

namespace Ren {
using ImgRef = StrongRef<Image, NamedStorage<Image>>;
using WeakImgRef = WeakRef<Image, NamedStorage<Image>>;
using ImageStorage = NamedStorage<Image>;

bool Image_Init(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, String name, const ImgParams &p,
                Span<const uint8_t> data, MemAllocators *mem_allocs, ILog *log);
bool Image_Init(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, String name, const ImgParams &p,
                Span<const uint8_t> data[6], MemAllocators *mem_allocs, ILog *log);

eImgUsage ImgUsageFromState(eResState state);
} // namespace Ren
