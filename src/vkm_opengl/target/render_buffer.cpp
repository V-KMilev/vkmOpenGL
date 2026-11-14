#include "vkm_opengl/target/render_buffer.h"

#include "vkm_log/assert.h"
#include "vkm_log/logger.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

RenderBuffer::RenderBuffer() : Object(GL_RENDERBUFFER, GL_RENDERBUFFER, 0) {
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
    }
    return *this;
}

void RenderBuffer::release() noexcept {
    if (m_id == 0) return;
    VKM_OPENGL_CHECK(glDeleteRenderbuffers(1, &m_id));
    m_id = 0;
}

void RenderBuffer::storage(GLenum internalFormat, int32_t width, int32_t height) {
    VKM_OPENGL_CHECK(glNamedRenderbufferStorage(m_id, internalFormat, width, height));
}

} // namespace Vkm::OpenGL
