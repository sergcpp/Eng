#include "ExDepthHierarchy.h"

#include <Ren/Context.h>

#include "../../utils/ShaderLoader.h"
#include "../Renderer_Structs.h"

Eng::ExDepthHierarchy::ExDepthHierarchy(ShaderLoader &sh, const view_state_t *view_state, const FgImgROHandle depth,
                                        const FgBufRWHandle atomic_counter, const FgImgRWHandle output) {
    view_state_ = view_state;

    depth_ = depth;
    atomic_ = atomic_counter;
    output_ = output;

    auto subgroup_select = [&sh](std::string_view subgroup_shader, std::string_view nosubgroup_shader) {
        return sh.ren_ctx().capabilities.subgroup ? subgroup_shader : nosubgroup_shader;
    };

    pi_depth_hierarchy_ = sh.FindOrCreatePipeline(subgroup_select(
        "internal/depth_hierarchy@MIPS_7.comp.glsl", "internal/depth_hierarchy@MIPS_7;NO_SUBGROUP.comp.glsl"));
}
