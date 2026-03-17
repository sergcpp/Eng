#pragma once

#include "../Common.h"
#include "../Fence.h"
#include "../utils/SmallVector.h"
#include "../utils/Storage.h"

namespace Ren {
struct ImageMain;
struct ImageCold;

using ImageRWHandle = Handle<ImageMain, RWTag>;
using ImageROHandle = Handle<ImageMain, ROTag>;
using ImageHandle = ImageRWHandle;

struct ApiContext {
    SmallVector<SyncFence, MaxFramesInFlight> in_flight_fences;

    int active_present_image = 0;

    int backend_frame = 0;
    SmallVector<ImageHandle, MaxFramesInFlight> present_image_handles;

    uint32_t queries[MaxFramesInFlight][MaxTimestampQueries] = {};
    mutable uint32_t query_counts[MaxFramesInFlight] = {};
    mutable uint64_t query_results[MaxFramesInFlight][MaxTimestampQueries] = {};

    // generation counters
    mutable uint32_t buffer_counter = 0;
    mutable uint32_t image_counter = 0;

    CommandBuffer BegSingleTimeCommands() const { return nullptr; }
    void EndSingleTimeCommands(CommandBuffer command_buf) const {}
};

bool ReadbackTimestampQueries(const ApiContext &api, int i);

} // namespace Ren