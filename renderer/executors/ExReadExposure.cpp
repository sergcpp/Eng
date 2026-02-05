#include "ExReadExposure.h"

#include <Ren/Context.h>

#include "../../utils/ShaderLoader.h"
#include "../framegraph/FgBuilder.h"

void Eng::ExReadExposure::Execute(const FgContext &fg) {
    const Ren::ImageROHandle input_tex = fg.AccessROImage(args_->input_tex);
    const Ren::BufferRWHandle output_buf = fg.AccessRWBuffer(args_->output_buf);

    { // Retrieve result of readback from previous frame
        const auto *mapped_ptr = (const float *)fg.ren_ctx().MapBuffer(output_buf);
        if (mapped_ptr) {
            exposure_ = mapped_ptr[fg.backend_frame()];
            fg.ren_ctx().UnmapBuffer(output_buf);
        }
    }

    // Copy data from current frame
    const uint32_t data_off = sizeof(float) * fg.backend_frame();
    fg.ren_ctx().CmdCopyImageToBuffer(input_tex, output_buf, fg.cmd_buf(), data_off);
}
