#pragma once

#include <cstdint>

#include "../Shader.h"
#include "../utils/SmallVector.h"
#include "../utils/Span.h"
#include "../utils/Storage.h"

namespace Ren {
class ILog;
struct ApiContext;

struct ShaderMain {
    VkShaderModule vk_module = {};
};

struct ShaderCold {
    String name;
    eShaderType type = eShaderType::_Count;

    SmallVector<Attribute, 8> attr_bindings;
    SmallVector<Uniform, 16> unif_bindings;
    SmallVector<Range, 1> pc_ranges;
};

bool Shader_Init(const ApiContext &api, ShaderMain &shader_main, ShaderCold &shader_cold,
                 Span<const uint8_t> shader_code, const String &name, eShaderType type, ILog *log);
void Shader_Destroy(const ApiContext &api, ShaderMain &shader_main, ShaderCold &shader_cold);
} // namespace Ren