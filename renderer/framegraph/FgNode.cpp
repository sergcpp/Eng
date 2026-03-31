#include "FgNode.h"

#include "FgBuilder.h"

Eng::FgResource *Eng::FgNode::FindUsageOf(const eFgResType type, const uint16_t index) {
    for (FgResource &r : input_) {
        if (r.type == type && r.opaque_handle.index == index) {
            return &r;
        }
    }
    for (FgResource &r : output_) {
        if (r.type == type && r.opaque_handle.index == index) {
            return &r;
        }
    }
    return nullptr;
}

Eng::FgBufROHandle Eng::FgNode::AddTransferInput(const FgBufROHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::CopySrc, Ren::eStage::Transfer, *this);
}

Eng::FgBufRWHandle Eng::FgNode::AddTransferOutput(std::string_view name, const FgBufDesc &desc) {
    return builder_.WriteBuffer(name, desc, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgBufRWHandle Eng::FgNode::AddTransferOutput(const FgBufRWHandle handle) {
    return builder_.WriteBuffer(handle, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgImgROHandle Eng::FgNode::AddTransferImageInput(const FgImgROHandle handle) {
    return builder_.ReadImage(handle, Ren::eResState::CopySrc, Ren::eStage::Transfer, *this);
}

Eng::FgImgRWHandle Eng::FgNode::AddTransferImageOutput(std::string_view name, const FgImgDesc &desc) {
    return builder_.WriteImage(name, desc, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgImgRWHandle Eng::FgNode::AddTransferImageOutput(const FgImgRWHandle handle) {
    return builder_.WriteImage(handle, Ren::eResState::CopyDst, Ren::eStage::Transfer, *this);
}

Eng::FgBufROHandle Eng::FgNode::AddStorageReadonlyInput(const FgBufROHandle handle,
                                                        const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadBuffer(handle, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgBufRWHandle Eng::FgNode::AddStorageOutput(std::string_view name, const FgBufDesc &desc,
                                                 const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteBuffer(name, desc, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgBufRWHandle Eng::FgNode::AddStorageOutput(const FgBufRWHandle handle, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteBuffer(handle, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgImgRWHandle Eng::FgNode::AddStorageImageOutput(std::string_view name, const FgImgDesc &desc,
                                                      const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteImage(name, desc, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgImgRWHandle Eng::FgNode::AddStorageImageOutput(const FgImgRWHandle handle,
                                                      const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.WriteImage(handle, Ren::eResState::UnorderedAccess, stages, *this);
}

Eng::FgImgRWHandle Eng::FgNode::AddColorOutput(std::string_view name, const FgImgDesc &desc) {
    return builder_.WriteImage(name, desc, Ren::eResState::RenderTarget, Ren::eStage::ColorAttachment, *this);
}

Eng::FgImgRWHandle Eng::FgNode::AddColorOutput(const FgImgRWHandle handle) {
    return builder_.WriteImage(handle, Ren::eResState::RenderTarget, Ren::eStage::ColorAttachment, *this);
}

Eng::FgImgRWHandle Eng::FgNode::AddDepthOutput(std::string_view name, const FgImgDesc &desc) {
    return builder_.WriteImage(name, desc, Ren::eResState::DepthWrite, Ren::eStage::DepthAttachment, *this);
}

Eng::FgImgRWHandle Eng::FgNode::AddDepthOutput(const FgImgRWHandle handle) {
    return builder_.WriteImage(handle, Ren::eResState::DepthWrite, Ren::eStage::DepthAttachment, *this);
}

Eng::FgBufROHandle Eng::FgNode::ReplaceTransferInput(const int slot_index, const FgBufROHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::CopySrc, Ren::eStage::Transfer, *this, slot_index);
}

Eng::FgImgRWHandle Eng::FgNode::ReplaceColorOutput(const int slot_index, const FgImgRWHandle handle) {
    return builder_.WriteImage(handle, Ren::eResState::RenderTarget, Ren::eStage::ColorAttachment, *this, slot_index);
}

Eng::FgBufROHandle Eng::FgNode::AddUniformBufferInput(const FgBufROHandle handle,
                                                      const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadBuffer(handle, Ren::eResState::UniformBuffer, stages, *this);
}

Eng::FgImgROHandle Eng::FgNode::AddTextureInput(const FgImgROHandle handle, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadImage(handle, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgImgROHandle Eng::FgNode::AddHistoryTextureInput(const FgImgROHandle handle,
                                                       const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadHistoryImage(handle, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgImgROHandle Eng::FgNode::AddHistoryTextureInput(std::string_view name, const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadHistoryImage(name, Ren::eResState::ShaderResource, stages, *this);
}

Eng::FgImgROHandle Eng::FgNode::AddCustomTextureInput(const FgImgROHandle handle, const Ren::eResState desired_state,
                                                      const Ren::Bitmask<Ren::eStage> stages) {
    return builder_.ReadImage(handle, desired_state, stages, *this);
}

Eng::FgBufROHandle Eng::FgNode::AddVertexBufferInput(const FgBufROHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::VertexBuffer, Ren::eStage::VertexInput, *this);
}

Eng::FgBufROHandle Eng::FgNode::AddIndexBufferInput(const FgBufROHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::IndexBuffer, Ren::eStage::VertexInput, *this);
}

Eng::FgBufROHandle Eng::FgNode::AddIndirectBufferInput(const FgBufROHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::IndirectArgument, Ren::eStage::DrawIndirect, *this);
}

Eng::FgBufROHandle Eng::FgNode::AddASBuildReadonlyInput(const FgBufROHandle handle) {
    return builder_.ReadBuffer(handle, Ren::eResState::BuildASRead, Ren::eStage::AccStructureBuild, *this);
}

Eng::FgBufRWHandle Eng::FgNode::AddASBuildOutput(const FgBufRWHandle handle) {
    return builder_.WriteBuffer(handle, Ren::eResState::BuildASWrite, Ren::eStage::AccStructureBuild, *this);
}

Eng::FgBufRWHandle Eng::FgNode::AddASBuildOutput(std::string_view name, const FgBufDesc &desc) {
    return builder_.WriteBuffer(name, desc, Ren::eResState::BuildASWrite, Ren::eStage::AccStructureBuild, *this);
}
