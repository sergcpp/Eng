#pragma once

#include "../SamplingParams.h"
#include "../utils/SparseStorage.h"

namespace Ren {
struct ApiContext;

struct alignas(8) Sampler {
    uint32_t id = 0;
    SamplingParams params;

    bool operator<(const Sampler &rhs) const { return params < rhs.params; }
};

bool Sampler_Init(const ApiContext &api, Sampler &sampler, SamplingParams params);
void Sampler_Destroy(const ApiContext &api, Sampler &sampler);
void Sampler_DestroyImmediately(const ApiContext &api, Sampler &sampler);

void GLUnbindSamplers(int start, int count);
} // namespace Ren