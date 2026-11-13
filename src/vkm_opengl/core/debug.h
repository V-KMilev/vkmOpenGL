#pragma once

#include <GL/glew.h>

namespace Vkm::OpenGL {
/**
 * @brief Enable OpenGL debug output with a default callback that logs messages.
 *
 * Call after load().
 *
 * @param synchronous If true, enables GL_DEBUG_OUTPUT_SYNCHRONOUS for immediate callbacks.
 */
void enableGLDebugLogging(bool synchronous = true);

} // namespace Vkm::OpenGL

