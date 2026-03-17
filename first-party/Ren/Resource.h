#pragma once

#include <cstdint>

#include <variant>

#include "Fwd.h"
#include "utils/Bitmask.h"
#include "utils/Span.h"

namespace Ren {
struct ApiContext;

enum class eStage : uint16_t {
    VertexInput,
    VertexShader,
    TessCtrlShader,
    TessEvalShader,
    GeometryShader,
    FragmentShader,
    ComputeShader,
    RayTracingShader,
    ColorAttachment,
    DepthAttachment,
    DrawIndirect,
    Transfer,
    AccStructureBuild
};

const Bitmask<eStage> AllStages = Bitmask<eStage>{eStage::VertexInput} | eStage::VertexShader | eStage::TessCtrlShader |
                                  eStage::TessEvalShader | eStage::GeometryShader | eStage::FragmentShader |
                                  eStage::ComputeShader | eStage::RayTracingShader | eStage::ColorAttachment |
                                  eStage::DepthAttachment | eStage::DrawIndirect | eStage::Transfer |
                                  eStage::AccStructureBuild;

enum class eResState : uint8_t {
    Undefined,
    Discarded,
    VertexBuffer,
    UniformBuffer,
    IndexBuffer,
    RenderTarget,
    UnorderedAccess,
    DepthRead,
    DepthWrite,
    StencilTestDepthFetch,
    ShaderResource,
    IndirectArgument,
    CopyDst,
    CopySrc,
    BuildASRead,
    BuildASWrite,
    RayTracing,
    _Count
};

enum class eImageLayout : uint8_t {
    Undefined,
    General,
    ColorAttachmentOptimal,
    DepthStencilAttachmentOptimal,
    DepthStencilReadOnlyOptimal,
    ShaderReadOnlyOptimal,
    TransferSrcOptimal,
    TransferDstOptimal,
    _Count
};

inline bool operator<(const eImageLayout lhs, const eImageLayout rhs) { return uint8_t(lhs) < uint8_t(rhs); }

#if defined(REN_VK_BACKEND)
int VKImageLayoutForState(eResState state);
uint32_t VKAccessFlagsForState(eResState state);
uint32_t VKPipelineStagesForState(eResState state);

inline eImageLayout ImageLayoutForState(const eResState state) { return eImageLayout(VKImageLayoutForState(state)); }
#else
inline eImageLayout ImageLayoutForState(const eResState state) { return eImageLayout::Undefined; }
#endif
bool IsRWState(eResState state);
Bitmask<eStage> StagesForState(eResState state);

struct TransitionInfo {
    std::variant<BufferHandle, ImageHandle> p_res;

    eResState old_state = eResState::Undefined;
    eResState new_state = eResState::Undefined;

    bool update_internal_state = false;

    TransitionInfo() = default;
    TransitionInfo(const BufferHandle _buf, const eResState _new_state)
        : p_res(_buf), new_state(_new_state), update_internal_state(true) {}
    TransitionInfo(const ImageHandle _img, const eResState _new_state)
        : p_res(_img), new_state(_new_state), update_internal_state(true) {}
};

void TransitionResourceStates(const ApiContext &api, const StoragesRef &storages, CommandBuffer cmd_buf,
                              Bitmask<eStage> src_stages_mask, Bitmask<eStage> dst_stages_mask,
                              Span<const TransitionInfo> transitions);
} // namespace Ren