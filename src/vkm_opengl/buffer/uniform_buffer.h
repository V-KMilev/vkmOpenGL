#pragma once

#include "vkm_opengl/buffer/buffer.h"

namespace Vkm::OpenGL {

/**
 * @brief Uniform buffer (UBO) wrapper.
 *
 * Adds nothing to Buffer but its target. bindBase() and bindRange() are
 * inherited and bind as GL_UNIFORM_BUFFER because that is what the buffer was
 * created with, so the type exists to make a call site read as a UBO and to
 * stop one being constructed against the wrong target by accident.
 */
class UniformBuffer : public Buffer {
    public:
        UniformBuffer(const void* data, uint32_t size, GLenum usage = GL_STATIC_DRAW);
        ~UniformBuffer() override = default;

        UniformBuffer(const UniformBuffer& other) = delete;
        UniformBuffer& operator=(const UniformBuffer& other) = delete;

        UniformBuffer(UniformBuffer && other) noexcept = default;
        UniformBuffer& operator=(UniformBuffer && other) noexcept = default;
};

} // namespace Vkm::OpenGL
