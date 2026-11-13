#include "vkm_opengl/buffer/index_buffer.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

namespace {
    uint32_t getTypeSize(GLenum type) {
        switch (type) {
            case GL_UNSIGNED_INT:   return sizeof(GLuint);
            case GL_UNSIGNED_SHORT: return sizeof(GLushort);
            case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
            default:                return sizeof(GLuint);
        }
    }
}

IndexBuffer::IndexBuffer(const void* data, uint32_t count, GLenum type, GLenum usage)
    : Buffer(GL_ELEMENT_ARRAY_BUFFER, data, count * getTypeSize(type), usage)
    , m_count(count)
    , m_type(type)
{}

uint32_t IndexBuffer::getCount() const {
    return m_count;
}

GLenum IndexBuffer::getType() const {
    return m_type;
}

void IndexBuffer::draw(GLenum mode) const {
    VKM_OPENGL_CHECK(glDrawElements(mode, static_cast<GLsizei>(m_count), m_type, nullptr));
}

void IndexBuffer::drawInstanced(uint32_t instanceCount, uint32_t baseInstance, GLenum mode) const {
    VKM_OPENGL_CHECK(glDrawElementsInstancedBaseInstance(
        mode,
        static_cast<GLsizei>(m_count),
        m_type,
        nullptr,
        static_cast<GLsizei>(instanceCount),
        baseInstance
    ));
}

} // namespace Vkm::OpenGL
