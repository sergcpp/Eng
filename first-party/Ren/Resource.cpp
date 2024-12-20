#include "Resource.h"

namespace Ren {
const eStageBits g_stage_bits_per_state[] = {
    {},                        // Undefined
    AllStages,                 // Discarded
    eStageBits::VertexInput,   // VertexBuffer
    eStageBits::VertexShader | /* eStageBits::TessCtrlShader | eStageBits::TessEvalShader | eStageBits::GeometryShader
                                  |*/
        eStageBits::FragmentShader | eStageBits::ComputeShader | eStageBits::RayTracingShader, // UniformBuffer
    eStageBits::VertexInput,                                                                   // IndexBuffer
    eStageBits::ColorAttachment,                                                               // RenderTarget
    eStageBits::VertexShader | /* eStageBits::TessCtrlShader | eStageBits::TessEvalShader | eStageBits::GeometryShader
                                  |*/
        eStageBits::FragmentShader | eStageBits::ComputeShader | eStageBits::RayTracingShader, // UnorderedAccess
    eStageBits::DepthAttachment,                                                               // DepthRead
    eStageBits::DepthAttachment,                                                               // DepthWrite
    eStageBits::DepthAttachment | eStageBits::FragmentShader,                                  // StencilTestDepthFetch
    eStageBits::VertexShader | /* eStageBits::TessCtrlShader | eStageBits::TessEvalShader | eStageBits::GeometryShader
                                  |*/
        eStageBits::FragmentShader | eStageBits::ComputeShader | eStageBits::RayTracingShader, // ShaderResource
    eStageBits::DrawIndirect,                                                                  // IndirectArgument
    eStageBits::Transfer,                                                                      // CopyDst
    eStageBits::Transfer,                                                                      // CopySrc
    eStageBits::AccStructureBuild,                                                             // BuildASRead
    eStageBits::AccStructureBuild,                                                             // BuildASWrite
    eStageBits::RayTracingShader                                                               // RayTracing
};
static_assert(std::size(g_stage_bits_per_state) == int(eResState::_Count), "!");
} // namespace Ren

Ren::eStageBits Ren::StageBitsForState(const eResState state) { return g_stage_bits_per_state[int(state)]; }

bool Ren::IsRWState(const eResState state) {
    return state == eResState::RenderTarget || state == eResState::UnorderedAccess || state == eResState::DepthWrite ||
           state == eResState::CopyDst || state == eResState::BuildASWrite;
}