#include "vkm_opengl/buffer/uniform_buffer.h"

namespace Vkm::OpenGL {

UniformBuffer::UniformBuffer(const void* data, uint32_t size, GLenum usage)
    : Buffer(GL_UNIFORM_BUFFER, data, size, usage) {}

} // namespace Vkm::OpenGL
