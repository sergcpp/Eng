#pragma once

#include <cstdint>

#include <functional>
#include <string_view>

#include "Fwd.h"
#include "Image.h"
#include "Program.h"
#include "Sampler.h"
#include "math/Vec.h"
#include "utils/SmallVector.h"
#include "utils/Storage.h"
#include "utils/String.h"

namespace Ren {
class ILog;

enum class eMatFlags : uint8_t { AlphaTest, AlphaBlend, DepthWrite, TwoSided, Emissive, CustomShaded };

using texture_load_callback =
    std::function<ImageHandle(std::string_view name, const uint8_t color[4], Bitmask<eImgFlags> flags)>;
using sampler_load_callback = std::function<SamplerHandle(SamplingParams params)>;
using pipelines_load_callback =
    std::function<void(Bitmask<eMatFlags> flags, std::string_view arg1, std::string_view arg2, std::string_view arg3,
                       std::string_view arg4, SmallVectorImpl<PipelineHandle> &out_pipelines)>;

struct MaterialMain {
    Bitmask<eMatFlags> flags;
    SmallVector<PipelineHandle, 2> pipelines;
    SmallVector<ImageHandle, 4> textures;
    SmallVector<SamplerHandle, 4> samplers;
};

struct MaterialCold {
    String name;
    SmallVector<Vec4f, 4> params;
    SmallVector<uint32_t, 4> next_texture_user;
};

bool Material_Init(MaterialMain &mat_main, MaterialCold &mat_cold, Ren::String name, Bitmask<eMatFlags> flags,
                   Span<const PipelineHandle> pipelines, Span<const ImageHandle> textures,
                   Span<const SamplerHandle> samplers, Span<const Vec4f> params, ILog *log);
bool Material_Init(MaterialMain &mat_main, MaterialCold &mat_cold, Ren::String name, std::string_view mat_src,
                   const pipelines_load_callback &on_pipes_load, const texture_load_callback &on_tex_load,
                   const sampler_load_callback &on_sampler_load, ILog *log);
} // namespace Ren
