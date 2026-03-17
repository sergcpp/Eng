#pragma once

#include <Ren/DrawCall.h>
#include <Ren/Framebuffer.h>
#include <Ren/Fwd.h>
#include <Ren/Image.h>
#include <Ren/Pipeline.h>
#include <Ren/RenderPass.h>
#include <Ren/VertexInput.h>
#include <Ren/math/Mat.h>

#if defined(REN_VK_BACKEND)
#include <Ren/DescriptorPool.h>
#endif

namespace Eng {
class ShaderLoader;

class FramebufferPool {
    std::vector<Ren::FramebufferHandle> framebuffers_;

  public:
    Ren::FramebufferHandle FindOrCreate(Ren::Context &ctx, Ren::RenderPassROHandle render_pass,
                                        const Ren::FramebufferAttachment &depth,
                                        const Ren::FramebufferAttachment &stencil,
                                        Ren::Span<const Ren::FramebufferAttachment> color_attachments);
    Ren::FramebufferHandle FindOrCreate(Ren::Context &ctx, Ren::RenderPassROHandle render_pass,
                                        const Ren::RenderTarget &depth, const Ren::RenderTarget &stencil,
                                        Ren::Span<const Ren::RenderTarget> color_attachments);
    Ren::FramebufferHandle FindOrCreate(Ren::Context &ctx, Ren::RenderPassROHandle render_pass,
                                        Ren::ImageRWHandle depth, Ren::ImageRWHandle stencil,
                                        Ren::Span<const Ren::ImageRWHandle> color_attachments);

    void Clear(Ren::Context &ctx);
};

class PrimDraw {
  public:
    struct Binding;
    struct RenderTarget;

    enum class ePrim { Quad, Sphere, _Count };

  private:
    bool initialized_ = false;

    Ren::SubAllocation quad_vtx1_, quad_vtx2_, quad_ndx_;
    Ren::SubAllocation sphere_vtx1_, sphere_vtx2_, sphere_ndx_;

    Ren::VertexInputHandle fs_quad_vtx_input_, sphere_vtx_input_;

    ShaderLoader *sh_ = nullptr;

    FramebufferPool framebuffers_;

  public:
    ~PrimDraw();

    bool LazyInit(ShaderLoader &sh);
    void CleanUp();

    void DrawPrim(Ren::CommandBuffer cmd_buf, ePrim prim, Ren::ProgramHandle p, const Ren::RenderTarget &depth_rt,
                  Ren::Span<const Ren::RenderTarget> color_rts, const Ren::RastState &new_rast_state,
                  Ren::RastState &applied_rast_state, Ren::Span<const Ren::Binding> bindings, const void *uniform_data,
                  int uniform_data_len, int uniform_data_offset, FramebufferPool *framebuffers = nullptr,
                  int instances = 1);

    void ClearTarget(Ren::CommandBuffer cmd_buf, const Ren::RenderTarget &depth_rt,
                     Ren::Span<const Ren::RenderTarget> color_rts, FramebufferPool *framebuffers = nullptr);
};
} // namespace Eng