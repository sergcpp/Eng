#include "ExDepthHierarchy.h"

#include <Ren/Context.h>

#include "../../utils/ShaderLoader.h"
#include "../Renderer_Structs.h"

Eng::ExDepthHierarchy::ExDepthHierarchy(ShaderLoader &sh, const view_state_t *view_state, const FgImgROHandle depth_tex,
                                        const FgBufRWHandle atomic_counter, const FgImgRWHandle output_tex) {
    view_state_ = view_state;

    depth_tex_ = depth_tex;
    atomic_buf_ = atomic_counter;
    output_tex_ = output_tex;

    auto subgroup_select = [&sh](std::string_view subgroup_shader, std::string_view nosubgroup_shader) {
        return sh.ren_ctx().capabilities.subgroup ? subgroup_shader : nosubgroup_shader;
    };

    pi_depth_hierarchy_ = sh.FindOrCreatePipeline(subgroup_select(
        "internal/depth_hierarchy@MIPS_7.comp.glsl", "internal/depth_hierarchy@MIPS_7;NO_SUBGROUP.comp.glsl"));
}
