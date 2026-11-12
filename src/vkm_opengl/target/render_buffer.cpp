#include "vkm_opengl/target/render_buffer.h"

#include "vkm_log/assert.h"
#include "vkm_log/logger.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

RenderBuffer::RenderBuffer()
: Object(GL_RENDERBUFFER, GL_RENDERBUFFER, 0)
, m_width(0)
, m_height(0) {
    VKM_OPENGL_CHECK(glCreateRenderbuffers(1, &m_id));
    VKM_ASSERT(m_id != 0);
}

RenderBuffer::~RenderBuffer() {
    release();
}

RenderBuffer& RenderBuffer::operator=(RenderBuffer && other) noexcept {
    if (this != &other) {
        release();
        Object::operator=(std::move(other));
        m_width  = other.m_width;
        m_height = other.m_height;
    }
    return *this;
}

void RenderBuffer::release() noexcept {
    if (m_id == 0) return;
    VKM_OPENGL_CHECK(glDeleteRenderbuffers(1, &m_id));
    m_id = 0;
}

void RenderBuffer::bind() const {
    VKM_OPENGL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, m_id));
}

void RenderBuffer::unbind() const {
    VKM_OPENGL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

int32_t RenderBuffer::getWidth() const {
    return m_width;
}

int32_t RenderBuffer::getHeight() const {
    return m_height;
}

void RenderBuffer::storage(GLenum internalFormat, int32_t width, int32_t height) {
    m_width = width;
    m_height = height;

    VKM_OPENGL_CHECK(glNamedRenderbufferStorage(m_id, internalFormat, width, height));
}

void RenderBuffer::storageMultisample(int32_t samples, GLenum internalFormat, int32_t width, int32_t height) {
    m_width = width;
    m_height = height;

    VKM_OPENGL_CHECK(glNamedRenderbufferStorageMultisample(m_id, samples, internalFormat, width, height));
}

} // namespace Vkm::OpenGL
