#pragma once

#include <Sys/InplaceFunction.h>

#include "FgBuilder.h"
#include "FgResource.h"

namespace Eng {
class FgExecutor {
  public:
    virtual ~FgExecutor() {}

    virtual void Execute(const FgContext &fg) = 0;
};

class FgNode {
  private:
    friend class FgBuilder;

    std::string name_;
    int16_t index_ = -1;
    eFgQueueType queue_;
    FgBuilder &builder_;
    Ren::SmallVector<FgResource, 16> input_;
    Ren::SmallVector<FgResource, 16> output_;

    std::unique_ptr<FgExecutor> executor_;
    FgExecutor *p_executor_ = nullptr;
    Sys::InplaceFunction<void(const FgContext &fg), 48> execute_cb_;

    mutable Ren::SmallVector<int16_t, 16> depends_on_nodes_;
    mutable bool visited_ = false;

  public:
    FgNode(std::string_view name, const int16_t index, eFgQueueType queue, FgBuilder &builder)
        : name_(name), index_(index), queue_(queue), builder_(builder) {}

    template <typename T, class... Args> T *AllocNodeData(Args &&...args) {
        return builder_.AllocNodeData<T>(std::forward<Args>(args)...);
    }

    template <typename F> void set_execute_cb(F &&f) { execute_cb_ = f; }

    void set_executor(std::unique_ptr<FgExecutor> &&exec) {
        executor_ = std::move(exec);
        p_executor_ = executor_.get();
    }

    template <class T, class... Args> void make_executor(Args &&...args) {
        executor_ = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        p_executor_ = executor_.get();
    }

    Ren::Span<const FgResource> input() const { return input_; }
    Ren::Span<const FgResource> output() const { return output_; }

    FgResource *FindUsageOf(eFgResType type, uint16_t index);

    // Non-owning version
    void set_executor(FgExecutor *exec) { p_executor_ = exec; }

    FgBufROHandle AddTransferInput(FgBufROHandle handle);
    FgBufROHandle AddTransferInput(Ren::BufferHandle handle);
    FgBufRWHandle AddTransferOutput(std::string_view name, const FgBufDesc &desc);
    FgBufRWHandle AddTransferOutput(FgBufRWHandle handle);
    FgBufRWHandle AddTransferOutput(Ren::BufferHandle handle);

    FgResRef AddTransferImageInput(const Ren::WeakImgRef &tex);
    FgResRef AddTransferImageInput(FgResRef handle);
    FgResRef AddTransferImageOutput(std::string_view name, const FgImgDesc &desc);
    FgResRef AddTransferImageOutput(const Ren::WeakImgRef &tex);
    FgResRef AddTransferImageOutput(FgResRef handle);

    FgBufROHandle AddStorageReadonlyInput(FgBufROHandle handle, Ren::Bitmask<Ren::eStage> stages);
    FgBufROHandle AddStorageReadonlyInput(Ren::BufferHandle buf, Ren::Bitmask<Ren::eStage> stages);
    FgBufRWHandle AddStorageOutput(std::string_view name, const FgBufDesc &desc, Ren::Bitmask<Ren::eStage> stages);
    FgBufRWHandle AddStorageOutput(FgBufRWHandle handle, Ren::Bitmask<Ren::eStage> stages);
    FgBufRWHandle AddStorageOutput(Ren::BufferHandle buf, Ren::Bitmask<Ren::eStage> stages);

    FgResRef AddStorageImageOutput(std::string_view name, const FgImgDesc &desc, Ren::Bitmask<Ren::eStage> stages);
    FgResRef AddStorageImageOutput(FgResRef handle, Ren::Bitmask<Ren::eStage> stages);
    FgResRef AddStorageImageOutput(const Ren::WeakImgRef &tex, Ren::Bitmask<Ren::eStage> stages);

    FgResRef AddClearImageOutput(std::string_view name, const FgImgDesc &desc) {
        return AddTransferImageOutput(name, desc);
    }
    FgResRef AddClearImageOutput(const Ren::WeakImgRef &tex) { return AddTransferImageOutput(tex); }
    FgResRef AddClearImageOutput(FgResRef handle) { return AddTransferImageOutput(handle); }

    FgResRef AddColorOutput(std::string_view name, const FgImgDesc &desc);
    FgResRef AddColorOutput(FgResRef handle);
    FgResRef AddColorOutput(const Ren::WeakImgRef &tex);
    FgResRef AddColorOutput(std::string_view name);
    FgResRef AddDepthOutput(std::string_view name, const FgImgDesc &desc);
    FgResRef AddDepthOutput(FgResRef handle);
    FgResRef AddDepthOutput(const Ren::WeakImgRef &tex);

    // TODO: try to get rid of this
    FgBufROHandle ReplaceTransferInput(int slot_index, Ren::BufferHandle buf);
    FgResRef ReplaceColorOutput(int slot_index, const Ren::WeakImgRef &tex);

    FgBufROHandle AddUniformBufferInput(FgBufROHandle handle, Ren::Bitmask<Ren::eStage> stages);

    FgResRef AddTextureInput(FgResRef handle, Ren::Bitmask<Ren::eStage> stages);
    FgResRef AddTextureInput(const Ren::WeakImgRef &tex, Ren::Bitmask<Ren::eStage> stages);
    FgResRef AddTextureInput(std::string_view name, Ren::Bitmask<Ren::eStage> stages);

    FgResRef AddHistoryTextureInput(FgResRef handle, Ren::Bitmask<Ren::eStage> stages);
    FgResRef AddHistoryTextureInput(std::string_view name, Ren::Bitmask<Ren::eStage> stages);

    FgResRef AddCustomTextureInput(FgResRef handle, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages);

    FgBufROHandle AddVertexBufferInput(FgBufROHandle handle);
    FgBufROHandle AddVertexBufferInput(Ren::BufferHandle buf);
    FgBufROHandle AddIndexBufferInput(FgBufROHandle handle);
    FgBufROHandle AddIndexBufferInput(Ren::BufferHandle buf);
    FgBufROHandle AddIndirectBufferInput(FgBufROHandle handle);

    FgBufROHandle AddASBuildReadonlyInput(FgBufROHandle handle);
    FgBufRWHandle AddASBuildOutput(Ren::BufferHandle handle);
    FgBufRWHandle AddASBuildOutput(std::string_view name, const FgBufDesc &desc);

    void Execute(const FgContext &fg) {
        if (p_executor_) {
            p_executor_->Execute(fg);
        } else if (execute_cb_) {
            execute_cb_(fg);
        }
    }

    std::string_view name() const { return name_; }
};
} // namespace Eng