#include "ShaderLoader.h"

#include <fstream>

#include <Ren/Context.h>
#include <Sys/AssetFile.h>
#include <Sys/MemBuf.h>

namespace ShaderLoaderInternal {
#if defined(__ANDROID__)
const char *SHADERS_PATH = "./assets/shaders/";
#else
const char *SHADERS_PATH = "./assets_pc/shaders/";
#endif

Ren::eShaderType ShaderTypeFromName(std::string_view name) {
    Ren::eShaderType type;
    if (std::strncmp(name.data() + name.length() - 10, ".vert.glsl", 10) == 0) {
        type = Ren::eShaderType::Vertex;
    } else if (std::strncmp(name.data() + name.length() - 10, ".frag.glsl", 10) == 0) {
        type = Ren::eShaderType::Fragment;
    } else if (std::strncmp(name.data() + name.length() - 10, ".tesc.glsl", 10) == 0) {
        type = Ren::eShaderType::TesselationControl;
    } else if (std::strncmp(name.data() + name.length() - 10, ".tese.glsl", 10) == 0) {
        type = Ren::eShaderType::TesselationEvaluation;
    } else if (std::strncmp(name.data() + name.length() - 10, ".geom.glsl", 10) == 0) {
        type = Ren::eShaderType::Geometry;
    } else if (std::strncmp(name.data() + name.length() - 10, ".comp.glsl", 10) == 0) {
        type = Ren::eShaderType::Compute;
    } else if (std::strncmp(name.data() + name.length() - 10, ".rgen.glsl", 10) == 0) {
        type = Ren::eShaderType::RayGen;
    } else if (std::strncmp(name.data() + name.length() - 11, ".rchit.glsl", 11) == 0) {
        type = Ren::eShaderType::ClosestHit;
    } else if (std::strncmp(name.data() + name.length() - 11, ".rahit.glsl", 11) == 0) {
        type = Ren::eShaderType::AnyHit;
    } else if (std::strncmp(name.data() + name.length() - 11, ".rmiss.glsl", 11) == 0) {
        type = Ren::eShaderType::Miss;
    } else if (std::strncmp(name.data() + name.length() - 10, ".rint.glsl", 10) == 0) {
        type = Ren::eShaderType::Intersection;
    } else {
        type = Ren::eShaderType::_Count;
    }
    return type;
}
} // namespace ShaderLoaderInternal

Eng::ShaderLoader::ShaderLoader(Ren::Context &ctx) : ctx_(ctx) {
    // prevent reallocation
    ctx.vtx_inputs().reserve(32);
    ctx.render_passes().reserve(128);
    ctx.shaders().reserve(2048);
    ctx.programs().reserve(1024);
    ctx.pipelines().reserve(2048);
}

Eng::ShaderLoader::~ShaderLoader() {
    // destroy pipelines
    while (!ctx_.pipelines().sorted_items().empty()) {
        const Ren::PipelineHandle pi = ctx_.pipelines().sorted_items().back();
        const auto &[pipeline_main, pipeline_cold] = ctx_.pipelines().Get(pi);
        Pipeline_Destroy(ctx_.api(), pipeline_main, pipeline_cold);
        ctx_.pipelines().PopBack();
    }
    // destroy programs
    while (!ctx_.programs().sorted_items().empty()) {
        const Ren::ProgramHandle pr = ctx_.programs().sorted_items().back();
        const auto &[program_main, program_cold] = ctx_.programs().Get(pr);
        Program_Destroy(ctx_.api(), program_main, program_cold);
        ctx_.programs().PopBack();
    }
    // destroy shaders
    auto &all_shaders = ctx_.shaders().items_by_name();
    for (auto it = all_shaders.begin(); it != all_shaders.end();) {
        const auto &[shader_main, shader_cold] = ctx_.shaders().Get(it->val);
        Shader_Destroy(ctx_.api(), shader_main, shader_cold);
        it = ctx_.shaders().Free(it);
    }
}

void Eng::ShaderLoader::LoadPipelineCache(const char *base_path) {
#if defined(REN_VK_BACKEND)
    { // Load pipeline cache
        std::vector<uint8_t> cache_data;

        std::string file_name = base_path;
        file_name += std::to_string(ctx_.device_id());
        file_name += ".vk_cache";

        std::ifstream in_file(file_name, std::ios::binary | std::ios::ate);
        if (in_file) {
            const size_t in_file_size = size_t(in_file.tellg());
            in_file.seekg(0, std::ios::beg);
            cache_data.resize(in_file_size);
            in_file.read((char *)cache_data.data(), in_file_size);
        }

        ctx_.InitPipelineCache(cache_data);
    }
#endif
}

void Eng::ShaderLoader::WritePipelineCache(const char *base_path) {
#if defined(REN_VK_BACKEND)
    const size_t data_size = ctx_.WritePipelineCache({});
    if (data_size) {
        std::vector<uint8_t> data(data_size);
        const size_t written_size = ctx_.WritePipelineCache(data);
        if (written_size != data_size) {
            ctx_.log()->Error("Failed to write pipeline cache");
        }

        std::string file_name = base_path;
        file_name += std::to_string(ctx_.device_id());
        file_name += ".vk_cache";

        { // Write out file
            std::ofstream out_file(file_name + "_temp", std::ios::binary);
            out_file.write((char *)data.data(), data_size);
            if (!out_file.good() || out_file.tellp() != data_size) {
                ctx_.log()->Error("Failed to write pipeline cache");
            }
        }

        remove(file_name.c_str());
        if (rename((file_name + "_temp").c_str(), file_name.c_str()) != 0) {
            ctx_.log()->Error("Failed to rename pipeline cache");
        }
    } else {
        ctx_.log()->Error("Failed to write pipeline cache");
    }
#endif
}

Ren::VertexInputHandle Eng::ShaderLoader::LoadVertexInput(Ren::Span<const Ren::VtxAttribDesc> attribs,
                                                          const Ren::BufferHandle elem_buf) {
    std::lock_guard<std::mutex> _(mtx_);
    return ctx_.FindOrCreateVertexInput(attribs, elem_buf);
}

Ren::RenderPassHandle Eng::ShaderLoader::LoadRenderPass(const Ren::RenderTargetInfo &depth_rt,
                                                        Ren::Span<const Ren::RenderTargetInfo> color_rts) {
    std::lock_guard<std::mutex> _(mtx_);

    Ren::RenderPassHandle ret = ctx_.render_passes().LowerBound(
        [&](const Ren::RenderPassMain &rp) { return rp.LessThan(depth_rt, color_rts); });
    if (!ret || !ctx_.render_passes().Get(ret).first.Equals(depth_rt, color_rts)) {
        Ren::RenderPassMain rp_main = {};
        if (!RenderPass_Init(ctx_.api(), rp_main, depth_rt, color_rts, ctx_.log())) {
            return {};
        }
        ret = ctx_.render_passes().Insert(std::move(rp_main), {});
        assert(ctx_.render_passes().CheckUnique());
    }
    return ret;
}

Ren::ProgramHandle Eng::ShaderLoader::LoadProgram(std::string_view vs_name, std::string_view fs_name,
                                                  std::string_view tcs_name, std::string_view tes_name,
                                                  std::string_view gs_name) {
    Ren::ShaderHandle vs_handle = LoadShader(vs_name);
    Ren::ShaderHandle fs_handle = LoadShader(fs_name);
    if (!vs_handle || !fs_handle) {
        ctx_.log()->Error("Error loading shaders %s/%s", vs_name.data(), fs_name.data());
        return {};
    }

    Ren::ShaderHandle tcs_handle, tes_handle;
    if (!tcs_name.empty() && !tes_name.empty()) {
        tcs_handle = LoadShader(tcs_name);
        tes_handle = LoadShader(tes_name);
        if (!tcs_handle || !tes_handle) {
            ctx_.log()->Error("Error loading shaders %s/%s", tcs_name.data(), tes_name.data());
            return {};
        }
    }
    Ren::ShaderHandle gs_handle;
    if (!gs_name.empty()) {
        gs_handle = LoadShader(gs_name);
        if (!gs_handle) {
            ctx_.log()->Error("Error loading shader %s", gs_name.data());
            return {};
        }
    }

    std::lock_guard<std::mutex> _(mtx_);

    std::array<Ren::ShaderHandle, int(Ren::eShaderType::_Count)> temp_shaders;
    temp_shaders[int(Ren::eShaderType::Vertex)] = vs_handle;
    temp_shaders[int(Ren::eShaderType::Fragment)] = fs_handle;
    temp_shaders[int(Ren::eShaderType::TesselationControl)] = tcs_handle;
    temp_shaders[int(Ren::eShaderType::TesselationEvaluation)] = tes_handle;
    temp_shaders[int(Ren::eShaderType::Geometry)] = gs_handle;
    Ren::ProgramHandle ret =
        ctx_.programs().LowerBound([&](const Ren::ProgramMain &p) { return p.shaders < temp_shaders; });
    if (!ret || ctx_.programs().Get(ret).first.shaders != temp_shaders) {
        Ren::ProgramMain prog_main;
        Ren::ProgramCold prog_cold;

        if (!Ren::Program_Init(ctx_.api(), ctx_.shaders(), prog_main, prog_cold, vs_handle, fs_handle, tcs_handle,
                               tes_handle, gs_handle, ctx_.log())) {
            return {};
        }

        ret = ctx_.programs().Insert(std::move(prog_main), std::move(prog_cold));
        assert(ctx_.programs().CheckUnique());
    }
    return ret;
}

Ren::ProgramHandle Eng::ShaderLoader::LoadProgram(std::string_view cs_name) {
    const Ren::ShaderHandle cs_handle = LoadShader(cs_name);

    std::array<Ren::ShaderHandle, int(Ren::eShaderType::_Count)> temp_shaders;
    temp_shaders[int(Ren::eShaderType::Compute)] = cs_handle;

    Ren::ProgramHandle ret;
    { // find program
        std::lock_guard<std::mutex> _(mtx_);
        ret = ctx_.programs().LowerBound([&](const Ren::ProgramMain &p) { return p.shaders < temp_shaders; });
    }
    if (!ret || ctx_.programs().Get(ret).first.shaders != temp_shaders) {
        assert(cs_handle);

        Ren::ProgramMain prog_main;
        Ren::ProgramCold prog_cold;

        if (!Ren::Program_Init(ctx_.api(), ctx_.shaders(), prog_main, prog_cold, cs_handle, ctx_.log())) {
            return {};
        }

        std::lock_guard<std::mutex> _(mtx_);
        ret = ctx_.programs().Insert(std::move(prog_main), std::move(prog_cold));
        assert(ctx_.programs().CheckUnique());
    }
    return ret;
}

#if defined(REN_VK_BACKEND)
Ren::ProgramHandle Eng::ShaderLoader::LoadProgram2(std::string_view raygen_name, std::string_view closesthit_name,
                                                   std::string_view anyhit_name, std::string_view miss_name,
                                                   std::string_view intersection_name) {
    const Ren::ShaderHandle raygen_handle = LoadShader(raygen_name);

    Ren::ShaderHandle closesthit_handle, anyhit_handle, miss_handle;
    if (!closesthit_name.empty()) {
        closesthit_handle = LoadShader(closesthit_name);
    }
    if (!anyhit_name.empty()) {
        anyhit_handle = LoadShader(anyhit_name);
    }
    if (!miss_name.empty()) {
        miss_handle = LoadShader(miss_name);
    }

    Ren::ShaderHandle intersection_handle;
    if (!intersection_name.empty()) {
        intersection_handle = LoadShader(intersection_name);
    }

    std::array<Ren::ShaderHandle, int(Ren::eShaderType::_Count)> temp_shaders;
    temp_shaders[int(Ren::eShaderType::RayGen)] = raygen_handle;
    temp_shaders[int(Ren::eShaderType::ClosestHit)] = closesthit_handle;
    temp_shaders[int(Ren::eShaderType::AnyHit)] = anyhit_handle;
    temp_shaders[int(Ren::eShaderType::Miss)] = miss_handle;
    temp_shaders[int(Ren::eShaderType::Intersection)] = intersection_handle;

    Ren::ProgramHandle ret;
    { // find program
        std::lock_guard<std::mutex> _(mtx_);
        ret = ctx_.programs().LowerBound([&](const Ren::ProgramMain &p) { return p.shaders < temp_shaders; });
    }
    if (!ret || ctx_.programs().Get(ret).first.shaders != temp_shaders) {
        assert(raygen_handle);

        Ren::ProgramMain prog_main;
        Ren::ProgramCold prog_cold;

        if (!Ren::Program_Init2(ctx_.api(), ctx_.shaders(), prog_main, prog_cold, raygen_handle, closesthit_handle,
                                anyhit_handle, miss_handle, intersection_handle, ctx_.log())) {
            return {};
        }

        std::lock_guard<std::mutex> _(mtx_);
        ret = ctx_.programs().Insert(std::move(prog_main), std::move(prog_cold));
        assert(ctx_.programs().CheckUnique());
    }
    return ret;
}
#endif

Ren::ShaderHandle Eng::ShaderLoader::LoadShader(std::string_view name) {
    using namespace ShaderLoaderInternal;

    std::lock_guard<std::mutex> _(mtx_);

    const Ren::eShaderType type = ShaderTypeFromName(name);
    if (type == Ren::eShaderType::_Count) {
        ctx_.log()->Error("Shader name is not correct (%s)", name.data());
        return {};
    }

    Ren::ShaderHandle ret = ctx_.shaders().Find(name);
    if (!ret) {
        const Ren::String name_str{name};
        ret = ctx_.shaders().Emplace(name_str);

        const auto &[shader_main, shader_cold] = ctx_.shaders().Get(ret);
#if defined(REN_VK_BACKEND)
        if (ctx_.capabilities.spirv) {
            std::string spv_name = SHADERS_PATH;
            spv_name += name;
            const size_t n = spv_name.rfind(".glsl");
            assert(n != std::string::npos);

#if defined(NDEBUG)
            spv_name.replace(n + 1, 4, "spv");
#else
            spv_name.replace(n + 1, 4, "spv_dbg");
#endif

            Sys::AssetFile spv_file(spv_name);
            if (spv_file) {
                const size_t spv_data_size = spv_file.size();

                std::vector<uint8_t> spv_data(spv_data_size);
                spv_file.Read((char *)&spv_data[0], spv_data_size);

                if (!Shader_Init(ctx_.api(), shader_main, shader_cold, spv_data, name_str, type, ctx_.log())) {
                    ctx_.shaders().Release(name);
                    return {};
                }
            } else {
                ctx_.log()->Error("Error loading shader %s", name.data());

                ctx_.shaders().Release(name);
                return {};
            }
        }
#endif
#if defined(REN_GL_BACKEND)
        const std::string shader_src = ReadGLSLContent(name, ctx_.log());
        if (!shader_src.empty()) {
            if (!Shader_Init(ctx_.api(), shader_main, shader_cold, shader_src, name_str, type, ctx_.log())) {
                ctx_.shaders().Release(name);
                return {};
            }
        } else {
            ctx_.log()->Error("Error loading shader %s", name.data());

            ctx_.shaders().Release(name);
            return {};
        }
#endif
    }

    return ret;
}

Ren::PipelineHandle Eng::ShaderLoader::LoadPipeline(const Ren::RastState &rast_state, const Ren::ProgramHandle prog,
                                                    const Ren::VertexInputHandle vtx_input,
                                                    const Ren::RenderPassHandle render_pass,
                                                    const uint32_t subpass_index) {
    Ren::PipelineHandle ret;
    { // find pipeline
        std::lock_guard<std::mutex> _(mtx_);
        ret = ctx_.pipelines().LowerBound(
            [&](const Ren::PipelineMain &pi) { return pi.LessThan(rast_state, prog, vtx_input, render_pass); });
    }
    if (!ret || !ctx_.pipelines().Get(ret).first.Equals(rast_state, prog, vtx_input, render_pass)) {
        assert(prog);

        Ren::PipelineMain pipeline_main;
        Ren::PipelineCold pipeline_cold;
        if (!Pipeline_Init(ctx_.api(), ctx_.storages(), pipeline_main, pipeline_cold, rast_state, prog, vtx_input,
                           render_pass, subpass_index, ctx_.log())) {
            return {};
        }

        std::lock_guard<std::mutex> _(mtx_);
        ret = ctx_.pipelines().Insert(std::move(pipeline_main), std::move(pipeline_cold));
        assert(ctx_.pipelines().CheckUnique());
    }
    return ret;
}

Ren::PipelineHandle Eng::ShaderLoader::LoadPipeline(std::string_view cs_name, const int subgroup_size) {
    const Ren::ProgramHandle prog_ref = LoadProgram(cs_name);
    return LoadPipeline(prog_ref, subgroup_size);
}

Ren::PipelineHandle Eng::ShaderLoader::LoadPipeline(const Ren::ProgramHandle prog, const int subgroup_size) {
    Ren::PipelineHandle ret;
    { // find pipeline
        std::lock_guard<std::mutex> _(mtx_);
        ret = ctx_.pipelines().LowerBound([&](const Ren::PipelineMain &pi) { return pi.LessThan({}, prog, {}, {}); });
    }
    if (!ret || !ctx_.pipelines().Get(ret).first.Equals({}, prog, {}, {})) {
        assert(prog);

        Ren::PipelineMain pipeline_main;
        Ren::PipelineCold pipeline_cold;
        if (!Pipeline_Init(ctx_.api(), ctx_.shaders(), ctx_.programs(), ctx_.buffers(), pipeline_main, pipeline_cold,
                           prog, ctx_.log(), subgroup_size)) {
            return {};
        }

        std::lock_guard<std::mutex> _(mtx_);
        ret = ctx_.pipelines().Insert(std::move(pipeline_main), std::move(pipeline_cold));
        assert(ctx_.pipelines().CheckUnique());
    }
    return ret;
}
