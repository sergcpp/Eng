#include "Pipeline.h"

bool Ren::Pipeline_Init(const ApiContext &api, const DualStorage<ShaderMain, ShaderCold> &shaders,
                        const DualStorage<ProgramMain, ProgramCold> &programs,
                        NamedDualStorage<BufferMain, BufferCold> &buffers, PipelineMain &pipeline_main,
                        PipelineCold &pipeline_cold, const ProgramHandle prog, ILog *log, const int) {
    pipeline_main.prog = prog;
    pipeline_cold.type = ePipelineType::Compute;

    return true;
}

bool Ren::Pipeline_Init(const ApiContext &api, const StoragesRef &storages, PipelineMain &pipeline_main,
                        PipelineCold &pipeline_cold, const RastState &rast_state, const ProgramHandle prog,
                        const VertexInputHandle vtx_input, const RenderPassHandle render_pass,
                        const uint32_t subpass_index, ILog *log) {
    pipeline_main.rast_state = rast_state;
    pipeline_main.render_pass = render_pass;
    pipeline_main.prog = prog;
    pipeline_main.vtx_input = vtx_input;
    pipeline_cold.type = ePipelineType::Graphics;

    return true;
}

void Ren::Pipeline_Destroy(const ApiContext &api, PipelineMain &pipeline_main, PipelineCold &pipeline_cold) {
    pipeline_main = {};
    pipeline_cold = {};
}
