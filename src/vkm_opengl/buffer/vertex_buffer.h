#pragma once

#include "vkm_opengl/buffer/buffer.h"

namespace Vkm::OpenGL {

/**
 * @brief Vertex buffer (VBO) wrapping an OpenGL array buffer.
 *
 * Owns a GPU vertex buffer; uploads data on construction and disallows copy/move
 * to keep ownership simple.
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
