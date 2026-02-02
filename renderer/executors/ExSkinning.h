#pragma once

#include <Ren/Fwd.h>

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

namespace Eng {
class ShaderLoader;
class ExSkinning final : public FgExecutor {
    Ren::PipelineHandle pi_skinning_;
    const DrawList *&p_list_;

    FgBufROHandle skin_vtx_buf_;
    FgBufROHandle skin_transforms_buf_;
    FgBufROHandle shape_keys_buf_;
    FgBufROHandle delta_buf_;

    FgBufRWHandle vtx_buf1_;
    FgBufRWHandle vtx_buf2_;

    void LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh);

  public:
    ExSkinning(const DrawList *&p_list, const FgBufROHandle skin_vtx_buf, const FgBufROHandle skin_transforms_buf,
               const FgBufROHandle shape_keys_buf, const FgBufROHandle delta_buf, const FgBufRWHandle vtx_buf1,
               const FgBufRWHandle vtx_buf2)
        : p_list_(p_list), skin_vtx_buf_(skin_vtx_buf), skin_transforms_buf_(skin_transforms_buf),
          shape_keys_buf_(shape_keys_buf), delta_buf_(delta_buf), vtx_buf1_(vtx_buf1), vtx_buf2_(vtx_buf2) {}

    void Execute(const FgContext &fg) override;
};
} // namespace Eng