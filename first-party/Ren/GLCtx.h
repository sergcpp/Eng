#pragma once

#include "Common.h"
#include "Fence.h"
#include "SmallVector.h"
#include "Storage.h"

namespace Ren {
class Image;
using ImgRef = StrongRef<Image, NamedStorage<Image>>;

struct ApiContext {
    SmallVector<SyncFence, MaxFramesInFlight> in_flight_fences;

    int active_present_image = 0;

    int backend_frame = 0;
    SmallVector<ImgRef, MaxFramesInFlight> present_image_refs;

    uint32_t queries[MaxFramesInFlight][MaxTimestampQueries] = {};
    mutable uint32_t query_counts[MaxFramesInFlight] = {};
    mutable uint64_t query_results[MaxFramesInFlight][MaxTimestampQueries] = {};

    // generation counters
    mutable uint32_t buffer_counter = 0;

    CommandBuffer BegSingleTimeCommands() const { return nullptr; }
    void EndSingleTimeCommands(CommandBuffer command_buf) const {}
};

bool ReadbackTimestampQueries(const ApiContext &api, int i);

} // namespace Ren