#pragma once

#include <cstdint>
#include <cstring>

#include <array>
#include <string>

#include "../Fwd.h"
#include "../Shader.h"
#include "../utils/SmallVector.h"
#include "../utils/Span.h"
#include "../utils/Storage.h"
#include "../utils/String.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace Ren {
class ILog;

struct ProgramMain {
    std::array<ShaderROHandle, int(eShaderType::_Count)> shaders;
    SmallVector<VkDescriptorSetLayout, 1> descr_set_layouts;
    SmallVector<VkPushConstantRange, 1> pc_ranges;

    bool operator==(const ProgramMain &rhs) const { return shaders == rhs.shaders; }
    bool operator!=(const ProgramMain &rhs) const { return shaders != rhs.shaders; }
    bool operator<(const ProgramMain &rhs) const { return shaders < rhs.shaders; }
};

struct ProgramCold {
    SmallVector<Attribute, 8> attributes;
    SmallVector<Uniform, 16> uniforms;

    const Attribute &attribute(const int i) const { return attributes[i]; }
    const Attribute &attribute(std::string_view name) const {
        for (int i = 0; i < int(attributes.size()); i++) {
            if (attributes[i].name == name) {
                return attributes[i];
            }
        }
        return attributes[0];
    }

    const Uniform &uniform(const int i) const { return uniforms[i]; }
    const Uniform &uniform(std::string_view name) const {
        for (int i = 0; i < int(uniforms.size()); i++) {
            if (uniforms[i].name == name) {
                return uniforms[i];
            }
        }
        return uniforms[0];
    }
    const Uniform &uniform_at(const int loc) const {
        int left = 0, right = uniforms.size() - 1;
        while (left <= right) {
            const int mid = left + (right - left) / 2;
            if (uniforms[mid].set == 0 && uniforms[mid].loc == loc) {
                return uniforms[mid];
            } else if (uniforms[mid].set == 0 && uniforms[mid].loc < loc) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        return uniforms[0];
    }
};

bool Program_Init(const ApiContext &api, const DualStorage<ShaderMain, ShaderCold> &shaders, ProgramMain &prog_main,
                  ProgramCold &prog_cold, ShaderROHandle vs, ShaderROHandle fs, ShaderROHandle tcs, ShaderROHandle tes,
                  ShaderROHandle gs, ILog *log);
bool Program_Init(const ApiContext &api, const DualStorage<ShaderMain, ShaderCold> &shaders, ProgramMain &prog_main,
                  ProgramCold &prog_cold, ShaderROHandle cs, ILog *log);
bool Program_Init2(const ApiContext &api, const DualStorage<ShaderMain, ShaderCold> &shaders, ProgramMain &prog_main,
                   ProgramCold &prog_cold, ShaderROHandle rgs, ShaderROHandle chs, ShaderROHandle ahs,
                   ShaderROHandle ms, ShaderROHandle is, ILog *log);
void Program_Destroy(const ApiContext &api, ProgramMain &prog_main, ProgramCold &prog_cold);
} // namespace Ren

#ifdef _MSC_VER
#pragma warning(pop)
#endif