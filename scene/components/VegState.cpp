#include "VegState.h"

#include <Sys/Json.h>

void Eng::VegState::Read(const Sys::JsObjectP &js_in, VegState &vs) {
    if (js_in.Has("movement_scale")) {
        vs.movement_scale = float(js_in.at("movement_scale").as_num().val);
    } else {
        vs.movement_scale = 1.0f;
    }

    if (js_in.Has("tree_mode")) {
        vs.tree_mode = float(js_in.at("tree_mode").as_num().val);
    } else {
        vs.tree_mode = 1.0f;
    }

    if (js_in.Has("bend_scale")) {
        vs.bend_scale = float(js_in.at("bend_scale").as_num().val);
    } else {
        vs.bend_scale = 1.0f;
    }

    if (js_in.Has("stretch")) {
        vs.stretch = float(js_in.at("stretch").as_num().val);
    } else {
        vs.stretch = 0.0f;
    }
}

void Eng::VegState::Write(const VegState &vs, Sys::JsObjectP &js_out) {
    if (vs.movement_scale != 1.0f) {
        js_out["movement_scale"] = Sys::JsNumber{vs.movement_scale};
    }

    if (vs.tree_mode != 1.0f) {
        js_out["tree_mode"] = Sys::JsNumber{vs.tree_mode};
    }

    if (vs.bend_scale != 1.0f) {
        js_out["bend_scale"] = Sys::JsNumber{vs.bend_scale};
    }

    if (vs.stretch != 0.0f) {
        js_out["stretch"] = Sys::JsNumber{vs.stretch};
    }
}
