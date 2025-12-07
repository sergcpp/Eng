#pragma once

#include "Buffer.h"

namespace Ren {
class ResizableBuffer {
    const ApiContext &api_;
    String name_;
    eBufType type_ = eBufType::_Count;
    uint32_t size_alignment_ = 0;
    SparseDualStorage<BufferMain, BufferCold> &storage_;
    BufferHandle handle_;
    SmallVector<eFormat, 4> views_;

  public:
    ResizableBuffer(const ApiContext &api, std::string_view name, const eBufType type, uint32_t size_alignment,
                    SparseDualStorage<BufferMain, BufferCold> &storage)
        : api_(api), name_(name), type_(type), size_alignment_(size_alignment), storage_(storage) {}
    ~ResizableBuffer();

    BufferRWHandle handle() { return handle_; }
    BufferROHandle handle() const { return handle_; }

    std::pair<BufferMain &, BufferCold &> buffer() { return storage_[handle_]; }
    std::pair<const BufferMain &, const BufferCold &> buffer() const { return storage_[handle_]; }

    bool Resize(uint32_t new_size, ILog *log, bool keep_content = true, bool release_immediately = false);
    void Release(bool immediately);

    int AddView(eFormat format);

    SubAllocation AllocSubRegion(uint32_t req_size, uint32_t req_alignment, std::string_view tag, ILog *log,
                                 const BufferMain *init_buf = nullptr, CommandBuffer cmd_buf = {},
                                 uint32_t init_off = 0);
    void FreeSubRegion(SubAllocation alloc);
};
} // namespace Ren