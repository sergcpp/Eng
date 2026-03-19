#include "SamplerVK.h"

#include "VKCtx.h"

namespace Ren {
#define X(_0, _1, _2, _3, _4) _1,
extern const VkFilter g_min_mag_filter_vk[] = {
#include "../Filter.inl"
};
#undef X

#define X(_0, _1, _2, _3, _4) _2,
extern const VkSamplerMipmapMode g_mipmap_mode_vk[] = {
#include "../Filter.inl"
};
#undef X

#define X(_0, _1, _2) _1,
extern const VkSamplerAddressMode g_wrap_mode_vk[] = {
#include "../Wrap.inl"
};
#undef X

#define X(_0, _1, _2) _1,
extern const VkCompareOp g_compare_ops_vk[] = {
#include "../CompareOp.inl"
};
#undef X

extern const float AnisotropyLevel = 4;
} // namespace Ren

bool Ren::Sampler_Init(const ApiContext &api, Sampler &sampler, const SamplingParams params) {
    VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler_info.magFilter = g_min_mag_filter_vk[size_t(params.filter)];
    sampler_info.minFilter = g_min_mag_filter_vk[size_t(params.filter)];
    sampler_info.addressModeU = g_wrap_mode_vk[size_t(params.wrap)];
    sampler_info.addressModeV = g_wrap_mode_vk[size_t(params.wrap)];
    sampler_info.addressModeW = g_wrap_mode_vk[size_t(params.wrap)];
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = AnisotropyLevel;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = (params.compare != eCompareOp::None) ? VK_TRUE : VK_FALSE;
    sampler_info.compareOp = g_compare_ops_vk[size_t(params.compare)];
    sampler_info.mipmapMode = g_mipmap_mode_vk[size_t(params.filter)];
    sampler_info.mipLodBias = params.lod_bias.to_float();
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = VK_LOD_CLAMP_NONE;

    sampler.params = params;

    const VkResult res = api.vkCreateSampler(api.device, &sampler_info, nullptr, &sampler.handle);
    return (res == VK_SUCCESS);
}

void Ren::Sampler_Destroy(const ApiContext &api, Sampler &sampler) {
    if (sampler.handle) {
        api.samplers_to_destroy[api.backend_frame].emplace_back(sampler.handle);
    }
    sampler = {};
}

void Ren::Sampler_DestroyImmediately(const ApiContext &api, Sampler &sampler) {
    if (sampler.handle) {
        api.vkDestroySampler(api.device, sampler.handle, nullptr);
    }
    sampler = {};
}
