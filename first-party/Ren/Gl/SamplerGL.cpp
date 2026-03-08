#include "SamplerGL.h"

#include "GL.h"

namespace Ren {
#define X(_0, _1, _2, _3, _4) _3,
extern const uint32_t g_min_filter_gl[] = {
#include "../Filter.inl"
};
#undef X

#define X(_0, _1, _2, _3, _4) _4,
extern const uint32_t g_mag_filter_gl[] = {
#include "../Filter.inl"
};
#undef X

#define X(_0, _1, _2) _2,
extern const uint32_t g_wrap_mode_gl[] = {
#include "../Wrap.inl"
};
#undef X

#define X(_0, _1, _2) _2,
extern const uint32_t g_compare_func_gl[] = {
#include "../CompareOp.inl"
};
#undef X

extern const float AnisotropyLevel = 4;
} // namespace Ren

bool Ren::Sampler_Init(const ApiContext &, SamplerMain &sampler_main, SamplerCold &sampler_cold,
                       const SamplingParams params) {
    GLuint new_sampler;
    glGenSamplers(1, &new_sampler);

    glSamplerParameteri(new_sampler, GL_TEXTURE_MIN_FILTER, g_min_filter_gl[int(params.filter)]);
    glSamplerParameteri(new_sampler, GL_TEXTURE_MAG_FILTER, g_mag_filter_gl[int(params.filter)]);

    glSamplerParameteri(new_sampler, GL_TEXTURE_WRAP_S, g_wrap_mode_gl[int(params.wrap)]);
    glSamplerParameteri(new_sampler, GL_TEXTURE_WRAP_T, g_wrap_mode_gl[int(params.wrap)]);
    glSamplerParameteri(new_sampler, GL_TEXTURE_WRAP_R, g_wrap_mode_gl[int(params.wrap)]);

    if (params.compare != eCompareOp::None) {
        glSamplerParameteri(new_sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(new_sampler, GL_TEXTURE_COMPARE_FUNC, g_compare_func_gl[int(params.compare)]);
    } else {
        glSamplerParameteri(new_sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

#ifndef __ANDROID__
    glSamplerParameterf(new_sampler, GL_TEXTURE_LOD_BIAS, params.lod_bias.to_float());
#endif

    // glSamplerParameterf(new_sampler, GL_TEXTURE_MIN_LOD, params.min_lod.to_float());
    // glSamplerParameterf(new_sampler, GL_TEXTURE_MAX_LOD, params.max_lod.to_float());

    glSamplerParameterf(new_sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, AnisotropyLevel);

    sampler_main.id = uint32_t(new_sampler);
    sampler_main.params = params;

    return true;
}

void Ren::Sampler_Destroy(const ApiContext &, SamplerMain &sampler_main, SamplerCold &sampler_cold) {
    if (sampler_main.id) {
        GLuint id = GLuint(sampler_main.id);
        glDeleteSamplers(1, &id);
    }
    sampler_main = {};
    sampler_cold = {};
}

void Ren::Sampler_DestroyImmediately(const ApiContext &api, SamplerMain &sampler_main, SamplerCold &sampler_cold) {
    Sampler_Destroy(api, sampler_main, sampler_cold);
}

void Ren::GLUnbindSamplers(const int start, const int count) {
    for (int i = start; i < start + count; i++) {
        glBindSampler(GLuint(i), 0);
    }
}
