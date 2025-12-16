#pragma once

#include <mutex>
#include <string>

#include <Ren/Pipeline.h>
#include <Ren/Program.h>
#include <Ren/RenderPass.h>
#include <Ren/Shader.h>
#include <Ren/VertexInput.h>

namespace Eng {
class ShaderLoader {
    std::mutex mtx_;
    Ren::Context &ctx_;

    static std::string ReadGLSLContent(std::string_view name, Ren::ILog *log);

  public:
    ShaderLoader(Ren::Context &ctx);
    ~ShaderLoader();

    Ren::Context &ren_ctx() { return ctx_; }

    void LoadPipelineCache(const char *base_path);
    void WritePipelineCache(const char *base_path);

    Ren::VertexInputHandle LoadVertexInput(Ren::Span<const Ren::VtxAttribDesc> attribs, Ren::BufferHandle elem_buf);

    Ren::RenderPassHandle LoadRenderPass(const Ren::RenderTargetInfo &depth_rt,
                                         Ren::Span<const Ren::RenderTargetInfo> color_rts);
    Ren::RenderPassHandle LoadRenderPass(const Ren::RenderTarget &depth_rt,
                                         Ren::Span<const Ren::RenderTarget> color_rts) {
        Ren::SmallVector<Ren::RenderTargetInfo, 4> infos;
        for (int i = 0; i < color_rts.size(); ++i) {
            infos.emplace_back(color_rts[i]);
        }
        return LoadRenderPass(Ren::RenderTargetInfo{depth_rt}, infos);
    }

#if defined(REN_GL_BACKEND) || defined(REN_VK_BACKEND)
    Ren::ProgramHandle LoadProgram(std::string_view vs_name, std::string_view fs_name, std::string_view tcs_name = {},
                                   std::string_view tes_name = {}, std::string_view gs_name = {});
    Ren::ProgramHandle LoadProgram(std::string_view cs_name);
#endif
    Ren::ShaderHandle LoadShader(std::string_view name);

#if defined(REN_VK_BACKEND)
    Ren::ProgramHandle LoadProgram2(std::string_view raygen_name, std::string_view closesthit_name,
                                    std::string_view anyhit_name, std::string_view miss_name,
                                    std::string_view intersection_name);
#endif

    Ren::PipelineHandle LoadPipeline(const Ren::RastState &rast_state, Ren::ProgramHandle prog,
                                     Ren::VertexInputHandle vtx_input, Ren::RenderPassHandle render_pass,
                                     uint32_t subpass_index);

    Ren::PipelineHandle LoadPipeline(std::string_view cs_name, int subgroup_size = -1);
    Ren::PipelineHandle LoadPipeline(const Ren::ProgramHandle prog, int subgroup_size = -1);
};
} // namespace Eng