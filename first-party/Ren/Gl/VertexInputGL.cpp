#include "../VertexInput.h"

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

extern const int g_type_sizes[];
} // namespace Ren

bool Ren::VertexInput_Init(VertexInput &vtx_input, Span<const VtxAttribDesc> _attribs) {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    vtx_input.attribs.assign(_attribs.begin(), _attribs.end());
    vtx_input.gl_vao = uint32_t(vao);

    SmallVector<std::pair<int, uint32_t>, 8> bound_buffers;
    for (int i = 0; i < int(vtx_input.attribs.size()); i++) {
        const VtxAttribDesc &a = vtx_input.attribs[i];
        if (IsIntegerType(a.type)) {
            glVertexAttribIFormat(GLuint(a.loc), GLint(a.size), g_attrib_types_gl[int(a.type)], GLuint(a.rel_offset));
        } else {
            glVertexAttribFormat(GLuint(a.loc), GLint(a.size), g_attrib_types_gl[int(a.type)],
                                 IsNormalizedType(a.type) ? GL_TRUE : GL_FALSE, GLuint(a.rel_offset));
        }
        int bound_index = -1;
        for (int i = 0; i < int(bound_buffers.size()); ++i) {
            if (bound_buffers[i].first == a.buf && bound_buffers[i].second == a.base_offset) {
                bound_index = i;
                break;
            }
        }
        if (bound_index == -1) {
            bound_index = int(bound_buffers.size());
            bound_buffers.emplace_back(a.buf, a.base_offset);
        }
        glVertexAttribBinding(GLuint(a.loc), GLuint(bound_index));
        glEnableVertexAttribArray(GLuint(a.loc));
    }

    glBindVertexArray(0);

    return true;
}

void Ren::VertexInput_Destroy(VertexInput &vtx_input) {
    if (vtx_input.gl_vao) {
        auto vao = GLuint(vtx_input.gl_vao);
        glDeleteVertexArrays(1, &vao);
    }
    vtx_input = {};
}

void Ren::VertexInput_BindBuffers(const ApiContext &api, const VertexInput &vtx_input,
                                  const SparseDualStorage<BufferMain, BufferCold> &buffers,
                                  Span<const BufferROHandle> attrib_bufs, const BufferROHandle elem_buf) {
    glBindVertexArray(GLuint(vtx_input.gl_vao));
    SmallVector<std::pair<int, uint32_t>, 8> bound_buffers;
    for (const VtxAttribDesc &a : vtx_input.attribs) {
        int bound_index = -1;
        for (int i = 0; i < int(bound_buffers.size()); ++i) {
            if (bound_buffers[i].first == a.buf && bound_buffers[i].second == a.base_offset) {
                bound_index = i;
                break;
            }
        }
        if (bound_index == -1) {
            GLsizei stride = a.stride;
            if (!stride) {
                stride = uint32_t(g_type_sizes[int(a.type)]) * a.size;
            }
            glBindVertexBuffer(GLuint(bound_buffers.size()), buffers.Get(attrib_bufs[a.buf]).first.buf,
                               GLintptr(a.base_offset), stride);
            bound_index = int(bound_buffers.size());
            bound_buffers.emplace_back(a.buf, a.base_offset);
        }
    }
    if (elem_buf) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.Get(elem_buf).first.buf);
    }
}
