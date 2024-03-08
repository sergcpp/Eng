#include "ShaderLoader.h"

#include <Ren/Context.h>
#include <Sys/AssetFile.h>
#include <Sys/MemBuf.h>

namespace ShaderLoaderInternal {
#if defined(__ANDROID__)
const char *SHADERS_PATH = "./assets/shaders/";
#else
const char *SHADERS_PATH = "./assets_pc/shaders/";
#endif

Ren::eShaderType ShaderTypeFromName(const char *name, const int len) {
    Ren::eShaderType type;
    if (std::strncmp(name + len - 10, ".vert.glsl", 10) == 0) {
        type = Ren::eShaderType::Vertex;
    } else if (std::strncmp(name + len - 10, ".frag.glsl", 10) == 0) {
        type = Ren::eShaderType::Fragment;
    } else if (std::strncmp(name + len - 10, ".tesc.glsl", 10) == 0) {
        type = Ren::eShaderType::TesselationControl;
    } else if (std::strncmp(name + len - 10, ".tese.glsl", 10) == 0) {
        type = Ren::eShaderType::TesselationEvaluation;
    } else if (std::strncmp(name + len - 10, ".comp.glsl", 10) == 0) {
        type = Ren::eShaderType::Compute;
    } else if (std::strncmp(name + len - 10, ".rgen.glsl", 10) == 0) {
        type = Ren::eShaderType::RayGen;
    } else if (std::strncmp(name + len - 11, ".rchit.glsl", 11) == 0) {
        type = Ren::eShaderType::ClosestHit;
    } else if (std::strncmp(name + len - 11, ".rahit.glsl", 11) == 0) {
        type = Ren::eShaderType::AnyHit;
    } else if (std::strncmp(name + len - 11, ".rmiss.glsl", 11) == 0) {
        type = Ren::eShaderType::Miss;
    } else if (std::strncmp(name + len - 10, ".rint.glsl", 10) == 0) {
        type = Ren::eShaderType::Intersection;
    } else {
        type = Ren::eShaderType::_Count;
    }
    return type;
}
} // namespace ShaderLoaderInternal

Eng::ShaderLoader::ShaderLoader() {
    temp_param_str_.reserve(1024);
    temp_param_def_.reserve(4096);
}

int Eng::ShaderLoader::ParamsToString(const Param *params, std::string &out_str, std::string &out_def) {
    const Param *param = params;
    while (param->key) {
        out_str += out_str.empty() ? '@' : ';';
        out_def += "#define ";
        out_str += param->key;
        out_def += param->key;
        if (param->val) {
            out_str += "=";
            out_str += param->val;
            out_def += " ";
            out_def += param->val;
        }
        out_def += '\n';
        ++param;
    }
    return int(param - params);
}

int Eng::ShaderLoader::ParamsStringToDef(const char *params, std::string &out_def) {
    if (!params || params[0] != '@') {
        return 0;
    }

    int count = 0;

    const char *p1 = params + 1;
    const char *p2 = p1 + 1;
    while (*p2) {
        if (*p2 == '=') {
            out_def += "#define ";
            out_def += std::string(p1, p2);
            out_def += " ";

            p1 = p2 + 1;
            while (p2 && *p2 && *p2 != ';') {
                ++p2;
            }

            out_def += std::string(p1, p2);
            out_def += '\n';

            if (*p2) {
                p1 = ++p2;
            }
            ++count;
        } else if (*p2 == ';') {
            out_def += "#define ";
            out_def += std::string(p1, p2);
            out_def += '\n';
            p1 = ++p2;
            ++count;
        }

        if (*p2) {
            ++p2;
        }
    }

    if (p1 != p2) {
        out_def += "#define ";
        out_def += std::string(p1, p2);
        out_def += '\n';
        ++count;
    }

    return count;
}

Ren::ProgramRef Eng::ShaderLoader::LoadProgram(Ren::Context &ctx, const char *name, const char *vs_name,
                                               const char *fs_name, const char *tcs_name, const char *tes_name) {
    Ren::eProgLoadStatus status;
    Ren::ProgramRef ret = ctx.LoadProgram(name, {}, {}, {}, {}, &status);
    if (!ret->ready()) {
        ctx.log()->Info("Loading %s", name);
        Ren::ShaderRef vs_ref = LoadShader(ctx, vs_name);
        Ren::ShaderRef fs_ref = LoadShader(ctx, fs_name);
        if (!vs_ref->ready() || !fs_ref->ready()) {
            ctx.log()->Error("Error loading program %s", name);
            return {};
        }

        Ren::ShaderRef tcs_ref, tes_ref;
        if (tcs_name && tes_name) {
            tcs_ref = LoadShader(ctx, tcs_name);
            tes_ref = LoadShader(ctx, tes_name);
            if (!tcs_ref->ready() || !tes_ref->ready()) {
                ctx.log()->Error("Error loading program %s", name);
                return {};
            }
        }
        ret->Init(vs_ref, fs_ref, tcs_ref, tes_ref, &status, ctx.log());
        if (status == Ren::eProgLoadStatus::SetToDefault) {
            ctx.log()->Error("Error loading program %s", name);
        }
    }
    return ret;
}

Ren::ProgramRef Eng::ShaderLoader::LoadProgram(Ren::Context &ctx, const char *name, const char *cs_name) {
    Ren::eProgLoadStatus status;
    Ren::ProgramRef ret = ctx.LoadProgram(name, {}, {}, {}, {}, &status);
    if (!ret->ready()) {
        ctx.log()->Info("Loading %s", name);
        Ren::ShaderRef cs_ref = LoadShader(ctx, cs_name);
        ret->Init(cs_ref, &status, ctx.log());
        if (status == Ren::eProgLoadStatus::SetToDefault) {
            ctx.log()->Error("Error loading program %s", name);
        }
    }
    return ret;
}

#if defined(USE_VK_RENDER)
Ren::ProgramRef Eng::ShaderLoader::LoadProgram(Ren::Context &ctx, const char *name, const char *raygen_name,
                                               const char *closesthit_name, const char *anyhit_name,
                                               const char *miss_name, const char *intersection_name) {
    Ren::eProgLoadStatus status;
    Ren::ProgramRef ret = ctx.LoadProgram(name, {}, {}, {}, {}, &status);
    if (!ret->ready()) {
        ctx.log()->Info("Loading %s", name);
        Ren::ShaderRef raygen_ref = LoadShader(ctx, raygen_name);

        Ren::ShaderRef closesthit_ref, anyhit_ref;
        if (closesthit_name) {
            closesthit_ref = LoadShader(ctx, closesthit_name);
        }
        if (anyhit_name) {
            anyhit_ref = LoadShader(ctx, anyhit_name);
        }

        Ren::ShaderRef miss_ref = LoadShader(ctx, miss_name);

        Ren::ShaderRef intersection_ref;
        if (intersection_name) {
            intersection_ref = LoadShader(ctx, intersection_name);
        }

        ret->Init(std::move(raygen_ref), std::move(closesthit_ref), std::move(anyhit_ref), std::move(miss_ref),
                  std::move(intersection_ref), &status, ctx.log());
        if (status == Ren::eProgLoadStatus::SetToDefault) {
            ctx.log()->Error("Error loading program %s", name);
        }
    }
    return ret;
}
#endif

Ren::ShaderRef Eng::ShaderLoader::LoadShader(Ren::Context &ctx, const char *name) {
    using namespace ShaderLoaderInternal;

    const char *params = strchr(name, '@');
    const int name_len = params ? int(params - name) : (int)strlen(name);
    if (name_len < 10) { // len of ".vert/.frag.glsl"
        ctx.log()->Error("Shader name is not correct (%s)", name);
        return {};
    }

    const Ren::eShaderType type = ShaderTypeFromName(name, name_len);
    if (type == Ren::eShaderType::_Count) {
        ctx.log()->Error("Shader name is not correct (%s)", name);
        return {};
    }

    Ren::eShaderLoadStatus status;
    Ren::ShaderRef ret = ctx.LoadShaderGLSL(name, nullptr, type, &status);
    if (!ret->ready()) {
        temp_param_def_.clear();

        if (ctx.capabilities.spirv && !ctx.capabilities.bindless_texture) {
            std::string spv_name = SHADERS_PATH;
            spv_name += name;
            const size_t n = spv_name.rfind(".glsl");
            assert(n != std::string::npos);

#if defined(USE_VK_RENDER)
            spv_name.replace(n + 1, 4, "spv");
#elif defined(USE_GL_RENDER)
            spv_name.replace(n + 1, 4, "spv_ogl");
#endif

            Sys::AssetFile spv_file(spv_name);
            if (spv_file) {
                const size_t spv_data_size = spv_file.size();

                std::unique_ptr<uint8_t[]> spv_data(new uint8_t[spv_data_size]);
                spv_file.Read((char *)&spv_data[0], spv_data_size);

                ret->Init(&spv_data[0], int(spv_data_size), type, &status, ctx.log());
                if (status == Ren::eShaderLoadStatus::CreatedFromData) {
                    return ret;
                }
            }
        }

        const int params_cnt = ParamsStringToDef(params, temp_param_def_);
        assert(params_cnt != -1);

        const std::string shader_src = ReadGLSLContent(name, ctx.log());
        if (!shader_src.empty()) {
            ret->Init(shader_src.c_str(), type, &status, ctx.log());
            if (status == Ren::eShaderLoadStatus::SetToDefault) {
                ctx.log()->Error("Error loading shader %s", name);
            }
        } else {
            ctx.log()->Error("Error loading shader %s", name);
        }
    }

    return ret;
}