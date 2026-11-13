#pragma once

#include <cstdint>

#include "vkm_opengl/buffer/fence.h"
#include "vkm_opengl/core/object.h"

namespace Vkm::OpenGL {

class Buffer;

/**
 * @brief A buffer the GPU copies into and the CPU reads once the copy has finished, without
 *        waiting for it.
 *
 * Mapped for reading for its whole life (persistent and coherent), so a read is a pointer, not a
 * map. capture() or captureDefault() copies and places a fence; data() is the copy once arrived()
 * says it has landed. Several in turn let the CPU read a few frames behind the GPU.
 */
class ReadbackBuffer : public Object {
    public:
        ReadbackBuffer() = delete;
        ~ReadbackBuffer() override;

        ReadbackBuffer(const ReadbackBuffer& other) = delete;
        ReadbackBuffer& operator=(const ReadbackBuffer& other) = delete;

        ReadbackBuffer(ReadbackBuffer && other) noexcept;
        ReadbackBuffer& operator=(ReadbackBuffer && other) noexcept;

        /**
         * @brief Make and map @p bytes of storage.
         *
         * @param bytes Its size.
         * @throws std::runtime_error When the driver will not map it, having let it go.
         */
        explicit ReadbackBuffer(uint32_t bytes);

    public:
        /**
         * @brief Copy the first size() bytes of @p source, from @p offset, and mark when they land.
         *
         * @param source The buffer read from.
         * @param offset Where in it the copy begins.
         */
        void capture(const Buffer& source, uint32_t offset = 0);

        /**
         * @brief Copy a rectangle of the default framebuffer's read buffer, and mark when it lands.
         *
         * Read through the pixel pack buffer into the start of this buffer, rows bottom first at
         * the pack alignment. The read framebuffer and pixel pack buffer bindings are put back after.
         *
         * @param x      Left, in pixels from the window's lower left.
         * @param y      Bottom.
         * @param width  Width in pixels, more than zero.
         * @param height Height in pixels, more than zero; the rectangle, under the pack state, must fit
         *               in size(), every row padded, which the library's asserts check for the formats
         *               and types they know.
         * @param format GL format of what is read, e.g. GL_RGBA.
         * @param type   GL type of each channel, e.g. GL_UNSIGNED_BYTE.
         */
        void captureDefault(int32_t x, int32_t y, int32_t width, int32_t height, GLenum format, GLenum type);

        /**
         * @brief Whether the capture in flight has landed, asked without waiting.
         *
         * True once a capture: it is then no longer in flight.
         *
         * @return False while it is in flight, and with none in flight.
         */
        bool arrived();

        /**
         * @brief Block until the capture in flight has landed, for a host that cannot wait for
         *        arrived(), as one closing with a capture in flight.
         *
         * data() is then the copy, and no capture is in flight. Returns at once with none.
         */
        void wait();

        /// Whether a capture is in flight.
        bool pending() const { return m_fence.pending(); }

        /// Forget a capture in flight: it is never read.
        void drop();

        /// The copy, coherent with the GPU: read only once arrived() says it has landed.
        const void* data() const { return m_data; }

        uint32_t size() const { return m_bytes; }

    private:
        /// Unmap and delete the buffer and zero m_id.
        void release() noexcept;

    private:
        uint32_t    m_bytes = 0;
        const void* m_data  = nullptr;
        Fence       m_fence;
};

} // namespace Vkm::OpenGL
