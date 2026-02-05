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

    Ren::HashMap32<Ren::String, Ren::ShaderHandle> shaders_;
    std::vector<Ren::ProgramHandle> programs_;
    std::vector<Ren::VertexInputHandle> vtx_inputs_;
    std::vector<Ren::PipelineHandle> pipelines_;
    std::vector<Ren::RenderPassHandle> render_passes_;

    static std::string ReadGLSLContent(std::string_view name, Ren::ILog *log);

  public:
    ShaderLoader(Ren::Context &ctx);
    ~ShaderLoader();

    Ren::Context &ren_ctx() { return ctx_; }

    void LoadPipelineCache(const char *base_path);
    void WritePipelineCache(const char *base_path);

    Ren::VertexInputHandle FindOrCreateVertexInput(Ren::Span<const Ren::VtxAttribDesc> attribs);

    Ren::RenderPassHandle FindOrCreateRenderPass(const Ren::RenderTargetInfo &depth_rt,
                                                 Ren::Span<const Ren::RenderTargetInfo> color_rts);
    Ren::RenderPassHandle FindOrCreateRenderPass(const Ren::RenderTarget &depth_rt,
                                                 Ren::Span<const Ren::RenderTarget> color_rts);

    Ren::ShaderHandle FindOrCreateShader(std::string_view name);
#if defined(REN_GL_BACKEND) || defined(REN_VK_BACKEND)
    Ren::ProgramHandle FindOrCreateProgram(std::string_view vs_name, std::string_view fs_name,
                                           std::string_view tcs_name = {}, std::string_view tes_name = {},
                                           std::string_view gs_name = {});
    Ren::ProgramHandle FindOrCreateProgram(std::string_view cs_name);
#endif
#if defined(REN_VK_BACKEND)
    Ren::ProgramHandle FindOrCreateProgram2(std::string_view rgs_name, std::string_view chs_name,
                                            std::string_view ans_name, std::string_view ms_name,
                                            std::string_view is_name);
#endif

    Ren::PipelineHandle FindOrCreatePipeline(const Ren::RastState &rast_state, Ren::ProgramROHandle prog,
                                             Ren::VertexInputROHandle vtx_input, Ren::RenderPassROHandle render_pass,
                                             uint32_t subpass_index);
    Ren::PipelineHandle FindOrCreatePipeline(std::string_view cs_name, int subgroup_size = -1);
    Ren::PipelineHandle FindOrCreatePipeline(Ren::ProgramROHandle prog, int subgroup_size = -1);
};
} // namespace Eng