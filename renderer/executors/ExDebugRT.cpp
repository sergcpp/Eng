#include "ExDebugRT.h"

#include <Ren/Context.h>

#include "../../utils/ShaderLoader.h"

void Eng::ExDebugRT::Execute(FgBuilder &builder) {
    LazyInit(builder.ctx(), builder.sh());

#if !defined(USE_GL_RENDER)
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
#if defined(USE_VK_RENDER)
        if (ctx.capabilities.hwrt) {
            Ren::ProgramRef debug_hwrt_prog =
                sh.LoadProgram2(ctx, "internal/rt_debug.rgen.glsl", "internal/rt_debug@GI_CACHE.rchit.glsl",
                                "internal/rt_debug.rahit.glsl", "internal/rt_debug.rmiss.glsl", {});
            assert(debug_hwrt_prog->ready());

            if (!pi_debug_hwrt_.Init(ctx.api_ctx(), debug_hwrt_prog, ctx.log())) {
                ctx.log()->Error("ExDebugRT: Failed to initialize pipeline!");
            }
        }
#endif
        Ren::ProgramRef debug_swrt_prog = sh.LoadProgram(ctx, "internal/rt_debug_swrt@GI_CACHE.comp.glsl");
        assert(debug_swrt_prog->ready());

        if (!pi_debug_swrt_.Init(ctx.api_ctx(), debug_swrt_prog, ctx.log())) {
            ctx.log()->Error("ExDebugRT: Failed to initialize pipeline!");
        }

        initialized = true;
    }
}
