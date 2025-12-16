#include "Renderer.h"

#include "Utils.h"

#include <cassert>

#include "../Ren/Context.h"
#include "../Ren/GL.h"
#include "../Ren/GLCtx.h"
#include "../Sys/Json.h"

namespace Gui {
extern const int TexAtlasSlot;
} // namespace Gui

Gui::Renderer::Renderer(Ren::Context &ctx) : ctx_(ctx) { instance_index_ = g_instance_count++; }

Gui::Renderer::~Renderer() {
    if (ctx_.capabilities.persistent_buf_mapping) {
        const Ren::ApiContext &api = ctx_.api();

        const auto &[vtx_stage_main, vtx_stage_cold] = ctx_.buffers().Get(vertex_stage_buf_);
        Buffer_Unmap(api, vtx_stage_main, vtx_stage_cold);

        const auto &[ndx_stage_main, ndx_stage_cold] = ctx_.buffers().Get(index_stage_buf_);
        Buffer_Unmap(api, ndx_stage_main, ndx_stage_cold);
    }

    ctx_.ReleaseBuffer(vertex_stage_buf_, true /* immediately */);
    ctx_.ReleaseBuffer(index_stage_buf_, true /* immediately */);
    ctx_.ReleaseBuffer(vertex_buf_, true /* immediately */);
    ctx_.ReleaseBuffer(index_buf_, true /* immediately */);
}

void Gui::Renderer::Draw(const int w, const int h) {
    const std::string label_name = name_ + "::Draw";
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, GLsizei(label_name.length()), label_name.c_str());

    //
    // Update buffers
    //
    const GLbitfield BufRangeMapFlags = GLbitfield(GL_MAP_COHERENT_BIT) | GLbitfield(GL_MAP_WRITE_BIT) |
                                        GLbitfield(GL_MAP_INVALIDATE_RANGE_BIT) | GLbitfield(GL_MAP_UNSYNCHRONIZED_BIT);

    const int stage_frame = ctx_.in_flight_frontend_frame[ctx_.backend_frame()];
    if (vtx_count_[stage_frame]) {
        //
        // Update stage buffer
        //
        glBindBuffer(GL_COPY_READ_BUFFER, ctx_.buffers().Get(vertex_stage_buf_).first.buf);

        const size_t vertex_buf_mem_offset = GLintptr(stage_frame) * MaxVerticesPerRange * sizeof(vertex_t);
        const size_t vertex_buf_mem_size = vtx_count_[stage_frame] * sizeof(vertex_t);
        if (!ctx_.capabilities.persistent_buf_mapping) {
            void *pinned_mem = glMapBufferRange(GL_COPY_READ_BUFFER, vertex_buf_mem_offset,
                                                MaxVerticesPerRange * sizeof(vertex_t), BufRangeMapFlags);
            if (pinned_mem) {
                memcpy(pinned_mem, vtx_stage_data_, vertex_buf_mem_size);
                glUnmapBuffer(GL_COPY_READ_BUFFER);
            } else {
                ctx_.log()->Error("[Gui::Renderer::Draw]: Failed to map vertex buffer!");
            }
        }

        //
        // Copy stage buffer contents to actual vertex buffer
        //
        glBindBuffer(GL_COPY_WRITE_BUFFER, ctx_.buffers().Get(vertex_buf_).first.buf);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, vertex_buf_mem_offset /* read_offset */,
                            0 /* write_offset */, vertex_buf_mem_size);

        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    }

    const size_t index_buf_mem_offset = size_t(stage_frame) * MaxIndicesPerRange * sizeof(uint16_t);

    if (ndx_count_[stage_frame]) {
        //
        // Update stage buffer
        //
        glBindBuffer(GL_COPY_READ_BUFFER, ctx_.buffers().Get(index_stage_buf_).first.buf);

        const size_t index_buf_mem_size = ndx_count_[stage_frame] * sizeof(uint16_t);
        if (!ctx_.capabilities.persistent_buf_mapping) {
            void *pinned_mem = glMapBufferRange(GL_COPY_READ_BUFFER, index_buf_mem_offset,
                                                MaxIndicesPerRange * sizeof(uint16_t), BufRangeMapFlags);
            if (pinned_mem) {
                memcpy(pinned_mem, ndx_stage_data_, index_buf_mem_size);
                glUnmapBuffer(GL_COPY_READ_BUFFER);
            } else {
                ctx_.log()->Error("[Gui::Renderer::Draw]: Failed to map index buffer!");
            }
        }

        //
        // Copy stage buffer contents to actual index buffer
        //
        glBindBuffer(GL_COPY_WRITE_BUFFER, ctx_.buffers().Get(index_buf_).first.buf);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, index_buf_mem_offset /* read_offset */,
                            0 /* write_offset */, index_buf_mem_size);

        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    }

    //
    // Submit draw call
    //
    const Ren::PipelineMain &pi_main = ctx_.pipelines().Get(pipeline_).first;
    const Ren::ProgramMain &pr_main = ctx_.programs().Get(pi_main.prog).first;
    const Ren::VertexInputMain &vi = ctx_.vtx_inputs().Get(pi_main.vtx_input).first;

    pi_main.rast_state.viewport[2] = w;
    pi_main.rast_state.viewport[3] = h;
    pi_main.rast_state.Apply();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindVertexArray(VertexInput_GetVAO(vi, ctx_.buffers()));
    glUseProgram(pr_main.id);

    glActiveTexture(GL_TEXTURE0 + TexAtlasSlot);
    glBindTexture(GL_TEXTURE_2D_ARRAY, GLuint(ctx_.image_atlas().id()));

    glDrawElements(GL_TRIANGLES, ndx_count_[stage_frame], GL_UNSIGNED_SHORT,
                   reinterpret_cast<const GLvoid *>(uintptr_t(0)));

    glBindVertexArray(0);
    glUseProgram(0);

    vtx_count_[stage_frame] = ndx_count_[stage_frame] = 0;

    glPopDebugGroup();
}

#undef VTX_POS_LOC
#undef VTX_COL_LOC
#undef VTX_UVS_LOC

#undef TEX_ATLAS_SLOT
