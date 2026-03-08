#include "Material.h"

#include <cstdlib>

#include "Pipeline.h"
#include "SamplingParams.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace Ren {
uint8_t from_hex_char(const char c) { return (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0'); }

const SamplingParams g_default_mat_sampler = {eFilter::Trilinear, eWrap::Repeat, eCompareOp::None, Fixed8{}};
} // namespace Ren

bool Ren::Material_Init(MaterialMain &mat_main, MaterialCold &mat_cold, Ren::String name,
                        const Bitmask<eMatFlags> flags, Span<const PipelineHandle> pipelines,
                        Span<const ImageHandle> textures, Span<const SamplerHandle> samplers, Span<const Vec4f> params,
                        ILog *log) {
    mat_main.flags = flags;
    mat_cold.name = name;
    for (int i = 0; i < int(pipelines.size()); i++) {
        mat_main.pipelines.emplace_back(pipelines[i]);
    }
    assert(textures.size() == samplers.size());
    for (int i = 0; i < int(textures.size()); i++) {
        mat_main.textures.emplace_back(textures[i]);
        mat_main.samplers.emplace_back(samplers[i]);
    }
    for (int i = 0; i < int(params.size()); i++) {
        mat_cold.params.emplace_back(params[i]);
    }
    return true;
}

bool Ren::Material_Init(MaterialMain &mat_main, MaterialCold &mat_cold, Ren::String name, std::string_view mat_src,
                        const pipelines_load_callback &on_pipes_load, const texture_load_callback &on_tex_load,
                        const sampler_load_callback &on_sampler_load, ILog *log) {
    mat_cold.name = name;

    if (mat_src.empty()) {
        return true;
    }

    // Parse material
    const char *delims = " \r\n";
    const char *p = mat_src.data();
    const char *q = strpbrk(p + 1, delims);

    bool multi_doc = false;
    SmallVector<std::string, 4> v_shader_names, f_shader_names, tc_shader_names, te_shader_names;

    for (; p && q; q = strpbrk(p, delims)) {
        if (p == q) {
            p = q + 1;
            continue;
        }
        std::string item(p, q);
        if (item == "pipelines:") {
            p = q + 1;
            q = strpbrk(p, delims);
            for (; p && q; q = strpbrk(p, delims)) {
                if (p == q) {
                    p = q + 1;
                    continue;
                }
                if (*p != '-') {
                    break;
                }
#if defined(REN_GL_BACKEND) || defined(REN_VK_BACKEND)
                p = q + 1;
                q = strpbrk(p, delims);
                v_shader_names.emplace_back(p, q);
                p = q + 1;
                q = strpbrk(p, delims);
                f_shader_names.emplace_back(p, q);

                if (q && q[0] == '\r' && q[0] == '\n') {
                    p = q + 1;
                    q = strpbrk(p, delims);
                    tc_shader_names.emplace_back(p, q);
                    p = q + 1;
                    q = strpbrk(p, delims);
                    te_shader_names.emplace_back(p, q);
                } else {
                    tc_shader_names.emplace_back();
                    te_shader_names.emplace_back();
                }
#endif
                if (!q) {
                    break;
                }
                while (*q != '\n') {
                    ++q;
                }
                p = q + 1;
            }
            item.clear();
            if (p && q) {
                item = std::string(p, q);
            }
        }
        if (item == "flags:") {
            p = q + 1;
            q = strpbrk(p, delims);
            for (; p && q; q = strpbrk(p, delims)) {
                if (p == q) {
                    p = q + 1;
                    continue;
                }
                if (*p != '-') {
                    break;
                }
                p = q + 1;
                q = strpbrk(p, delims);
                const std::string flag = std::string(p, q);

                if (flag == "alpha_test") {
                    mat_main.flags |= eMatFlags::AlphaTest;
                } else if (flag == "alpha_blend") {
                    mat_main.flags |= eMatFlags::AlphaBlend;
                } else if (flag == "depth_write") {
                    mat_main.flags |= eMatFlags::DepthWrite;
                } else if (flag == "two_sided") {
                    mat_main.flags |= eMatFlags::TwoSided;
                } else if (flag == "emissive") {
                    mat_main.flags |= eMatFlags::Emissive;
                } else if (flag == "custom_shaded") {
                    mat_main.flags |= eMatFlags::CustomShaded;
                } else {
                    log->Error("Unknown flag %s", flag.c_str());
                }
                if (!q) {
                    break;
                }
                while (*q != '\n') {
                    ++q;
                }
                p = q + 1;
            }
            item.clear();
            if (p && q) {
                item = std::string(p, q);
            }
        }
        if (item == "textures:") {
            p = q + 1;
            q = strpbrk(p, delims);
            for (; p && q; q = strpbrk(p, delims)) {
                if (p == q) {
                    p = q + 1;
                    continue;
                }
                if (*p != '-') {
                    break;
                }
                p = q + 1;
                q = strpbrk(p, delims);
                const std::string texture_name = std::string(p, q);
                if (texture_name != "none") {
                    uint8_t texture_color[] = {0, 255, 255, 255};
                    Bitmask<eImgFlags> texture_flags;

                    const char *_p = q + 1;
                    const char *_q = strpbrk(_p, delims);

                    SamplingParams sampler_params = g_default_mat_sampler;

                    for (; _p && _q; _q = strpbrk(_p, delims)) {
                        if (_p == _q) {
                            break;
                        }

                        const char *flag = _p;
                        const int flag_len = int(_q - _p);

                        if (flag[0] == '#') {
                            texture_color[0] = from_hex_char(flag[1]) * 16 + from_hex_char(flag[2]);
                            texture_color[1] = from_hex_char(flag[3]) * 16 + from_hex_char(flag[4]);
                            texture_color[2] = from_hex_char(flag[5]) * 16 + from_hex_char(flag[6]);
                            texture_color[3] = from_hex_char(flag[7]) * 16 + from_hex_char(flag[8]);
                        } else if (strncmp(flag, "signed", flag_len) == 0) {
                            assert(false && "Deprecated flag!");
                        } else if (strncmp(flag, "srgb", flag_len) == 0) {
                            assert(false && "Deprecated flag!");
                        } else if (strncmp(flag, "norepeat", flag_len) == 0) {
                            assert(false && "Deprecated flag!");
                            sampler_params.wrap = eWrap::ClampToEdge;
                        } else {
                            break;
                        }

                        p = _p;
                        q = _q;

                        _p = _q + 1;
                    }

                    mat_main.textures.emplace_back(on_tex_load(texture_name, texture_color, texture_flags));
                    mat_main.samplers.emplace_back(on_sampler_load(sampler_params));
                } else {
                    mat_main.textures.emplace_back();
                    mat_main.samplers.emplace_back();
                }
                if (!q) {
                    break;
                }
                while (*q != '\n') {
                    ++q;
                }
                p = q + 1;
            }
            item.clear();
            if (p && q) {
                item = std::string(p, q);
            }
        }
        if (item == "params:") {
            p = q + 1;
            q = strpbrk(p, delims);
            for (; p && q; q = strpbrk(p, delims)) {
                if (p == q) {
                    p = q + 1;
                    continue;
                }
                if (p[0] != '-' || p[1] != ' ') {
                    break;
                }
                Vec4f &par = mat_cold.params.emplace_back();
                p = q + 1;
                q = strpbrk(p, delims);
                par[0] = strtof(p, nullptr);
                p = q + 1;
                q = strpbrk(p, delims);
                par[1] = strtof(p, nullptr);
                p = q + 1;
                q = strpbrk(p, delims);
                par[2] = strtof(p, nullptr);
                p = q + 1;
                q = strpbrk(p, delims);
                par[3] = strtof(p, nullptr);
                if (!q) {
                    break;
                }
                while (*q != '\n') {
                    ++q;
                }
                p = q + 1;
            }
            item.clear();
            if (p && q) {
                item = std::string(p, q);
            }
        }
        if (item == "---") {
            multi_doc = true;
            break;
        }

        if (!q) {
            break;
        }
        p = q + 1;
    }

    assert(mat_main.textures.size() == mat_main.samplers.size());

    for (size_t i = 0; i < v_shader_names.size(); ++i) {
        on_pipes_load(mat_main.flags, v_shader_names[i], f_shader_names[i], tc_shader_names[i], te_shader_names[i],
                      mat_main.pipelines);
    }

    return true;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
