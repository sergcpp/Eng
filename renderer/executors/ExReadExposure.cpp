#include "ExReadExposure.h"

#include <Ren/Context.h>

#include "../../utils/ShaderLoader.h"

void Eng::ExReadExposure::Execute(const FgContext &fg) {
    const Ren::Image &input_tex = fg.AccessROImage(args_->input_tex);
    const Ren::BufferHandle output_buf = fg.AccessRWBuffer(args_->output_buf);

    const auto &[output_buf_main, output_buf_cold] = fg.storages().buffers.Get(output_buf);

    { // Retrieve result of readback from previous frame
        const auto *mapped_ptr = (const float *)Ren::Buffer_Map(fg.ren_ctx().api(), output_buf_main, output_buf_cold);
        if (mapped_ptr) {
            exposure_ = mapped_ptr[fg.backend_frame()];
            Ren::Buffer_Unmap(fg.ren_ctx().api(), output_buf_main, output_buf_cold);
        }
    }

    input_tex.CopyTextureData(output_buf_main, output_buf_cold, fg.cmd_buf(), sizeof(float) * fg.backend_frame(),
                              sizeof(float));
}
