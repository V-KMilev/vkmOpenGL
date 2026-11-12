#include "vkm_opengl/buffer/shader_storage_buffer.h"

namespace Vkm::OpenGL {

ShaderStorageBuffer::ShaderStorageBuffer(const void* data, uint32_t size, GLenum usage)
    : Buffer(GL_SHADER_STORAGE_BUFFER, data, size, usage) {}

} // namespace Vkm::OpenGL
