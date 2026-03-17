#pragma once

#include "../SamplingParams.h"
#include "../utils/Storage.h"

namespace Ren {
struct ApiContext;

struct SamplerMain {
    uint32_t id = 0;
    SamplingParams params;

    bool operator<(const SamplerMain &rhs) const { return params < rhs.params; }
};

struct SamplerCold {
    // TODO:
};

bool Sampler_Init(const ApiContext &api, SamplerMain &sampler_main, SamplerCold &sampler_cold, SamplingParams params);
void Sampler_Destroy(const ApiContext &api, SamplerMain &sampler_main, SamplerCold &sampler_cold);
void Sampler_DestroyImmediately(const ApiContext &api, SamplerMain &sampler_main, SamplerCold &sampler_cold);

void GLUnbindSamplers(int start, int count);
} // namespace Ren