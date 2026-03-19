#pragma once

#include "../Ren/Image.h"
#include "../Ren/ImageRegion.h"

#include "BaseElement.h"

namespace Gui {
class Image : public BaseElement {
  protected:
    const Ren::SparseDualStorage<Ren::ImageRegionMain, Ren::ImageRegionCold> *storage_ = nullptr;
    Ren::ImageRegionHandle tex_;
    Vec2f uvs_px_[2];

  public:
    Image(const Ren::SparseDualStorage<Ren::ImageRegionMain, Ren::ImageRegionCold> *storage, Ren::ImageRegionHandle tex,
          const Vec2f &pos, const Vec2f &size, const BaseElement *parent);
    Image(Ren::Context &ctx, std::string_view tex_name, const Vec2f &pos, const Vec2f &size, const BaseElement *parent);

    [[nodiscard]] const Ren::SparseDualStorage<Ren::ImageRegionMain, Ren::ImageRegionCold> *storage() const {
        return storage_;
    }

    [[nodiscard]] Ren::ImageRegionHandle tex() { return tex_; }
    [[nodiscard]] const Ren::ImageRegionHandle tex() const { return tex_; }

    [[nodiscard]] const Vec2f *uvs_px() const { return uvs_px_; }

    void set_uvs(const Vec2f uvs[2]) {
        uvs_px_[0] = uvs[0];
        uvs_px_[1] = uvs[1];
    }

    void Draw(Renderer *r) override;

    void ResizeToContent(const Vec2f &pos);
};
} // namespace Gui
