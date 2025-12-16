#include "FgNode.h"

#include "FgBuilder.h"

Eng::FgResource *Eng::FgNode::FindUsageOf(const eFgResType type, const uint16_t index) {
    for (FgResource &r : input_) {
        if (r.type == type && r.index == index) {
            return &r;
        }
    }
    for (FgResource &r : output_) {
        if (r.type == type && r.index == index) {
            return &r;
        }
    }
    return nullptr;
}

Eng::FgBufHandle Eng::FgNode::AddTransferInput(const FgBufHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::CopySrc, Ren::eStage::Transfer, *this);
}

Eng::FgBufHandle Eng::FgNode::AddTransferInput(const Ren::BufferHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::CopySrc, Ren::eStage::Transfer, *this);
}

Eng::FgBufHandle Eng::FgNode::AddTransferOutput(std::string_view name, const FgBufDesc &desc) {
    return builder_.WriteBuffer(name, desc, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgBufHandle Eng::FgNode::AddTransferOutput(const FgBufHandle handle) {
    return builder_.WriteBuffer(handle, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgBufHandle Eng::FgNode::AddTransferOutput(const Ren::BufferHandle handle) {
    return builder_.WriteBuffer(handle, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgResRef Eng::FgNode::AddTransferImageInput(const Ren::WeakImgRef &tex) {
    return builder_.ReadImage(tex, Ren::eResState::CopySrc, Ren::eStage::Transfer, *this);
}

Eng::FgResRef Eng::FgNode::AddTransferImageInput(const FgResRef handle) {
    return builder_.ReadImage(handle, Ren::eResState::CopySrc, Ren::eStage::Transfer, *this);
}

Eng::FgResRef Eng::FgNode::AddTransferImageOutput(std::string_view name, const FgImgDesc &desc) {
    return builder_.WriteImage(name, desc, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgResRef Eng::FgNode::AddTransferImageOutput(const Ren::WeakImgRef &tex) {
    return builder_.WriteImage(tex, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgResRef Eng::FgNode::AddTransferImageOutput(const FgResRef handle) {
    return builder_.WriteImage(handle, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgBufHandle Eng::FgNode::AddStorageReadonlyInput(const FgBufHandle handle,
                                                      const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadBuffer(handle, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgBufHandle Eng::FgNode::AddStorageReadonlyInput(const Ren::BufferHandle buf,
                                                      const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadBuffer(buf, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgBufHandle Eng::FgNode::AddStorageOutput(std::string_view name, const FgBufDesc &desc,
                                               const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteBuffer(name, desc, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgBufHandle Eng::FgNode::AddStorageOutput(const FgBufHandle handle, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteBuffer(handle, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgBufHandle Eng::FgNode::AddStorageOutput(const Ren::BufferHandle buf, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteBuffer(buf, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddStorageImageOutput(std::string_view name, const FgImgDesc &desc,
                                                 const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteImage(name, desc, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddStorageImageOutput(const FgResRef handle, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteImage(handle, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddStorageImageOutput(const Ren::WeakImgRef &tex, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteImage(tex, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddColorOutput(std::string_view name, const FgImgDesc &desc) {
    return builder_.WriteImage(name, desc, Ren::eResState::RenderTarget, Ren::eStage::ColorAttachment, *this);
}

Eng::FgResRef Eng::FgNode::AddColorOutput(const FgResRef handle) {
    return builder_.WriteImage(handle, Ren::eResState::RenderTarget, Ren::eStage::ColorAttachment, *this);
}

Eng::FgResRef Eng::FgNode::AddColorOutput(const Ren::WeakImgRef &tex) {
    return builder_.WriteImage(tex, Ren::eResState::RenderTarget, Ren::eStage::ColorAttachment, *this);
}

Eng::FgResRef Eng::FgNode::AddColorOutput(std::string_view name) {
    return builder_.WriteImage(name, Ren::eResState::RenderTarget, Ren::eStage::ColorAttachment, *this);
}

Eng::FgResRef Eng::FgNode::AddDepthOutput(std::string_view name, const FgImgDesc &desc) {
    return builder_.WriteImage(name, desc, Ren::eResState::DepthWrite, Ren::eStage::DepthAttachment, *this);
}

Eng::FgResRef Eng::FgNode::AddDepthOutput(const FgResRef handle) {
    return builder_.WriteImage(handle, Ren::eResState::DepthWrite, Ren::eStage::DepthAttachment, *this);
}

Eng::FgResRef Eng::FgNode::AddDepthOutput(const Ren::WeakImgRef &tex) {
    return builder_.WriteImage(tex, Ren::eResState::DepthWrite, Ren::eStage::DepthAttachment, *this);
}

Eng::FgBufHandle Eng::FgNode::ReplaceTransferInput(const int slot_index, const Ren::BufferHandle buf) {
    return builder_.ReadBuffer(buf, Ren::eResState::CopySrc, Ren::eStage::Transfer, *this, slot_index);
}

Eng::FgResRef Eng::FgNode::ReplaceColorOutput(const int slot_index, const Ren::WeakImgRef &tex) {
    return builder_.WriteImage(tex, Ren::eResState::RenderTarget, Ren::eStage::ColorAttachment, *this, slot_index);
}

Eng::FgBufHandle Eng::FgNode::AddUniformBufferInput(const FgBufHandle handle, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadBuffer(handle, Ren::eResState::UniformBuffer, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddTextureInput(const FgResRef handle, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadImage(handle, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddTextureInput(const Ren::WeakImgRef &tex, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadImage(tex, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddTextureInput(std::string_view name, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadImage(name, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddHistoryTextureInput(const FgResRef handle, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadHistoryImage(handle, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddHistoryTextureInput(std::string_view name, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadHistoryImage(name, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgResRef Eng::FgNode::AddCustomTextureInput(const FgResRef handle, const Ren::eResState desired_state,
                                                 const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadImage(handle, desired_state, stages, *this);
}

Eng::FgBufHandle Eng::FgNode::AddVertexBufferInput(const FgBufHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::VertexBuffer, Ren::eStage::VertexInput, *this);
}

Eng::FgBufHandle Eng::FgNode::AddVertexBufferInput(const Ren::BufferHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::VertexBuffer, Ren::eStage::VertexInput, *this);
}

Eng::FgBufHandle Eng::FgNode::AddIndexBufferInput(const FgBufHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::IndexBuffer, Ren::eStage::VertexInput, *this);
}

Eng::FgBufHandle Eng::FgNode::AddIndexBufferInput(const Ren::BufferHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::IndexBuffer, Ren::eStage::VertexInput, *this);
}

Eng::FgBufHandle Eng::FgNode::AddIndirectBufferInput(const FgBufHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::IndirectArgument, Ren::eStage::DrawIndirect, *this);
}

Eng::FgBufHandle Eng::FgNode::AddASBuildReadonlyInput(const FgBufHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::BuildASRead, Ren::eStage::AccStructureBuild, *this);
}

Eng::FgBufHandle Eng::FgNode::AddASBuildOutput(const Ren::BufferHandle handle) {
    return builder_.WriteBuffer(handle, Ren::eResState::BuildASWrite, Ren::eStage::AccStructureBuild, *this);
}

Eng::FgBufHandle Eng::FgNode::AddASBuildOutput(std::string_view name, const FgBufDesc &desc) {
    return builder_.WriteBuffer(name, desc, Ren::eResState::BuildASWrite, Ren::eStage::AccStructureBuild, *this);
}
