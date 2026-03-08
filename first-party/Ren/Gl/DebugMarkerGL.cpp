#include "../DebugMarker.h"

#include "GL.h"

Ren::DebugMarker::DebugMarker(const ApiContext &api, CommandBuffer cmd_buf, std::string_view name) : api_(api) {
#ifndef DISABLE_MARKERS
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, GLsizei(name.length()), name.data());
#endif
}

Ren::DebugMarker::~DebugMarker() {
#ifndef DISABLE_MARKERS
    glPopDebugGroup();
#endif
}