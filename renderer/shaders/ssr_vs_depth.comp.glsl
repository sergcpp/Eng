#version 310 es

#if defined(GL_ES) || defined(VULKAN) || defined(GL_SPIRV)
    precision highp int;
    precision highp float;
#endif

#include "_cs_common.glsl"
#include "ssr_vs_depth_interface.h"

/*
UNIFORM_BLOCKS
    SharedDataBlock : $ubSharedDataLoc
    UniformParams : $ubUnifParamLoc
*/

LAYOUT_PARAMS uniform UniformParams {
    Params g_params;
};

#if defined(VULKAN) || defined(GL_SPIRV)
layout (binding = REN_UB_SHARED_DATA_LOC, std140)
#else
layout (std140)
#endif
uniform SharedDataBlock {
    SharedData g_shrd_data;
};

layout(binding = DEPTH_TEX_SLOT) uniform highp sampler2D g_depth_tex;
layout(binding = OUT_VS_DEPTH_IMG_SLOT, r32f) uniform image2D out_vs_depth_img;

layout(local_size_x = LOCAL_GROUP_SIZE_X, local_size_y = LOCAL_GROUP_SIZE_Y, local_size_z = 1) in;

void main() {
    ivec2 pix_uvs = ivec2(gl_GlobalInvocationID.xy);
    if (pix_uvs.x >= g_params.resolution.x || pix_uvs.y >= g_params.resolution.y) {
        return;
    }

    float vs_depth = LinearizeDepth(texelFetch(g_depth_tex, pix_uvs, 0).r, g_shrd_data.clip_info);
    imageStore(out_vs_depth_img, pix_uvs, vec4(vs_depth));
}

