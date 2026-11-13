#include "vkm_opengl/buffer/readback_buffer.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "vkm_log/assert.h"

#include "vkm_opengl/buffer/buffer.h"
#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

namespace {

constexpr GLbitfield READ_MAPPED = GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

/**
 * @brief The bytes a read of a rectangle packs under the pack state set now, for the asserts.
 *
 * @param width  Width in pixels.
 * @param height Height in pixels.
 * @param format GL format read.
 * @param type   GL type read.
 * @return The bytes; zero for a format or type not known here, which nothing then checks.
 */
[[maybe_unused]] size_t packedBytes(int32_t width, int32_t height, GLenum format, GLenum type) {
    size_t channels = 0;
    switch (format) {
        case GL_RED: case GL_GREEN: case GL_BLUE: case GL_ALPHA: case GL_RED_INTEGER:
        case GL_DEPTH_COMPONENT: case GL_STENCIL_INDEX: case GL_DEPTH_STENCIL: channels = 1; break;
        case GL_RG: case GL_RG_INTEGER: channels = 2; break;
        case GL_RGB: case GL_BGR: case GL_RGB_INTEGER: channels = 3; break;
        case GL_RGBA: case GL_BGRA: case GL_RGBA_INTEGER: channels = 4; break;
        default: return 0;
    }
    size_t pixel = 0;
    switch (type) {
        case GL_UNSIGNED_BYTE: case GL_BYTE: pixel = channels; break;
        case GL_UNSIGNED_SHORT: case GL_SHORT: case GL_HALF_FLOAT: pixel = 2 * channels; break;
        case GL_UNSIGNED_INT: case GL_INT: case GL_FLOAT: pixel = 4 * channels; break;
        // Packed: one word a pixel, whatever the channels.
        case GL_UNSIGNED_INT_24_8: case GL_UNSIGNED_INT_10F_11F_11F_REV: case GL_UNSIGNED_INT_2_10_10_10_REV:
        case GL_UNSIGNED_INT_8_8_8_8: case GL_UNSIGNED_INT_8_8_8_8_REV: pixel = 4; break;
        default: return 0;
    }
    GLint alignment = 4;
    GLint rowLength = 0;
    GLint skipRows  = 0;
    GLint skipped   = 0;
    VKM_OPENGL_CHECK(glGetIntegerv(GL_PACK_ALIGNMENT, &alignment));
    VKM_OPENGL_CHECK(glGetIntegerv(GL_PACK_ROW_LENGTH, &rowLength));
    VKM_OPENGL_CHECK(glGetIntegerv(GL_PACK_SKIP_ROWS, &skipRows));
    VKM_OPENGL_CHECK(glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipped));
    const size_t row    = pixel * static_cast<size_t>(rowLength > 0 ? rowLength : width);
    const size_t stride = (row + alignment - 1) / alignment * alignment;
    const size_t start  = stride * static_cast<size_t>(skipRows) + pixel * static_cast<size_t>(skipped);
    // Every row padded, the last too, as one driver asks in the default layout; with a row length or a
    // skip set this asks for more than drivers need, which stops loudly rather than fails unseen.
    return start + stride * static_cast<size_t>(height);
}

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
    , m_bytes(std::exchange(other.m_bytes, 0))
    , m_data(std::exchange(other.m_data, nullptr))
    , m_fence(std::move(other.m_fence)) {}

ReadbackBuffer& ReadbackBuffer::operator=(ReadbackBuffer && other) noexcept {
    if (this != &other) {
        release();
        Object::operator=(std::move(other));
        m_bytes  = std::exchange(other.m_bytes, 0);
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
    deleteBuffer(m_id);
    m_id = 0;
}

void ReadbackBuffer::capture(const Buffer& source, uint32_t offset) {
    VKM_OPENGL_CHECK(glCopyNamedBufferSubData(source.getID(), m_id, offset, 0, m_bytes));
    m_fence.place();
}

void ReadbackBuffer::captureDefault(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height,
    GLenum format,
    GLenum type
) {
    VKM_ASSERT(
        width > 0 && height > 0 && packedBytes(width, height, format, type) <= m_bytes,
        "a %dx%d capture is empty or does not fit a %u byte readback buffer",
        width,
        height,
        m_bytes
    );
    GLint reading = 0;
    GLint packing = 0;
    VKM_OPENGL_CHECK(glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &reading));
    VKM_OPENGL_CHECK(glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packing));
    VKM_OPENGL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));
    VKM_OPENGL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, m_id));
    const auto room = static_cast<GLsizei>(m_bytes);
    VKM_OPENGL_CHECK(glReadnPixels(x, y, width, height, format, type, room, nullptr));
    VKM_OPENGL_CHECK(glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(packing)));
    VKM_OPENGL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(reading)));
    m_fence.place();
}

bool ReadbackBuffer::arrived() {
    return m_fence.pending() && m_fence.passed();
}

void ReadbackBuffer::wait() {
    m_fence.wait();
}

void ReadbackBuffer::drop() {
    m_fence = Fence{};
}

} // namespace Vkm::OpenGL
