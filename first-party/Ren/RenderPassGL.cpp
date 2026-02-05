#include "RenderPass.h"

bool Ren::RenderPass_Init(const ApiContext &api, RenderPassMain &rp_main, const RenderTargetInfo &depth_rt,
                          Span<const RenderTargetInfo> color_rts, ILog *log) {
    rp_main.depth_rt = depth_rt;
    rp_main.color_rts.assign(std::begin(color_rts), std::end(color_rts));
    return true;
}

void Ren::RenderPass_Destroy(const ApiContext &api, RenderPassMain &rp_main) { rp_main = {}; }

void Ren::RenderPass_DestroyImmediately(const ApiContext &api, RenderPassMain &rp_main) {
    RenderPass_Destroy(api, rp_main);
}