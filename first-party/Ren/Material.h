#pragma once

#include <cstdint>

#include <functional>
#include <string_view>

#include "Fwd.h"
#include "Image.h"
#include "Program.h"
#include "Sampler.h"
#include "SmallVector.h"
#include "Storage.h"
#include "String.h"

#include "MVec.h"

namespace Ren {
class ILog;

enum class eMatFlags { AlphaTest, AlphaBlend, DepthWrite, TwoSided, Emissive, CustomShaded };

enum class eMatLoadStatus { Found, SetToDefault, CreatedFromData, CreatedFromData_NeedsMore };

using texture_load_callback =
    std::function<ImgRef(std::string_view name, const uint8_t color[4], Bitmask<eImgFlags> flags)>;
using sampler_load_callback = std::function<SamplerHandle(SamplingParams params)>;
using pipelines_load_callback =
    std::function<void(Bitmask<eMatFlags> flags, std::string_view arg1, std::string_view arg2, std::string_view arg3,
                       std::string_view arg4, SmallVectorImpl<PipelineHandle> &out_pipelines)>;

struct MaterialMain {
    Bitmask<eMatFlags> flags;
    SmallVector<PipelineHandle, 2> pipelines;
};

struct MaterialCold {
    String name;
    SmallVector<Vec4f, 4> params;
    SmallVector<uint32_t, 4> next_texture_user;
};

class Material : public RefCounter {
    bool ready_ = false;
    String name_;

    void InitFromMAT(std::string_view mat_src, eMatLoadStatus *status, const pipelines_load_callback &on_pipes_load,
                     const texture_load_callback &on_tex_load, const sampler_load_callback &on_sampler_load, ILog *log);

  public:
    Bitmask<eMatFlags> flags;
    SmallVector<PipelineHandle, 4> pipelines;
    SmallVector<ImgRef, 4> textures;
    SmallVector<SamplerHandle, 4> samplers;
    SmallVector<Vec4f, 4> params;
    SmallVector<uint32_t, 4> next_texture_user;

    Material() = default;
    Material(std::string_view name, std::string_view mat_src, eMatLoadStatus *status,
             const pipelines_load_callback &on_pipes_load, const texture_load_callback &on_tex_load,
             const sampler_load_callback &on_sampler_load, ILog *log);
    Material(std::string_view name, Bitmask<eMatFlags> flags, Span<const PipelineHandle> pipelines,
             Span<const ImgRef> textures, Span<const SamplerHandle> samplers, Span<const Vec4f> params, ILog *log);

    Material(const Mesh &rhs) = delete;
    Material(Material &&rhs) = default;

    Material &operator=(const Material &rhs) = delete;
    Material &operator=(Material &&rhs) noexcept = default;

    bool ready() const { return ready_; }
    const String &name() const { return name_; }

    void Init(Bitmask<eMatFlags> flags, Span<const PipelineHandle> _pipelines, Span<const ImgRef> _textures,
              Span<const SamplerHandle> _samplers, Span<const Vec4f> _params, ILog *log);
    void Init(std::string_view mat_src, eMatLoadStatus *status, const pipelines_load_callback &on_pipes_load,
              const texture_load_callback &on_tex_load, const sampler_load_callback &on_sampler_load, ILog *log);
};

using MaterialRef = StrongRef<Material, NamedStorage<Material>>;
using MaterialStorage = NamedStorage<Material>;
} // namespace Ren
