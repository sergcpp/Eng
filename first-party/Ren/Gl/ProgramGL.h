#pragma once

#include <cstdint>
#include <cstring>

#include <array>
#include <string>

#include "../Fwd.h"
#include "../Shader.h"
#include "../utils/SmallVector.h"
#include "../utils/SparseStorage.h"
#include "../utils/String.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace Ren {
class ILog;

struct ProgramMain {
    uint32_t id = 0;
    std::array<ShaderROHandle, int(eShaderType::_Count)> shaders;

    bool operator==(const ProgramMain &rhs) const { return shaders == rhs.shaders; }
    bool operator!=(const ProgramMain &rhs) const { return shaders != rhs.shaders; }
    bool operator<(const ProgramMain &rhs) const { return shaders < rhs.shaders; }

    bool has_tessellation() const {
        return shaders[int(eShaderType::TesselationControl)] && shaders[int(eShaderType::TesselationEvaluation)];
    }
};

struct ProgramCold {
    SmallVector<Attribute, 8> attributes;
    SmallVector<Uniform, 16> uniforms;
    SmallVector<UniformBlock, 1> uniform_blocks;

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

    const UniformBlock &uniform_block(const int i) const { return uniform_blocks[i]; }
    const UniformBlock &uniform_block(std::string_view name) const {
        for (int i = 0; i < int(uniform_blocks.size()); i++) {
            if (uniform_blocks[i].name == name) {
                return uniform_blocks[i];
            }
        }
        return uniform_blocks[0];
    }
};

bool Program_Init(const ApiContext &api, const SparseDualStorage<ShaderMain, ShaderCold> &shaders,
                  ProgramMain &prog_main, ProgramCold &prog_cold, ShaderROHandle vs, ShaderROHandle fs,
                  ShaderROHandle tcs, ShaderROHandle tes, ShaderROHandle gs, ILog *log);
bool Program_Init(const ApiContext &api, const SparseDualStorage<ShaderMain, ShaderCold> &shaders,
                  ProgramMain &prog_main, ProgramCold &prog_cold, ShaderROHandle cs, ILog *log);
void Program_Destroy(const ApiContext &api, ProgramMain &prog_main, ProgramCold &prog_cold);
} // namespace Ren

#ifdef _MSC_VER
#pragma warning(pop)
#endif