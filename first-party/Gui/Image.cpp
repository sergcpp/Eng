#include "Image.h"

#include <fstream>
#include <memory>

#include "../Ren/Context.h"

#include "Renderer.h"

Gui::Image::Image(const Ren::SparseDualStorage<Ren::ImageRegionMain, Ren::ImageRegionCold> *storage,
                  Ren::ImageRegionHandle tex, const Vec2f &pos, const Vec2f &size, const BaseElement *parent)
    : BaseElement(pos, size, parent), storage_(storage), tex_(tex) {
    const auto &[reg_main, reg_cold] = (*storage_)[tex_];
    uvs_px_[0] = Vec2f{float(reg_main.pos[0]), float(reg_main.pos[1])};
    uvs_px_[1] = Vec2f{float(reg_main.pos[0] + reg_cold.params.w), float(reg_main.pos[1] + reg_cold.params.h)};
}

Gui::Image::Image(Ren::Context &ctx, std::string_view tex_name, const Vec2f &pos, const Vec2f &size,
                  const BaseElement *parent)
    : BaseElement(pos, size, parent) {
    uvs_px_[0] = uvs_px_[1] = Vec2f{0};

    std::ifstream in_file(tex_name.data(), std::ios::binary | std::ios::ate);
    const size_t in_file_size = size_t(in_file.tellg());
    in_file.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(in_file_size);
    in_file.read((char *)data.data(), std::streamsize(in_file_size));

    tex_ = ctx.CreateImageRegion(Ren::String{tex_name}, data, {}, ctx.current_cmd_buf());
    assert(tex_);
    storage_ = &ctx.image_regions();

    const auto &[reg_main, reg_cold] = (*storage_)[tex_];

    uvs_px_[0] = Vec2f{float(reg_main.pos[0]), float(reg_main.pos[1])};
    uvs_px_[1] = Vec2f{float(reg_main.pos[0] + reg_cold.params.w), float(reg_main.pos[1] + reg_cold.params.h)};
}

void Gui::Image::Draw(Renderer *r) {
    const Vec2f pos[2] = {dims_[0], dims_[0] + dims_[1]};

    const auto &[reg_main, reg_cold] = (*storage_)[tex_];
    const int tex_layer = reg_main.pos[2];

    r->PushImageQuad(eDrawMode::Passthrough, tex_layer, ColorWhite, pos, uvs_px_);
}

void Gui::Image::ResizeToContent(const Vec2f &pos) {
    const auto &[reg_main, reg_cold] = (*storage_)[tex_];
    const Vec2i parent_size_px = parent_->size_px();

    BaseElement::Resize(pos, Vec2f{2 * float(reg_cold.params.w) / float(parent_size_px[0]),
                                   2 * float(reg_cold.params.h) / float(parent_size_px[1])});
}
