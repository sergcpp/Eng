#pragma once

#include <Ren/utils/Span.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

namespace Eng {
class ExUpdateAccBuffers final : public FgExecutor {
    const DrawList *&p_list_;
    int rt_index_;

    FgBufRWHandle rt_geo_instances_;
    FgBufRWHandle rt_obj_instances_;

    void Execute_HWRT(const FgContext &fg);
    void Execute_SWRT(const FgContext &fg);

  public:
    ExUpdateAccBuffers(const DrawList *&p_list, int rt_index, const FgBufRWHandle rt_geo_instances,
                       const FgBufRWHandle rt_obj_instances)
        : p_list_(p_list), rt_index_(rt_index), rt_geo_instances_(rt_geo_instances),
          rt_obj_instances_(rt_obj_instances) {}

    void Execute(const FgContext &fg) override;
};
} // namespace Eng