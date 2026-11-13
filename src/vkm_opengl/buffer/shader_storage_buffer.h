#pragma once

#include "vkm_opengl/buffer/buffer.h"

namespace Vkm::OpenGL {

/**
 * @brief Shader Storage Buffer Object (SSBO) wrapper.
 *
 * Adds nothing to Buffer but its target. bindBase() and bindRange() are
 * inherited and bind as GL_SHADER_STORAGE_BUFFER because that is what the
 * buffer was created with, so the type exists to make a call site read as an
 * SSBO and to stop one being constructed against the wrong target by accident.
 */
class ShaderStorageBuffer : public Buffer {
    public:
        ShaderStorageBuffer(
            const void* data,
            uint32_t size,
            GLenum usage = GL_DYNAMIC_DRAW
        );
        ~ShaderStorageBuffer() override = default;

        ShaderStorageBuffer(const ShaderStorageBuffer& other) = delete;
        ShaderStorageBuffer& operator=(const ShaderStorageBuffer& other) = delete;

        ShaderStorageBuffer(ShaderStorageBuffer && other) noexcept = default;
        ShaderStorageBuffer& operator=(ShaderStorageBuffer && other) noexcept = default;
};

} // namespace Vkm::OpenGL
