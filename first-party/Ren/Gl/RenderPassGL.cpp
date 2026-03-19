#include "../RenderPass.h"

bool Ren::RenderPass_Init(const ApiContext &api, RenderPass &rp, const RenderTargetInfo &depth_rt,
                          Span<const RenderTargetInfo> color_rts, ILog *log) {
    rp.depth_rt = depth_rt;
    rp.color_rts.assign(std::begin(color_rts), std::end(color_rts));
    return true;
}

void Ren::RenderPass_Destroy(const ApiContext &api, RenderPass &rp) { rp = {}; }

void Ren::RenderPass_DestroyImmediately(const ApiContext &api, RenderPass &rp) { RenderPass_Destroy(api, rp); }