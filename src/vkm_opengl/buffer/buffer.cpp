#include "vkm_opengl/buffer/buffer.h"

#include <utility>

#include "vkm_log/assert.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

namespace {

GLuint   g_indirect       = 0;  ///< The indirect binding as vkmOpenGL last made it, for one context drawn
                                ///< from one thread.
uint64_t g_deletedBuffers = 0;  ///< Buffers deleteBuffer() has deleted, for deletedBuffers().

} // namespace

void bindRange(GLenum target, uint32_t binding, const BufferSpan& span) {
    VKM_OPENGL_CHECK(glBindBufferRange(target, binding, span.buffer, span.offset, span.bytes));
}

void bindIndirect(GLuint buffer) {
    if (buffer == g_indirect) return;
    VKM_OPENGL_CHECK(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer));
    g_indirect = buffer;
}

void deleteBuffer(GLuint buffer) {
    if (buffer == 0) return;
    VKM_OPENGL_CHECK(glDeleteBuffers(1, &buffer));
    // The driver unbinds a deleted buffer from where it was bound.
    if (buffer == g_indirect) g_indirect = 0;
    ++g_deletedBuffers;
}

uint64_t deletedBuffers() {
    return g_deletedBuffers;
}

Buffer::Buffer(GLenum target, const void* data, uint32_t size, GLenum usage)
    : Object(target, GL_BUFFER, 0)
    , m_size(size)
{
    VKM_OPENGL_CHECK(glCreateBuffers(1, &m_id));
    VKM_ASSERT(m_id != 0);
    VKM_OPENGL_CHECK(glNamedBufferData(m_id, m_size, data, usage));
}

Buffer::~Buffer() {
    release();
}

Buffer& Buffer::operator=(Buffer && other) noexcept {
    if (this != &other) {
        release();
        Object::operator=(std::move(other));
        m_size = other.m_size;
    }
    return *this;
}

void Buffer::release() noexcept {
    deleteBuffer(m_id);
    m_id = 0;
}

uint32_t Buffer::getSize() const {
    return m_size;
}

void Buffer::update(const void* data, uint32_t size, uint32_t offset) {
    VKM_ASSERT(offset + size <= m_size);
    VKM_OPENGL_CHECK(glNamedBufferSubData(m_id, offset, size, data));
}

void Buffer::bindBase(uint32_t bindingPoint) const {
    VKM_OPENGL_CHECK(glBindBufferBase(m_target, bindingPoint, m_id));
}

void Buffer::copyFrom(const Buffer& source, uint32_t sourceOffset, uint32_t offset, uint32_t bytes) {
    VKM_OPENGL_CHECK(glCopyNamedBufferSubData(source.getID(), m_id, sourceOffset, offset, bytes));
}

void Buffer::clear(GLenum internalFormat, GLenum format, GLenum type, const void* value) {
    VKM_OPENGL_CHECK(glClearNamedBufferData(m_id, internalFormat, format, type, value));
}

void Buffer::read(uint32_t offset, uint32_t bytes, void* out) const {
    VKM_OPENGL_CHECK(glGetNamedBufferSubData(m_id, offset, bytes, out));
}

} // namespace Vkm::OpenGL
