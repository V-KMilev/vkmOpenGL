#pragma once

#include <cstdint>

#include "vkm_opengl/core/object.h"

namespace Vkm::OpenGL {

class Buffer;

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
 * @brief A buffer mapped into client address space, unmapped on destruction.
 *
 * Obtained from Buffer::map() / mapRange(), never constructed directly. Move
 * transfers the mapping, so a moved-from object unmaps nothing.
 */
class MappedBuffer {
    public:
        MappedBuffer() = delete;
        ~MappedBuffer();

        MappedBuffer(const MappedBuffer& other) = delete;
        MappedBuffer& operator=(const MappedBuffer& other) = delete;

        MappedBuffer(MappedBuffer && other) noexcept;
        MappedBuffer& operator=(MappedBuffer && other) noexcept;

    public:
        /**
         * @brief The mapped memory.
         *
         * @return Pointer to the mapping, or null if the map failed.
         */
        void* data() const { return m_data; }

        /**
         * @brief Whether the mapping succeeded.
         *
         * @return True when data() is safe to write through.
         */
        bool isValid() const { return m_data != nullptr; }

    private:
        friend class Buffer;

        MappedBuffer(const Buffer& buffer, void* data);

    private:
        const Buffer* m_buffer;
        void*           m_data;
};

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
         * @brief Binds the buffer to the specified target.
         * @param target OpenGL buffer target (default is GL_NONE, uses the internal target).
         */
        void bind(GLenum target = GL_NONE) const;

        /**
         * @brief Unbinds the buffer from the specified target.
         * @param target OpenGL buffer target (default is GL_NONE, uses the internal target).
         */
        void unbind(GLenum target = GL_NONE) const;

        /**
         * @brief Returns the buffer size in bytes.
         *
         * @return Size of the current storage allocation.
         */
        uint32_t getSize() const;

        /**
         * @brief The target this buffer was created against.
         *
         * @return The GL_*_BUFFER constant bind() uses when given no override.
         */
        GLenum getTarget() const;

        /**
         * @brief Updates a subset or the entirety of the buffer's data.
         */
        void update(const void* data, uint32_t size, uint32_t offset = 0);

        /**
         * @brief Reallocate the buffer's storage, discarding its contents.
         *
         * Orphaning: the GL name is unchanged, so vertex-array attribute
         * bindings that reference this buffer stay valid across a grow. Goes
         * through the wrapper rather than glNamedBufferData directly so the tracked
         * size cannot drift from the storage it describes.
         *
         * @param size New size in bytes.
         * @param data Optional initial contents (null allocates only).
         */
        void allocate(uint32_t size, const void* data = nullptr);

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
         * @brief Bind a sub-range of the buffer to an indexed binding point.
         *
         * Same target rule and same restriction as bindBase(); the shader sees
         * only the requested window.
         *
         * @param bindingPoint Index of the binding point the shader declares.
         * @param offset       Byte offset the range starts at.
         * @param size         Length of the range in bytes.
         */
        void bindRange(uint32_t bindingPoint, uint32_t offset, uint32_t size) const;

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

        /**
         * @brief Map the whole buffer into client address space.
         *
         * The mapping unmaps itself when the returned object goes out of
         * scope, which is why nothing here hands back a bare pointer: an
         * unmatched unmap leaves the buffer unusable by the GPU, and there is
         * no way to notice from the call site.
         *
         * @param access GL_READ_ONLY / GL_WRITE_ONLY / GL_READ_WRITE.
         * @return The mapping; check isValid() before writing through it.
         */
        [[nodiscard]] MappedBuffer map(GLenum access = GL_WRITE_ONLY);

        /**
         * @brief Map part of the buffer into client address space.
         *
         * Same scoped-unmap contract as map().
         *
         * @param offset Byte offset the mapping starts at.
         * @param length Length of the mapping in bytes.
         * @param access Bitfield of GL_MAP_* flags.
         * @return The mapping; check isValid() before writing through it.
         */
        [[nodiscard]] MappedBuffer mapRange(uint32_t offset, uint32_t length, GLbitfield access);

    private:
        /**
         * @brief Delete the GL buffer and zero m_id.
         *
         * Idempotent, so it is safe on a moved-from buffer or after a previous release().
         */
        void release() noexcept;

    protected:
        uint32_t m_size;
        GLenum   m_usage;
};

} // namespace Vkm::OpenGL
