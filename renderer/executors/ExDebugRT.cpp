#include "ExDebugRT.h"

#include <Ren/Context.h>

#include "../../utils/ShaderLoader.h"

void Eng::ExDebugRT::Execute(FgBuilder &builder) {
    LazyInit(builder.ctx(), builder.sh());

#if !defined(REN_GL_BACKEND)
    if (builder.ctx().capabilities.hwrt) {
        Execute_HWRT(builder);
    } else
#endif
    {
        Execute_SWRT(builder);
    }
}

void Eng::ExDebugRT::LazyInit(Ren::Context &ctx, Eng::ShaderLoader &sh) {
    if (!initialized) {
#if defined(REN_VK_BACKEND)
        if (ctx.capabilities.hwrt) {
            Ren::ProgramRef debug_hwrt_prog =
                sh.LoadProgram2("internal/rt_debug.rgen.glsl", "internal/rt_debug@GI_CACHE.rchit.glsl",
                                "internal/rt_debug.rahit.glsl", "internal/rt_debug.rmiss.glsl", {});
            if (!pi_debug_hwrt_.Init(ctx.api_ctx(), debug_hwrt_prog, ctx.log())) {
                ctx.log()->Error("ExDebugRT: Failed to initialize pipeline!");
            }
        }
#endif
        Ren::ProgramRef debug_swrt_prog = sh.LoadProgram("internal/rt_debug_swrt@GI_CACHE.comp.glsl");
        if (!pi_debug_swrt_.Init(ctx.api_ctx(), debug_swrt_prog, ctx.log())) {
            ctx.log()->Error("ExDebugRT: Failed to initialize pipeline!");
        }

        initialized = true;
    }
}
