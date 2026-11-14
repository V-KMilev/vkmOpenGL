#pragma once

#include <cstdint>

#include "vkm_opengl/core/object.h"

namespace Vkm::OpenGL {

/**
 * @brief A range of a buffer: a whole buffer, or where a StreamRing write() put its bytes.
 *
 * The buffer is named, not implied, because a ring that outgrows itself moves to a new one
 * mid-frame and what was written before stays where it was.
 */
struct BufferSpan {
    GLuint   buffer = 0;
    uint32_t offset = 0;
    uint32_t bytes  = 0;

    /// Whether it holds any bytes.
    explicit operator bool() const { return bytes > 0; }
};

/**
 * @brief Bind @p span to an indexed binding point of @p target.
 *
 * @param target  GL_SHADER_STORAGE_BUFFER or GL_UNIFORM_BUFFER.
 * @param binding The binding point.
 * @param span    The range; one a StreamRing wrote must be bound in the frame it was written.
 */
void bindRange(GLenum target, uint32_t binding, const BufferSpan& span);

/**
 * @brief Bind @p buffer as the draw-indirect buffer, unless it is bound there already.
 *
 * A draw that binds again what is bound costs the driver a revalidation all the same.
 *
 * @param buffer The buffer the commands lie in; 0 binds none.
 */
void bindIndirect(GLuint buffer);

/**
 * @brief Delete @p buffer, and forget it wherever vkmOpenGL remembers a binding to it.
 *
 * The one way vkmOpenGL deletes a buffer: the driver hands a deleted name out again, and a binding
 * remembered by that name (bindIndirect, VertexArray::setVertexBuffer) would read as made.
 *
 * @param buffer The buffer; 0 is none.
 */
void deleteBuffer(GLuint buffer);

/// How many buffers deleteBuffer() has deleted; a binding remembered at another count is made again.
uint64_t deletedBuffers();

/**
 * @brief Abstract base class for OpenGL buffer objects.
 *
 * Provides a common interface and resource management for GPU buffer objects.
 * Not constructible directly, only via derived classes (e.g., VertexBuffer, ShaderStorageBuffer).
 */
class Buffer : public Object {
    public:
        Buffer() = delete;

        Buffer(const Buffer& other) = delete;
        Buffer& operator=(const Buffer& other) = delete;

        Buffer(Buffer && other) noexcept = default;
        Buffer& operator=(Buffer && other) noexcept;

    protected:
        Buffer(GLenum target, const void* data, uint32_t size, GLenum usage);
        ~Buffer() override;

    public:
        /**
         * @brief Returns the buffer size in bytes.
         *
         * @return Size of the current storage allocation.
         */
        uint32_t getSize() const;

        /**
         * @brief Updates a subset or the entirety of the buffer's data.
         */
        void update(const void* data, uint32_t size, uint32_t offset = 0);

        /**
         * @brief Bind the whole buffer to an indexed binding point.
         *
         * Uses the buffer's own target, so a UniformBuffer binds as a UBO and a
         * ShaderStorageBuffer as an SSBO with no per-subclass override. Only the
         * indexed targets accept this (uniform, shader-storage, atomic-counter,
         * transform-feedback); calling it on a vertex or index buffer is a GL
         * error, which VKM_OPENGL_CHECK reports when VKM_OPENGL_CHECKS is on.
         *
         * @param bindingPoint Index of the binding point the shader declares.
         */
        void bindBase(uint32_t bindingPoint) const;

        /**
         * @brief Copy @p bytes of @p source, from @p sourceOffset, into this buffer at @p offset.
         *
         * @param source       The buffer copied from; may be this one, the ranges apart.
         * @param sourceOffset Where in it the copy begins.
         * @param offset       Where in this buffer it lands.
         * @param bytes        How many bytes.
         */
        void copyFrom(const Buffer& source, uint32_t sourceOffset, uint32_t offset, uint32_t bytes);

        /**
         * @brief Fill the whole buffer with one value repeated.
         *
         * @param internalFormat The sized format each repeat is stored as, e.g. GL_R32UI.
         * @param format         GL format of @p value, e.g. GL_RED_INTEGER.
         * @param type           GL type of its channels.
         * @param value          One repeat's worth; null fills with zeros.
         */
        void clear(GLenum internalFormat, GLenum format, GLenum type, const void* value);

        /**
         * @brief Read @p bytes from @p offset back into @p out.
         *
         * Waits for every command writing it: for a test or a bake, not a frame.
         *
         * @param offset Where the read begins.
         * @param bytes  How many bytes.
         * @param out    Where they land, room for @p bytes.
         */
        void read(uint32_t offset, uint32_t bytes, void* out) const;

    private:
        /**
         * @brief Delete the GL buffer and zero m_id.
         *
         * Idempotent, so it is safe on a moved-from buffer or after a previous release().
         */
        void release() noexcept;

    protected:
        uint32_t m_size;
};

} // namespace Vkm::OpenGL
