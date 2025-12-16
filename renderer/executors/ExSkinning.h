#pragma once

#include <Ren/Fwd.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

namespace Eng {
class ShaderLoader;
class ExSkinning final : public FgExecutor {
    Ren::PipelineHandle pi_skinning_;
    const DrawList *&p_list_;

    FgBufHandle skin_vtx_buf_;
    FgBufHandle skin_transforms_buf_;
    FgBufHandle shape_keys_buf_;
    FgBufHandle delta_buf_;

    FgBufHandle vtx_buf1_;
    FgBufHandle vtx_buf2_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh);

  public:
    ExSkinning(const DrawList *&p_list, const FgBufHandle skin_vtx_buf, const FgBufHandle skin_transforms_buf,
               const FgBufHandle shape_keys_buf, const FgBufHandle delta_buf, const FgBufHandle vtx_buf1,
               const FgBufHandle vtx_buf2)
        : p_list_(p_list), skin_vtx_buf_(skin_vtx_buf), skin_transforms_buf_(skin_transforms_buf),
          shape_keys_buf_(shape_keys_buf), delta_buf_(delta_buf), vtx_buf1_(vtx_buf1), vtx_buf2_(vtx_buf2) {}

    void Execute(const FgContext &fg) override;
};
} // namespace Eng