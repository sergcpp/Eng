#pragma once

#include <cstdint>

#include "../Shader.h"
#include "../utils/SmallVector.h"
#include "../utils/Span.h"
#include "../utils/Storage.h"
#include "../utils/String.h"

namespace Ren {
struct ApiContext;
class ILog;

struct ShaderMain {
    uint32_t id = 0;
};

struct ShaderCold {
    String name;
    eShaderType type = eShaderType::_Count;
    eShaderSource source = eShaderSource::_Count;

    SmallVector<Attribute, 8> attr_bindings;
    SmallVector<Uniform, 16> unif_bindings;
    SmallVector<UniformBlock, 1> blck_bindings;
};

bool Shader_Init(const ApiContext &api, ShaderMain &shader_main, ShaderCold &shader_cold, std::string_view shader_src,
                 const String &name, eShaderType type, ILog *log);
bool Shader_Init(const ApiContext &api, ShaderMain &shader_main, ShaderCold &shader_cold,
                 Span<const uint8_t> shader_code, const String &name, eShaderType type, ILog *log);
void Shader_Destroy(const ApiContext &api, ShaderMain &shader_main, ShaderCold &shader_cold);
} // namespace Ren