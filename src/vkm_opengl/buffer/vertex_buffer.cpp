#include "vkm_opengl/buffer/vertex_buffer.h"

namespace Vkm::OpenGL {

VertexBuffer::VertexBuffer(
    const void* data,
    uint32_t size,
    GLenum usage
) : Buffer(GL_ARRAY_BUFFER, data, size, usage) {}

} // namespace Vkm::OpenGL
