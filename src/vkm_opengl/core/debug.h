#pragma once

#include <GL/glew.h>

namespace Vkm::OpenGL {
/**
 * @brief Enable OpenGL debug output with a default callback that logs messages.
 *
 * Call after load(). The driver reports asynchronously: a message may come after the call that
 * caused it, and from a thread of the driver's own.
 */
void enableGLDebugLogging();

} // namespace Vkm::OpenGL

