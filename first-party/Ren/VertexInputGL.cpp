#include "VertexInput.h"

#include "GL.h"

namespace Ren {
const uint32_t g_attrib_types_gl[] = {
    0xffffffff,        // Undefined
    GL_HALF_FLOAT,     // Float16
    GL_FLOAT,          // Float32
    GL_UNSIGNED_INT,   // Uint32
    GL_UNSIGNED_SHORT, // Uint16
    GL_UNSIGNED_SHORT, // Uint16UNorm
    GL_SHORT,          // Int16SNorm
    GL_UNSIGNED_BYTE,  // Uint8UNorm
    GL_INT,            // Int32
};
static_assert(std::size(g_attrib_types_gl) == size_t(eType::_Count));

bool IsIntegerType(const eType type) { return type == eType::Uint32 || type == eType::Int32 || type == eType::Uint16; }
bool IsNormalizedType(const eType type) {
    return type == eType::Uint16_unorm || type == eType::Int16_snorm || type == eType::Uint8_unorm;
}
} // namespace Ren

bool Ren::VertexInput_Init(VertexInputMain &vtx_input, Span<const VtxAttribDesc> _attribs,
                           const BufferROHandle _elem_buf) {
    vtx_input.attribs.assign(_attribs.begin(), _attribs.end());
    vtx_input.elem_buf = _elem_buf;
    vtx_input.cached_attribs_buf.resize(uint32_t(_attribs.size()));
    return true;
}

void Ren::VertexInput_Destroy(VertexInputMain &vtx_input) {
    if (vtx_input.gl_vao) {
        auto vao = GLuint(vtx_input.gl_vao);
        glDeleteVertexArrays(1, &vao);
    }
    vtx_input = {};
}

uint32_t Ren::VertexInput_GetVAO(const VertexInputMain &vtx_input, const DualStorage<BufferMain, BufferCold> &buffers) {
    bool changed = false;
    if (vtx_input.elem_buf) {
        changed |= (buffers.Get(vtx_input.elem_buf).first.buf != vtx_input.cached_elem_buf.first);
        changed |= (buffers.Get(vtx_input.elem_buf).first.generation != vtx_input.cached_elem_buf.second);
    }
    for (int i = 0; i < int(vtx_input.attribs.size()); ++i) {
        changed |= (buffers.Get(vtx_input.attribs[i].buf).first.buf != vtx_input.cached_attribs_buf[i].first);
        changed |= (buffers.Get(vtx_input.attribs[i].buf).first.generation != vtx_input.cached_attribs_buf[i].second);
    }
    if (changed) {
        if (!vtx_input.gl_vao) {
            GLuint vao;
            glGenVertexArrays(1, &vao);
            vtx_input.gl_vao = uint32_t(vao);
        }
        glBindVertexArray(GLuint(vtx_input.gl_vao));

        for (int i = 0; i < int(vtx_input.attribs.size()); i++) {
            const VtxAttribDesc &a = vtx_input.attribs[i];

            glBindBuffer(GL_ARRAY_BUFFER, buffers.Get(a.buf).first.buf);
            glEnableVertexAttribArray(GLuint(a.loc));
            if (IsIntegerType(a.type)) {
                glVertexAttribIPointer(GLuint(a.loc), GLint(a.size), g_attrib_types_gl[int(a.type)], GLsizei(a.stride),
                                       reinterpret_cast<void *>(uintptr_t(a.offset)));
            } else {
                glVertexAttribPointer(GLuint(a.loc), GLint(a.size), g_attrib_types_gl[int(a.type)],
                                      IsNormalizedType(a.type) ? GL_TRUE : GL_FALSE, GLsizei(a.stride),
                                      reinterpret_cast<void *>(uintptr_t(a.offset)));
            }

            vtx_input.cached_attribs_buf[i].first = buffers.Get(a.buf).first.buf;
            vtx_input.cached_attribs_buf[i].second = buffers.Get(a.buf).first.generation;
        }
        if (vtx_input.elem_buf) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.Get(vtx_input.elem_buf).first.buf);
            vtx_input.cached_elem_buf.first = buffers.Get(vtx_input.elem_buf).first.buf;
            vtx_input.cached_elem_buf.second = buffers.Get(vtx_input.elem_buf).first.generation;
        }
        glBindVertexArray(0);
    }
    return vtx_input.gl_vao;
}
