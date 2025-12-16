#pragma once

#include "SamplingParams.h"
#include "Storage.h"
#include "VK.h"

namespace Ren {
struct ApiContext;

struct SamplerMain {
    VkSampler handle = {};
    SamplingParamsPacked params;

    bool operator<(const SamplerMain &rhs) const { return params < rhs.params; }
};

struct SamplerCold {
    // TODO:
};

bool Sampler_Init(const ApiContext &api, SamplerMain &sampler_main, SamplerCold &sampler_cold, SamplingParams params);
void Sampler_Destroy(const ApiContext &api, SamplerMain &sampler_main, SamplerCold &sampler_cold);
} // namespace Ren