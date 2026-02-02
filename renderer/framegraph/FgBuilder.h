#pragma once

#include <climits>
#include <cstdint>

#include <unordered_set>
#include <variant>
#include <vector>

#include <Ren/Buffer.h>
#include <Ren/Common.h>
#include <Ren/Framebuffer.h>
#include <Ren/Fwd.h>
#include <Ren/HashMap32.h>
#include <Ren/Image.h>
#include <Ren/Log.h>
#include <Ren/Pipeline.h>
#include <Ren/RastState.h>
#include <Ren/Sampler.h>
#include <Ren/SmallVector.h>
#include <Ren/SparseArray.h>
#include <Sys/MonoAlloc.h>

#include "FgResource.h"

namespace Eng {
class ShaderLoader;
class PrimDraw;

struct fg_node_slot_t {
    int16_t node_index;
    int16_t slot_index;
};
static_assert(sizeof(fg_node_slot_t) == 4);

struct fg_node_range_t {
    int16_t first_write_node = SHRT_MAX;
    int16_t last_write_node = -1;
    int16_t first_read_node = SHRT_MAX;
    int16_t last_read_node = -1;

    bool has_writer() const { return first_write_node <= last_write_node; }
    bool has_reader() const { return first_read_node <= last_read_node; }
    bool is_used() const { return has_writer() || has_reader(); }

    bool can_alias() const {
        if (has_reader() && has_writer() && first_read_node <= first_write_node) {
            return false;
        }
        return true;
    }

    int last_used_node() const {
        int16_t last_node = 0;
        if (has_writer()) {
            last_node = std::max(last_node, last_write_node);
        }
        if (has_reader()) {
            last_node = std::max(last_node, last_read_node);
        }
        return last_node;
    }

    int first_used_node() const {
        int16_t first_node = SHRT_MAX;
        if (has_writer()) {
            first_node = std::min(first_node, first_write_node);
        }
        if (has_reader()) {
            first_node = std::min(first_node, first_read_node);
        }
        return first_node;
    }

    int length() const { return last_used_node() - first_used_node(); }
};

struct FgAllocRes {
    union {
        struct {
            mutable uint8_t img_read_count;
            mutable uint8_t img_write_count;
        };
        uint16_t _img_generation = 0;
    };

    // TODO: Use Ren::String here
    std::string name;
    bool external = false;
    int alias_of = -1; // used in case of simple resource-to-resource aliasing
    int history_of = -1;
    int history_index = -1;

    Ren::Bitmask<Ren::eStage> used_in_stages, aliased_in_stages;
    Ren::SmallVector<fg_node_slot_t, 32> written_in_nodes;
    Ren::SmallVector<fg_node_slot_t, 32> read_in_nodes;
    Ren::SmallVector<FgResRef, 32> overlaps_with; // used in case of memory-level aliasing
    fg_node_range_t lifetime;
};

struct FgAllocImg : public FgAllocRes {
    FgImgDesc desc;
    Ren::WeakImgRef ref;
    Ren::ImgRef strong_ref;
};

struct FgAllocBufMain {
    Ren::BufferHandle handle;
};

struct FgAllocBufCold : public FgAllocRes {
    FgBufDesc desc;
};

using FgBufROHandle = Ren::Handle<FgAllocBufMain, Ren::ROTag>;
using FgBufRWHandle = Ren::Handle<FgAllocBufMain, Ren::RWTag>;

enum class eFgQueueType : uint8_t { Graphics, Compute, Transfer };

class FgNode;

class FgContext {
  protected:
    Ren::Context &ctx_;
    ShaderLoader &sh_;

    mutable Ren::RastState rast_state_;

    Ren::NamedDualStorage<FgAllocBufMain, FgAllocBufCold> buffers_;

    Ren::SparseArray<FgAllocImg> images_;
    Ren::HashMap32<std::string, uint16_t> name_to_image_;

    FgContext(Ren::Context &ctx, ShaderLoader &sh) : ctx_(ctx), sh_(sh) {}

  public:
    Ren::Context &ren_ctx() const { return ctx_; }
    ShaderLoader &sh() const { return sh_; }
    Ren::RastState &rast_state() const { return rast_state_; }

    const Ren::StoragesRef &storages() const;
    const Ren::DualStorage<Ren::ProgramMain, Ren::ProgramCold> &programs() const;
    const Ren::DualStorage<Ren::PipelineMain, Ren::PipelineCold> &pipelines() const;

    Ren::CommandBuffer cmd_buf() const;
    Ren::ILog *log() const;
    Ren::DescrMultiPoolAlloc &descr_alloc() const;

    int backend_frame() const;

    Ren::BufferROHandle AccessROBuffer(FgBufROHandle handle) const;
    const Ren::Image &AccessROImage(FgResRef handle) const;

    Ren::BufferHandle AccessRWBuffer(FgBufRWHandle handle) const;
    const Ren::Image &AccessRWImage(FgResRef handle) const;

    // TODO: Get rid of these!
    Ren::WeakImgRef AccessROImageRef(FgResRef handle) const;
    Ren::WeakImgRef AccessRWImageRef(FgResRef handle) const;
};

class FgBuilder : public FgContext {
    PrimDraw &prim_draw_; // needed to clear rendertargets

    void InsertResourceTransitions(FgNode &node);
    void HandleResourceTransition(const FgResource &res, Ren::SmallVectorImpl<Ren::TransitionInfo> &res_transitions,
                                  Ren::Bitmask<Ren::eStage> &src_stages, Ren::Bitmask<Ren::eStage> &dst_stages);
    void CheckResourceStates(FgNode &node);

    bool DependsOn_r(int16_t dst_node, int16_t src_node);
    int16_t FindPreviousWrittenInNode(const FgResource &res);
    int16_t FindPreviousWrittenInNode(FgResRef res);
    int16_t FindPreviousWrittenInNode(FgBufRWHandle res);
    void FindPreviousReadInNodes(const FgResource &res, Ren::SmallVectorImpl<int16_t> &out_nodes);
    void TraverseNodeDependencies_r(FgNode *node, int recursion_depth, std::vector<FgNode *> &out_node_stack);

    void PrepareAllocResources();
    void PrepareResourceLifetimes();
    void AllocateNeededResources_Simple();
    bool AllocateNeededResources_MemHeaps();
    void ClearResources_Simple();
    void ClearResources_MemHeaps();
    void ReleaseMemHeaps();
    void BuildResourceLinkedLists();

    void ClearBuffer_AsTransfer(Ren::BufferHandle buf, Ren::CommandBuffer cmd_buf);
    void ClearBuffer_AsStorage(Ren::BufferHandle buf, Ren::CommandBuffer cmd_buf);

    void ClearImage_AsTransfer(Ren::ImgRef &img, Ren::CommandBuffer cmd_buf);
    void ClearImage_AsStorage(Ren::ImgRef &img, Ren::CommandBuffer cmd_buf);
    void ClearImage_AsTarget(Ren::ImgRef &img, Ren::CommandBuffer cmd_buf);

    static const int AllocBufSize = 4 * 1024 * 1024;
    std::unique_ptr<char[]> alloc_buf_;
    Sys::MonoAlloc<char> alloc_;
    std::vector<FgNode *> nodes_;
    std::vector<FgNode *> reordered_nodes_;
    std::vector<std::unique_ptr<void, void (*)(void *)>> nodes_data_;

    template <typename T> static void node_data_deleter(void *_ptr) {
        T *ptr = reinterpret_cast<T *>(_ptr);
        ptr->~T();
        // no deallocation is needed
    }

    std::vector<Ren::SmallVector<uint32_t, 4>> img_alias_chains_, buf_alias_chains_;
    std::vector<Ren::MemHeap> memory_heaps_;

    Ren::PipelineHandle pi_clear_image_[3][int(Ren::eFormat::_Count)];
    Ren::PipelineHandle pi_clear_buffer_;

  public:
    FgBuilder(Ren::Context &ctx, ShaderLoader &sh, PrimDraw &prim_draw);
    ~FgBuilder() { Reset(); }

    bool ready() const { return !reordered_nodes_.empty(); }

    FgNode &AddNode(std::string_view name, eFgQueueType queue = eFgQueueType::Graphics);
    FgNode *FindNode(std::string_view name);
    FgNode *GetReorderedNode(const int i) { return reordered_nodes_[i]; }

    std::string GetResourceDebugInfo(const FgResource &res) const;
    void GetResourceFrameLifetime(const FgAllocBufCold &b, uint16_t out_lifetime[2][2]) const;
    void GetResourceFrameLifetime(const FgAllocImg &t, uint16_t out_lifetime[2][2]) const;

    const Ren::NamedDualStorage<FgAllocBufMain, FgAllocBufCold> &buffers() const { return buffers_; }
    const Ren::SparseArray<FgAllocImg> &images() const { return images_; }

    template <typename T, class... Args> T *AllocNodeData(Args &&...args) {
        char *mem = alloc_.allocate(sizeof(T) + alignof(T));
        auto *new_data = reinterpret_cast<T *>(mem + alignof(T) - (uintptr_t(mem) % alignof(T)));
        alloc_.construct(new_data, std::forward<Args>(args)...);
        nodes_data_.push_back(std::unique_ptr<T, void (*)(void *)>(new_data, node_data_deleter<T>));
        return new_data;
    }

    FgBufROHandle ReadBuffer(FgBufROHandle handle, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                           FgNode &node);
    FgBufROHandle ReadBuffer(Ren::BufferHandle handle, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                           FgNode &node, int slot_index = -1);

    FgResRef ReadImage(FgResRef handle, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages, FgNode &node);
    FgResRef ReadImage(std::string_view name, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                       FgNode &node);
    FgResRef ReadImage(const Ren::WeakImgRef &ref, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                       FgNode &node);

    FgResRef ReadHistoryImage(FgResRef handle, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                              FgNode &node);
    FgResRef ReadHistoryImage(std::string_view name, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                              FgNode &node);

    FgBufRWHandle WriteBuffer(FgBufRWHandle handle, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                            FgNode &node);
    FgBufRWHandle WriteBuffer(std::string_view name, const FgBufDesc &desc, Ren::eResState desired_state,
                            Ren::Bitmask<Ren::eStage> stages, FgNode &node);
    FgBufRWHandle WriteBuffer(Ren::BufferHandle handle, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                            FgNode &node);

    FgResRef WriteImage(FgResRef handle, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages, FgNode &node);
    FgResRef WriteImage(std::string_view name, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                        FgNode &node);
    FgResRef WriteImage(std::string_view name, const FgImgDesc &desc, Ren::eResState desired_state,
                        Ren::Bitmask<Ren::eStage> stages, FgNode &node);
    FgResRef WriteImage(const Ren::WeakImgRef &ref, Ren::eResState desired_state, Ren::Bitmask<Ren::eStage> stages,
                        FgNode &node, int slot_index = -1);

    FgResRef ImportResource(const Ren::WeakImgRef &ref);

    void Reset();
    void Compile(Ren::Span<const std::variant<FgResRef, FgBufRWHandle>> backbuffer_sources = {});
    void Execute();

    struct node_timing_t {
        std::string_view name;
        int query_beg = -1, query_end = -1;
    };

    Ren::SmallVector<node_timing_t, 256> node_timings_[Ren::MaxFramesInFlight];
};
} // namespace Eng