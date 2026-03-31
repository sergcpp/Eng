#pragma once

#include "Fwd.h"
#include "ImageParams.h"
#include "utils/Span.h"
#include "utils/String.h"

namespace Ren {
class ImageAtlasArray;

struct ImageRegionMain {
    ImageAtlasArray *atlas = nullptr;
    int pos[3] = {};
};

struct ImageRegionCold {
    String name;
    ImgParams params;
};

using ImageRegionHandle = Handle<ImageRegionMain, 0>;

bool ImageRegion_Init(ImageRegionMain &reg_main, ImageRegionCold &reg_cold, String name, Span<const uint8_t> data,
                      const ImgParams &p, CommandBuffer cmd_buf, ImageAtlasArray *atlas, ILog *log);
bool ImageRegion_Init(ImageRegionMain &reg_main, ImageRegionCold &reg_cold, String name, const BufferMain &sbuf,
                      int data_off, int data_len, const ImgParams &p, CommandBuffer cmd_buf, ImageAtlasArray *atlas,
                      ILog *log);

bool ImageRegion_InitFromDDSFile(ImageRegionMain &reg_main, ImageRegionCold &reg_cold, String name,
                                 Span<const uint8_t> data, ImgParams p, ImageAtlasArray *atlas, ILog *log);
bool ImageRegion_InitFromRAWData(ImageRegionMain &reg_main, ImageRegionCold &reg_cold, String name,
                                 const BufferMain &sbuf, int data_off, int data_len, CommandBuffer cmd_buf,
                                 const ImgParams &p, ImageAtlasArray *atlas);
} // namespace Ren