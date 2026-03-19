#include "ProgramVK.h"

#include "../Log.h"
#include "VKCtx.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace Ren {
extern const VkShaderStageFlagBits g_shader_stages_vk[];

bool InitDescrSetLayouts(const ApiContext &api, ProgramMain &prog_main,
                         const SparseDualStorage<ShaderMain, ShaderCold> &shaders, ILog *log) {
    SmallVector<VkDescriptorSetLayoutBinding, 16> layout_bindings[4];

    for (int i = 0; i < int(eShaderType::_Count); ++i) {
        const ShaderROHandle sh_handle = prog_main.shaders[i];
        if (!sh_handle) {
            continue;
        }

        const std::pair<const ShaderMain &, const ShaderCold &> sh = shaders.Get(sh_handle);
        for (const Descr &u : sh.second.unif_bindings) {
            auto &bindings = layout_bindings[u.set];

            const auto it = std::find_if(std::begin(bindings), std::end(bindings),
                                         [&u](const VkDescriptorSetLayoutBinding &b) { return u.loc == b.binding; });
            if (it == std::end(bindings)) {
                auto &new_binding = bindings.emplace_back();
                new_binding.binding = u.loc;
                new_binding.descriptorType = u.desc_type;

                if (bool(u.flags & eDescrFlags::UnboundedArray) && u.desc_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
                    assert(u.count == 1);
                    new_binding.descriptorCount = api.max_combined_image_samplers;
                } else {
                    new_binding.descriptorCount = u.count;
                }

                new_binding.stageFlags = g_shader_stages_vk[i];
                new_binding.pImmutableSamplers = nullptr;
            } else {
                it->stageFlags |= g_shader_stages_vk[i];
            }
        }
    }

    for (int i = 0; i < 4; ++i) {
        if (layout_bindings[i].empty()) {
            continue;
        }

        VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout_info.bindingCount = layout_bindings[i].size();
        layout_info.pBindings = layout_bindings[i].cdata();

        const VkDescriptorBindingFlagsEXT bind_flags[2] = {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT, 0};

        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extended_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT};
        extended_info.bindingCount = layout_info.bindingCount;
        extended_info.pBindingFlags = &bind_flags[0];

        if (i == 1) {
            layout_info.pNext = &extended_info;
        }

        prog_main.descr_set_layouts.emplace_back();
        const VkResult res =
            api.vkCreateDescriptorSetLayout(api.device, &layout_info, nullptr, &prog_main.descr_set_layouts.back());

        if (res != VK_SUCCESS) {
            log->Error("Failed to create descriptor set layout!");
            return false;
        }
    }

    return true;
}

void InitBindings(const ApiContext &api, ProgramMain &prog_main, ProgramCold &prog_cold,
                  const SparseDualStorage<ShaderMain, ShaderCold> &shaders, ILog *log) {
    for (int i = 0; i < int(eShaderType::_Count); ++i) {
        const ShaderROHandle sh_handle = prog_main.shaders[i];
        if (!sh_handle) {
            continue;
        }

        const std::pair<const ShaderMain &, const ShaderCold &> sh = shaders.Get(sh_handle);
        for (const Descr &u : sh.second.unif_bindings) {
            auto it = std::find(std::begin(prog_cold.uniforms), std::end(prog_cold.uniforms), u);
            if (it == std::end(prog_cold.uniforms)) {
                prog_cold.uniforms.emplace_back(u);
            }
        }

        for (const Range r : sh.second.pc_ranges) {
            auto it = std::find_if(
                std::begin(prog_main.pc_ranges), std::end(prog_main.pc_ranges),
                [&](const VkPushConstantRange &rng) { return r.offset == rng.offset && r.size == rng.size; });

            if (it == std::end(prog_main.pc_ranges)) {
                VkPushConstantRange &new_rng = prog_main.pc_ranges.emplace_back();
                new_rng.stageFlags = g_shader_stages_vk[i];
                new_rng.offset = r.offset;
                new_rng.size = r.size;
            } else {
                it->stageFlags |= g_shader_stages_vk[i];
            }
        }
    }

    if (const ShaderROHandle sh_handle = prog_main.shaders[int(eShaderType::Vertex)]) {
        const std::pair<const ShaderMain &, const ShaderCold &> sh = shaders.Get(sh_handle);
        for (const Descr &a : sh.second.attr_bindings) {
            prog_cold.attributes.emplace_back(a);
        }
    }

    std::sort(std::begin(prog_cold.uniforms), std::end(prog_cold.uniforms), [](const Descr &lhs, const Descr &rhs) {
        if (lhs.set == rhs.set) {
            return (lhs.loc < rhs.loc);
        }
        return (lhs.set < rhs.set);
    });
}
} // namespace Ren

bool Ren::Program_Init(const ApiContext &api, const SparseDualStorage<ShaderMain, ShaderCold> &shaders,
                       ProgramMain &prog_main, ProgramCold &prog_cold, const ShaderROHandle vs, const ShaderROHandle fs,
                       const ShaderROHandle tcs, const ShaderROHandle tes, const ShaderROHandle gs, ILog *log) {
    // store shaders
    prog_main.shaders[int(eShaderType::Vertex)] = vs;
    prog_main.shaders[int(eShaderType::Fragment)] = fs;
    prog_main.shaders[int(eShaderType::TesselationControl)] = tcs;
    prog_main.shaders[int(eShaderType::TesselationEvaluation)] = tes;
    prog_main.shaders[int(eShaderType::Geometry)] = gs;

    if (!InitDescrSetLayouts(api, prog_main, shaders, log)) {
        log->Error("Failed to initialize descriptor set layouts!");
        return false;
    }
    InitBindings(api, prog_main, prog_cold, shaders, log);

    return true;
}

bool Ren::Program_Init(const ApiContext &api, const SparseDualStorage<ShaderMain, ShaderCold> &shaders,
                       ProgramMain &prog_main, ProgramCold &prog_cold, const ShaderROHandle cs, ILog *log) {
    // store shader
    prog_main.shaders[int(eShaderType::Compute)] = cs;

    if (!InitDescrSetLayouts(api, prog_main, shaders, log)) {
        log->Error("Failed to initialize descriptor set layouts!");
        return false;
    }
    InitBindings(api, prog_main, prog_cold, shaders, log);

    return true;
}

bool Ren::Program_Init2(const ApiContext &api, const SparseDualStorage<ShaderMain, ShaderCold> &shaders,
                        ProgramMain &prog_main, ProgramCold &prog_cold, const ShaderROHandle rgs,
                        const ShaderROHandle chs, const ShaderROHandle ahs, const ShaderROHandle ms,
                        const ShaderROHandle is, ILog *log) {
    // store shaders
    prog_main.shaders[int(eShaderType::RayGen)] = rgs;
    prog_main.shaders[int(eShaderType::ClosestHit)] = chs;
    prog_main.shaders[int(eShaderType::AnyHit)] = ahs;
    prog_main.shaders[int(eShaderType::Miss)] = ms;
    prog_main.shaders[int(eShaderType::Intersection)] = is;

    if (!InitDescrSetLayouts(api, prog_main, shaders, log)) {
        log->Error("Failed to initialize descriptor set layouts!");
        return false;
    }
    InitBindings(api, prog_main, prog_cold, shaders, log);

    return true;
}

void Ren::Program_Destroy(const ApiContext &api, ProgramMain &prog_main, ProgramCold &prog_cold) {
    for (VkDescriptorSetLayout &l : prog_main.descr_set_layouts) {
        if (l) {
            api.vkDestroyDescriptorSetLayout(api.device, l, nullptr);
        }
    }
    prog_main = {};
    prog_cold = {};
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
