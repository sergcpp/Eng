#include "AccStructure.h"

#include <Ren/Material.h>
#include <Ren/Mesh.h>
#include <Sys/Json.h>

const Ren::Bitmask<Eng::AccStructure::eRayType> Eng::AccStructure::DefaultVisMask =
    Ren::Bitmask<eRayType>{eRayType::Camera} | eRayType::Diffuse | eRayType::Specular | eRayType::Refraction |
    eRayType::Shadow;

void Eng::AccStructure::Read(const JsObjectP &js_in, AccStructure &acc) {
    acc.vis_mask = DefaultVisMask;

    if (js_in.Has("visible_to_camera")) {
        JsLiteral v = js_in.at("visible_to_camera").as_lit();
        if (v.val == JsLiteralType::True) {
            acc.vis_mask |= eRayType::Camera;
        } else {
            acc.vis_mask &= ~Ren::Bitmask(eRayType::Camera);
        }
    }

    if (js_in.Has("visible_to_diffuse")) {
        JsLiteral v = js_in.at("visible_to_diffuse").as_lit();
        if (v.val == JsLiteralType::True) {
            acc.vis_mask |= eRayType::Diffuse;
        } else {
            acc.vis_mask &= ~Ren::Bitmask(eRayType::Diffuse);
        }
    }

    if (js_in.Has("visible_to_specular")) {
        JsLiteral v = js_in.at("visible_to_specular").as_lit();
        if (v.val == JsLiteralType::True) {
            acc.vis_mask |= eRayType::Specular;
        } else {
            acc.vis_mask &= ~Ren::Bitmask(eRayType::Specular);
        }
    }

    if (js_in.Has("visible_to_refraction")) {
        JsLiteral v = js_in.at("visible_to_refraction").as_lit();
        if (v.val == JsLiteralType::True) {
            acc.vis_mask |= eRayType::Refraction;
        } else {
            acc.vis_mask &= ~Ren::Bitmask(eRayType::Refraction);
        }
    }

    if (js_in.Has("visible_to_shadow")) {
        JsLiteral v = js_in.at("visible_to_shadow").as_lit();
        if (v.val == JsLiteralType::True) {
            acc.vis_mask |= eRayType::Shadow;
        } else {
            acc.vis_mask &= ~Ren::Bitmask(eRayType::Shadow);
        }
    }
}

void Eng::AccStructure::Write(const AccStructure &acc, JsObjectP &js_out) {
    const auto &alloc = js_out.elements.get_allocator();

    if (acc.mesh) {
        // write mesh file name
        js_out.Insert("mesh_file", JsStringP{acc.mesh->name(), alloc});
    }

    if (!acc.material_override.empty()) {
        JsArrayP js_material_override(alloc);

        for (const auto &mat : acc.material_override) {
            js_material_override.Push(JsStringP{mat.first->name(), alloc});
        }

        js_out.Insert("material_override", std::move(js_material_override));
    }

    // write visibility
    if ((acc.vis_mask & eRayType::Camera) != (DefaultVisMask & eRayType::Camera)) {
        js_out.Insert("visible_to_camera",
                      JsLiteral((acc.vis_mask & eRayType::Camera) ? JsLiteralType::True : JsLiteralType::False));
    }
    if ((acc.vis_mask & eRayType::Diffuse) != (DefaultVisMask & eRayType::Diffuse)) {
        js_out.Insert("visible_to_diffuse",
                      JsLiteral((acc.vis_mask & eRayType::Diffuse) ? JsLiteralType::True : JsLiteralType::False));
    }
    if ((acc.vis_mask & eRayType::Specular) != (DefaultVisMask & eRayType::Specular)) {
        js_out.Insert("visible_to_specular",
                      JsLiteral((acc.vis_mask & eRayType::Specular) ? JsLiteralType::True : JsLiteralType::False));
    }
    if ((acc.vis_mask & eRayType::Refraction) != (DefaultVisMask & eRayType::Refraction)) {
        js_out.Insert("visible_to_refraction",
                      JsLiteral((acc.vis_mask & eRayType::Refraction) ? JsLiteralType::True : JsLiteralType::False));
    }
    if ((acc.vis_mask & eRayType::Shadow) != (DefaultVisMask & eRayType::Shadow)) {
        js_out.Insert("visible_to_shadow",
                      JsLiteral((acc.vis_mask & eRayType::Shadow) ? JsLiteralType::True : JsLiteralType::False));
    }
}