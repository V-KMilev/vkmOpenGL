#include "vkm_opengl/buffer/buffer.h"

#include <utility>

#include "vkm_log/assert.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

namespace {

// One context, drawn from one thread: the indirect binding as vkmOpenGL last made it.
GLuint   g_indirect       = 0;
uint64_t g_deletedBuffers = 0;

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

Buffer::Buffer(
    GLenum target,
    const void* data,
    uint32_t size,
    GLenum usage
) : Object(target, GL_BUFFER, 0),
    m_size(size),
    m_usage(usage)
{
    VKM_OPENGL_CHECK(glCreateBuffers(1, &m_id));
    VKM_ASSERT(m_id != 0);
    VKM_OPENGL_CHECK(glNamedBufferData(m_id, m_size, data, m_usage));
}

Buffer::~Buffer() {
    release();
}

Buffer& Buffer::operator=(Buffer && other) noexcept {
    if (this != &other) {
        release();
        Object::operator=(std::move(other));
        m_size  = other.m_size;
        m_usage = other.m_usage;
    }
    return *this;
}

void Buffer::release() noexcept {
    deleteBuffer(m_id);
    m_id = 0;
}

void Buffer::bind(GLenum target) const {
    // The override is what lets one buffer serve two roles - storage a compute
    // stage writes and, say, the indirect commands a draw reads. Ignoring it
    // silently bound to the wrong target and left the intended one empty.
    const GLenum to = target == GL_NONE ? m_target : target;
    if (to == GL_DRAW_INDIRECT_BUFFER) {
        bindIndirect(m_id);
        return;
    }
    VKM_OPENGL_CHECK(glBindBuffer(to, m_id));
}

void Buffer::unbind(GLenum target) const {
    const GLenum to = target == GL_NONE ? m_target : target;
    if (to == GL_DRAW_INDIRECT_BUFFER) {
        bindIndirect(0);
        return;
    }
    VKM_OPENGL_CHECK(glBindBuffer(to, 0));
}

uint32_t Buffer::getSize() const {
    return m_size;
}

GLenum Buffer::getTarget() const {
    return m_target;
}

void Buffer::update(const void* data, uint32_t size, uint32_t offset) {
    VKM_ASSERT(offset + size <= m_size);
    VKM_OPENGL_CHECK(glNamedBufferSubData(m_id, offset, size, data));
}

void Buffer::allocate(uint32_t size, const void* data) {
    VKM_OPENGL_CHECK(glNamedBufferData(m_id, size, data, m_usage));
    m_size = size;
}

void Buffer::bindBase(uint32_t bindingPoint) const {
    VKM_OPENGL_CHECK(glBindBufferBase(m_target, bindingPoint, m_id));
}

void Buffer::bindRange(uint32_t bindingPoint, uint32_t offset, uint32_t size) const {
    VKM_OPENGL_CHECK(glBindBufferRange(m_target, bindingPoint, m_id, offset, size));
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

MappedBuffer::MappedBuffer(const Buffer& buffer, void* data)
    : m_buffer(&buffer)
    , m_data(data) {}

MappedBuffer::~MappedBuffer() {
    if (m_data == nullptr) return;
    VKM_OPENGL_CHECK(glUnmapNamedBuffer(m_buffer->getID()));
}

MappedBuffer::MappedBuffer(MappedBuffer && other) noexcept
    : m_buffer(other.m_buffer)
    , m_data(other.m_data) {
    other.m_data = nullptr;
}

MappedBuffer& MappedBuffer::operator=(MappedBuffer && other) noexcept {
    if (this != &other) {
        m_buffer     = other.m_buffer;
        m_data       = other.m_data;
        other.m_data = nullptr;
    }
    return *this;
}

MappedBuffer Buffer::map(GLenum access) {
    void* data = nullptr;
    VKM_OPENGL_CHECK(data = glMapNamedBuffer(m_id, access));
    return MappedBuffer(*this, data);
}

MappedBuffer Buffer::mapRange(uint32_t offset, uint32_t length, GLbitfield access) {
    VKM_ASSERT(offset + length <= m_size);
    void* data = nullptr;
    VKM_OPENGL_CHECK(data = glMapNamedBufferRange(m_id, offset, length, access));
    return MappedBuffer(*this, data);
}

} // namespace Vkm::OpenGL
