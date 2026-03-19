#pragma once

#include "../SamplingParams.h"
#include "../utils/SparseStorage.h"
#include "VK.h"

namespace Ren {
struct ApiContext;

struct Sampler {
    VkSampler handle = {};
    SamplingParams params;

    bool operator<(const Sampler &rhs) const { return params < rhs.params; }
};

bool Sampler_Init(const ApiContext &api, Sampler &sampler, SamplingParams params);
void Sampler_Destroy(const ApiContext &api, Sampler &sampler);
void Sampler_DestroyImmediately(const ApiContext &api, Sampler &sampler);
} // namespace Ren