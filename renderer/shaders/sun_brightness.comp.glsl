#version 320 es

#if defined(GL_ES) || defined(VULKAN) || defined(GL_SPIRV)
    precision highp int;
    precision highp float;
#endif

#include "_cs_common.glsl"
#include "sun_brightness_interface.h"
#include "atmosphere_common.glsl"

layout(binding = TRANSMITTANCE_LUT_SLOT) uniform sampler2D g_trasmittance_lut;
layout(binding = MULTISCATTER_LUT_SLOT) uniform sampler2D g_multiscatter_lut;

layout(binding = MOON_TEX_SLOT) uniform sampler2D g_moon_tex;
layout(binding = WEATHER_TEX_SLOT) uniform sampler2D g_weather_tex;
layout(binding = CIRRUS_TEX_SLOT) uniform sampler2D g_cirrus_tex;
layout(binding = NOISE3D_TEX_SLOT) uniform sampler3D g_noise3d_tex;

layout(std430, binding = OUT_BUF_SLOT) writeonly buffer OutBuf {
    vec3 g_out_buf;
};

shared uvec3 g_avg_transmittance;

layout (local_size_x = LOCAL_GROUP_SIZE_X, local_size_y = LOCAL_GROUP_SIZE_Y, local_size_z = 1) in;

void main() {
    if (gl_LocalInvocationIndex == 0) {
        g_avg_transmittance = uvec3(0.0);
    }

    const vec2 u = vec2(float(gl_LocalInvocationID.x) / LOCAL_GROUP_SIZE_X,
                        float(gl_LocalInvocationID.y) / LOCAL_GROUP_SIZE_Y);
    const vec3 sample_dir = MapToCone(u, g_shrd_data.sun_dir.xyz, g_shrd_data.sun_dir.w);

    vec3 transmittance;
    IntegrateScattering(vec3(0.0, g_shrd_data.atmosphere.viewpoint_height, 0.0), sample_dir, FLT_MAX, 0,
                        g_trasmittance_lut, g_multiscatter_lut, g_moon_tex, g_weather_tex, g_cirrus_tex, g_noise3d_tex, transmittance);

    barrier(); groupMemoryBarrier();

    atomicAdd(g_avg_transmittance.x, uint(transmittance.x * 10000.0));
    atomicAdd(g_avg_transmittance.y, uint(transmittance.y * 10000.0));
    atomicAdd(g_avg_transmittance.z, uint(transmittance.z * 10000.0));

    barrier(); groupMemoryBarrier();

    if (gl_LocalInvocationIndex == 0) {
        g_out_buf = g_shrd_data.sun_col.xyz * vec3(g_avg_transmittance) / 640000.0;
    }
}