#version 320 es
#extension GL_ARB_shading_language_packing : require

#if defined(GL_ES) || defined(VULKAN) || defined(GL_SPIRV)
    precision highp int;
    precision highp float;
#endif

#include "_cs_common.glsl"
#include "gi_common.glsl"
#include "ssr_common.glsl"
#include "ssr_blur_interface.h"

#pragma multi_compile _ PER_PIXEL_KERNEL_ROTATION

layout (binding = BIND_UB_SHARED_DATA_BUF, std140) uniform SharedDataBlock {
    SharedData g_shrd_data;
};

LAYOUT_PARAMS uniform UniformParams {
    Params g_params;
};

layout(binding = DEPTH_TEX_SLOT) uniform sampler2D g_depth_tex;
layout(binding = NORM_TEX_SLOT) uniform usampler2D g_normal_tex;
layout(binding = REFL_TEX_SLOT) uniform sampler2D g_refl_tex;
layout(binding = SAMPLE_COUNT_TEX_SLOT) uniform sampler2D g_sample_count_tex;
layout(binding = VARIANCE_TEX_SLOT) uniform sampler2D g_variance_tex;

layout(std430, binding = TILE_LIST_BUF_SLOT) readonly buffer TileList {
    uint g_tile_list[];
};

layout(binding = OUT_DENOISED_IMG_SLOT, rgba16f) uniform image2D g_out_denoised_img;

layout (local_size_x = LOCAL_GROUP_SIZE_X, local_size_y = LOCAL_GROUP_SIZE_Y, local_size_z = 1) in;

#define PREFILTER_NORMAL_SIGMA 512.0

/* mediump */ float GetEdgeStoppingNormalWeight(/* mediump */ vec3 normal_p, /* mediump */ vec3 normal_q) {
    return pow(clamp(dot(normal_p, normal_q), 0.0, 1.0), PREFILTER_NORMAL_SIGMA);
}

// http://marc-b-reynolds.github.io/quaternions/2016/07/06/Orthonormal.html
mat3 GetBasis(vec3 N) {
    float sz = sign(N.z);
    float a  = 1.0 / (sz + N.z);
    float ya = N.y * a;
    float b  = N.x * ya;
    float c  = N.x * sz;

    vec3 T = vec3(c * N.x * a - 1.0, sz * b, c);
    vec3 B = vec3(b, N.y * ya - sz, N.y);

    // Note: due to the quaternion formulation, the generated frame is rotated by 180 degrees,
    // s.t. if N = (0, 0, 1), then T = (-1, 0, 0) and B = (0, -1, 0).
    return mat3(T, B, N);
}

float GetSpecularDominantFactor(float NoV, float linearRoughness) {
    linearRoughness = saturate(linearRoughness);

    const float a = 0.298475 * log(39.4115 - 39.0029 * linearRoughness);
    const float dominantFactor = pow(saturate(1.0 - NoV), 10.8649) * (1.0 - a) + a;

    return saturate(dominantFactor);
}

vec3 GetSpecularDominantDirectionWithFactor( vec3 N, vec3 V, float dominantFactor ) {
    vec3 R = reflect(-V, N);
    vec3 D = mix(N, R, dominantFactor);
    return normalize(D);
}

vec4 GetSpecularDominantDirection( vec3 N, vec3 V, float linearRoughness ) {
    const float NoV = abs( dot( N, V ) );
    const float dominantFactor = GetSpecularDominantFactor( NoV, linearRoughness );
    return vec4( GetSpecularDominantDirectionWithFactor( N, V, dominantFactor ), dominantFactor );
}

// Ray Tracing Gems II, Listing 49-9
mat2x3 GetKernelBasis(const vec3 X, vec3 N, float radius_ws, float roughness) {
    const mat3 basis = GetBasis(N);
    vec3 T = basis[0];
    vec3 B = basis[1];

    vec3 V = -normalize(X);
    vec4 D = GetSpecularDominantDirection(N, V, roughness);
    float NoD = abs(dot(N, D.xyz));

    if (NoD < 0.999) {
        const vec3 R = reflect(-D.xyz, N);
        T = normalize(cross(N, R));
        B = cross(R, T);
    }

    T *= radius_ws;
    B *= radius_ws;

    return mat2x3(T, B);
}

vec2 GetKernelSampleCoordinates(mat4 xform, vec3 offset, vec3 Xv, vec3 Tv, vec3 Bv, vec4 rotator) {
    // We can't rotate T and B instead, because T is skewed
    offset.xy = RotateVector(rotator, offset.xy);

    vec3 p = Xv + Tv * offset.x + Bv * offset.y;

    vec4 projected = xform * vec4(p, 1.0);
    projected.xyz /= projected.w;
    projected.xy = 0.5 * projected.xy + 0.5;
#if defined(VULKAN)
    projected.y = (1.0 - projected.y);
#endif // VULKAN
    return projected.xy;
}

float IsInScreen(vec2 uv) {
    return float(all(equal(saturate(uv), uv)) ? 1.0 : 0.0);
}

float GetGaussianWeight(float r) {
    // radius is normalized to 1
    return exp( -0.66 * r * r );
}

const vec3 g_Special8[8] = vec3[8](
    // https://www.desmos.com/calculator/abaqyvswem
    vec3( -1.00               ,  0.00               , 1.0 ),
    vec3(  0.00               ,  1.00               , 1.0 ),
    vec3(  1.00               ,  0.00               , 1.0 ),
    vec3(  0.00               , -1.00               , 1.0 ),
    vec3( -0.25 * SQRT_2      ,  0.25 * SQRT_2      , 0.5 ),
    vec3(  0.25 * SQRT_2      ,  0.25 * SQRT_2      , 0.5 ),
    vec3(  0.25 * SQRT_2      , -0.25 * SQRT_2      , 0.5 ),
    vec3( -0.25 * SQRT_2      , -0.25 * SQRT_2      , 0.5 )
);

// Acos(x) (approximate)
// http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiJhY29zKHgpIiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjowLCJlcSI6InNxcnQoMS14KSpzcXJ0KDIpIiwiY29sb3IiOiIjRjIwQzBDIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiMCIsIjEiLCIwIiwiMiJdLCJzaXplIjpbMTE1MCw5MDBdfV0-
#define _AcosApprox(x) (sqrt(2.0) * sqrt(saturate(1.0 - (x))))

vec2 GetCombinedWeight(
    float baseWeight,
    vec2 geometry_weight_params, vec3 Nv, vec3 Xvs,
    float normalWeightParams, vec3 N, vec4 Ns,
    vec2 hitDistanceWeightParams, float hit_dist, vec2 minwh,
    vec2 roughnessWeightParams) {
    vec4 a = vec4(geometry_weight_params.x, normalWeightParams, hitDistanceWeightParams.x, roughnessWeightParams.x);
    vec4 b = vec4(geometry_weight_params.y, -0.001, hitDistanceWeightParams.y, roughnessWeightParams.y);

    vec4 t;
    t.x = dot(Nv, Xvs);
    t.y = _AcosApprox(saturate(dot(N, Ns.xyz)));
    t.z = hit_dist;
    t.w = Ns.w;

    t = SmoothStep01(1.0 - abs(t * a + b));

    baseWeight *= t.x;// * t.y * t.w;

    return vec2(baseWeight);// * mix(minwh, vec2(1.0), t.z);
}

/*float GetNormalWeightParams(float non_linear_accum_speed, float curvature, float view_z, float roughness, float strictness) {
    // Estimate how many samples from a potentially bumpy normal map fit in the pixel
    float pixel_radius = PixelRadiusToWorld( gUnproject, gIsOrtho, gScreenSize.y, view_z ); // TODO: for the entire screen to unlock compression in the next line (no fancy solid angle math)
    float pixelRadiusNorm = pixel_radius / ( 1.0 + pixel_radius );
    float pixelAreaNorm = pixelRadiusNorm * pixelRadiusNorm;

    float s = mix(0.01, 0.15, pixelAreaNorm);
    s = mix(s, 1.0, non_linear_accum_speed) * strictness;
    s = mix(s, 1.0, curvature * curvature);

    float params = STL::ImportanceSampling::GetSpecularLobeHalfAngle( roughness, lerp( 0.75, 0.95, curvature ) );
    params *= saturate( s );
    params = 1.0 / max( params, NRD_ENCODING_ERRORS.x );

    return params;
}*/

float GetSpecMagicCurve(float roughness, float power) {
    float f = 1.0 - exp2( -200.0 * roughness * roughness );
    f *= pow(roughness, power);
    return f;
}

float GetBlurRadius(float radius, float hit_dist, float view_z, float non_linear_accum_speed, float radius_bias, float radius_scale, float roughness) {
    // Modify by hit distance
    float hit_dist_factor = hit_dist / (hit_dist + view_z);
    float s = hit_dist_factor;

    // Scale down if accumulation goes well
    //float keepBlurringDistantReflections = saturate( 1.0 - STL::Math::Pow01( roughness, 0.125 ) ) * hit_dist_factor;
    //s *= lerp( keepBlurringDistantReflections * float( radius_bias != 0.0 ), 1.0, non_linear_accum_speed ); // TODO: not apply in BLUR pass too?

    s *= non_linear_accum_speed;

    // A non zero addition is needed to avoid under-blurring:
    float addon = 3.0; // TODO: was 3.0 * ( 1.0 + 2.0 * boost )
    addon = min(addon, radius * 0.333);
    addon *= roughness;
    addon *= hit_dist_factor;

    // Avoid over-blurring on contact
    radius_bias *= mix(roughness, 1.0, hit_dist_factor);

    // Final blur radius
    float r = s * radius + addon;
    r = r * (radius_scale + radius_bias) + radius_bias;
    r *= GetSpecMagicCurve(roughness, 0.25);

    return r;
}

void Blur(ivec2 dispatch_thread_id, ivec2 group_thread_id, uvec2 screen_size) {
    vec2 pix_uv = (vec2(dispatch_thread_id) + 0.5) / vec2(screen_size);
#if defined(VULKAN)
    pix_uv.y = 1.0 - pix_uv.y;
#endif // VULKAN
    const float center_depth = texelFetch(g_depth_tex, dispatch_thread_id, 0).x;
    const float center_depth_lin = LinearizeDepth(center_depth, g_shrd_data.clip_info);

    const vec4 normal_fetch = UnpackNormalAndRoughness(texelFetch(g_normal_tex, dispatch_thread_id, 0).x);
    const vec3 center_normal_ws = normal_fetch.xyz;
    const float center_roughness = normal_fetch.w;
    const vec3 center_normal_vs = normalize((g_shrd_data.view_from_world * vec4(center_normal_ws, 0.0)).xyz);

    const vec3 center_point_vs = ReconstructViewPosition(pix_uv, g_shrd_data.frustum_info, -center_depth_lin, 0.0 /* is_ortho */);

    float sample_count = texelFetch(g_sample_count_tex, dispatch_thread_id, 0).x;
    float variance = texelFetch(g_variance_tex, dispatch_thread_id, 0).x;
    float accumulation_speed = 1.0 / max(sample_count, 1.0);

    const float PlaneDistSensitivity = 0.005;
    const vec2 geometry_weight_params = GetGeometryWeightParams(PlaneDistSensitivity, center_point_vs, center_normal_vs, accumulation_speed);

    const float RadiusBias = 0.0;
#ifdef PER_PIXEL_KERNEL_ROTATION
    const float RadiusScale = 2.0;
#else
    const float RadiusScale = 1.0;
#endif

    const float InitialBlurRadius = 0.1;

    /* mediump */ vec4 sum = texelFetch(g_refl_tex, dispatch_thread_id, 0);
    /* mediump */ vec2 total_weight = vec2(1.0);

    float hit_dist = sum.w;
    float blur_radius = GetBlurRadius(InitialBlurRadius, hit_dist, center_depth_lin, accumulation_speed, RadiusBias, RadiusScale, center_roughness);
    float blur_radius_ws = PixelRadiusToWorld(g_shrd_data.taa_info.w, 0.0 /* is_ortho */, blur_radius, center_depth_lin);

    mat2x3 TvBv = GetKernelBasis(center_point_vs, center_normal_vs, blur_radius_ws, center_roughness);
    vec4 kernel_rotator = GetBlurKernelRotation(uvec2(dispatch_thread_id), g_params.rotator, g_params.frame_index.x);

    const bool needs_blur = IsGlossyReflection(center_roughness) && !IsMirrorReflection(center_roughness);
    for (int i = 0; i < 8 && needs_blur; ++i) {
        const vec3 offset = g_Special8[i];
        const vec2 uv = GetKernelSampleCoordinates(g_shrd_data.clip_from_view, offset, center_point_vs, TvBv[0], TvBv[1], kernel_rotator);

        const float neighbor_depth = LinearizeDepth(textureLod(g_depth_tex, uv, 0.0).x, g_shrd_data.clip_info);
        const vec3 neighbor_normal_ws = UnpackNormalAndRoughness(textureLod(g_normal_tex, uv, 0.0).x).xyz;

        vec2 reconstruct_uv = uv;
#if defined(VULKAN)
        reconstruct_uv.y = 1.0 - reconstruct_uv.y;
#endif // VULKAN
        vec3 neighbor_point_vs = ReconstructViewPosition(reconstruct_uv, g_shrd_data.frustum_info, -neighbor_depth, 0.0 /* is_ortho */);

        /* mediump */ float weight = IsInScreen(uv);
        weight *= GetGaussianWeight(offset.z);
        weight *= GetEdgeStoppingNormalWeight(center_normal_ws, neighbor_normal_ws);
        //weight *= GetEdgeStoppingDepthWeight(center_depth_lin, neighbor_depth);
        weight *= GetEdgeStoppingPlanarDistanceWeight(geometry_weight_params, center_normal_vs, neighbor_point_vs);

        sum += weight * textureLod(g_refl_tex, uv, 0.0);
        total_weight += vec2(weight);
    }

    sum /= total_weight.xxxy;

    imageStore(g_out_denoised_img, dispatch_thread_id, sum);
}

void main() {
    const uint packed_coords = g_tile_list[gl_WorkGroupID.x];
    const ivec2 dispatch_thread_id = ivec2(packed_coords & 0xffffu, (packed_coords >> 16) & 0xffffu) + ivec2(gl_LocalInvocationID.xy);
    const ivec2  dispatch_group_id = dispatch_thread_id / 8;
    const uvec2 remapped_group_thread_id = RemapLane8x8(gl_LocalInvocationIndex);
    const uvec2 remapped_dispatch_thread_id = dispatch_group_id * 8 + remapped_group_thread_id;

    Blur(ivec2(remapped_dispatch_thread_id), ivec2(remapped_group_thread_id), g_params.img_size.xy);
}