#pragma once

#include "SamplingParams.h"
#include "Storage.h"

namespace Ren {
struct ApiContext;

struct SamplerMain {
    uint32_t id = 0;
    SamplingParamsPacked params;

    bool operator<(const SamplerMain &rhs) const { return params < rhs.params; }
};

struct SamplerCold {
    // TODO:
};

bool Sampler_Init(const ApiContext &api, SamplerMain &sampler_main, SamplerCold &sampler_cold, SamplingParams params);
void Sampler_Destroy(const ApiContext &api, SamplerMain &sampler_main, SamplerCold &sampler_cold);

class Sampler : public RefCounter {
    uint32_t id_ = 0;
    SamplingParamsPacked params_;

    void Destroy();

  public:
    Sampler() = default;
    Sampler(const Sampler &rhs) = delete;
    Sampler(Sampler &&rhs) noexcept { (*this) = std::move(rhs); }
    ~Sampler() { Destroy(); }

    uint32_t id() const { return id_; }
    SamplingParams params() const { return params_; }

    Sampler &operator=(const Sampler &rhs) = delete;
    Sampler &operator=(Sampler &&rhs);

    void Init(ApiContext *api, SamplingParams params);
};

void GLUnbindSamplers(int start, int count);
} // namespace Ren