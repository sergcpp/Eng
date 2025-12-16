#pragma once

#include <string_view>

#include "Fwd.h"

namespace Ren {
struct ApiContext;
struct DebugMarker {
    DebugMarker(const ApiContext &api, CommandBuffer cmd_buf, std::string_view name);
    ~DebugMarker();

  private:
    const ApiContext &api_;
    CommandBuffer cmd_buf_ = {};
};
} // namespace Ren
