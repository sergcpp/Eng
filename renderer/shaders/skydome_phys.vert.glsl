#version 320 es

#include "_vs_common.glsl"
#include "skydome_interface.h"

LAYOUT_PARAMS uniform UniformParams {
    Params g_params;
};

layout(location = VTX_POS_LOC) in vec3 g_in_vtx_pos;

layout(location = 0) out vec3 g_vtx_pos;

void main() {
    vec3 vertex_position_ws = (g_params.xform * vec4(g_in_vtx_pos, 1.0)).xyz;
    g_vtx_pos = vertex_position_ws;
    gl_Position = g_params.clip_from_world * vec4(vertex_position_ws, 1.0);
#if defined(VULKAN)
    gl_Position.y = -gl_Position.y;
#endif
}