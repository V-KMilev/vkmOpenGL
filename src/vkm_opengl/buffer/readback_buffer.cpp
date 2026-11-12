#include "vkm_opengl/buffer/readback_buffer.h"

#include <stdexcept>
#include <utility>

#include "vkm_opengl/buffer/buffer.h"
#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

namespace {

constexpr GLbitfield READ_MAPPED = GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

} // namespace

ReadbackBuffer::ReadbackBuffer(uint32_t bytes)
    : Object(GL_COPY_WRITE_BUFFER, GL_BUFFER, 0)
    , m_bytes(bytes)
{
    VKM_OPENGL_CHECK(glCreateBuffers(1, &m_id));
    VKM_OPENGL_CHECK(glNamedBufferStorage(m_id, bytes, nullptr, READ_MAPPED));
    VKM_OPENGL_CHECK(m_data = glMapNamedBufferRange(m_id, 0, bytes, READ_MAPPED));
    if (m_data == nullptr) {
        release();
        throw std::runtime_error("A readback buffer could not be mapped");
    }
}

ReadbackBuffer::~ReadbackBuffer() {
    release();
}

ReadbackBuffer::ReadbackBuffer(ReadbackBuffer && other) noexcept
    : Object(std::move(other))
    , m_bytes(other.m_bytes)
    , m_data(std::exchange(other.m_data, nullptr))
    , m_fence(std::move(other.m_fence)) {}

ReadbackBuffer& ReadbackBuffer::operator=(ReadbackBuffer && other) noexcept {
    if (this != &other) {
        release();
        Object::operator=(std::move(other));
        m_bytes  = other.m_bytes;
        m_data   = std::exchange(other.m_data, nullptr);
        m_fence  = std::move(other.m_fence);
    }
    return *this;
}

void ReadbackBuffer::release() noexcept {
    m_fence = Fence{};
    if (m_id == 0) return;
    if (m_data != nullptr) VKM_OPENGL_CHECK(glUnmapNamedBuffer(m_id));
    m_data = nullptr;
    VKM_OPENGL_CHECK(glDeleteBuffers(1, &m_id));
    m_id = 0;
}

void ReadbackBuffer::capture(const Buffer& source, uint32_t offset) {
    VKM_OPENGL_CHECK(glCopyNamedBufferSubData(source.getID(), m_id, offset, 0, m_bytes));
    m_fence.place();
}

bool ReadbackBuffer::arrived() {
    return m_fence.pending() && m_fence.passed();
}

void ReadbackBuffer::drop() {
    m_fence = Fence{};
}

} // namespace Vkm::OpenGL
