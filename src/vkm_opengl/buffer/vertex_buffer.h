#pragma once

#include "vkm_opengl/buffer/buffer.h"

namespace Vkm::OpenGL {

/**
 * @brief Vertex buffer (VBO) wrapping an OpenGL array buffer.
 *
 * Owns a GPU vertex buffer, filled at construction; it moves, and never copies.
 */
class VertexBuffer : public Buffer {
    public:
        VertexBuffer(
            const void* data,
            uint32_t size,
            GLenum usage = GL_STATIC_DRAW
        );
};

} // namespace Vkm::OpenGL
