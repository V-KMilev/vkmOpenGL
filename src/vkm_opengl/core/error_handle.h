#pragma once

#include <GL/glew.h>

namespace Vkm::OpenGL {

/**
 * @brief Drain the OpenGL error queue, logging each error, and stop the program if there was any.
 *
 * GL queues errors rather than holding one, so this loops until the queue is
 * empty and reports every entry - stopping at the first would leave the rest to
 * surface at some unrelated later check.
 *
 * @param file     Source file the check is performed from.
 * @param line     Line number in that file.
 * @param function Name of the enclosing function.
 */
void checkGLError(
    const char* file,
    int line,
    const char* function
);

} // namespace Vkm::OpenGL

/**
 * @brief Run an OpenGL call and check the error queue immediately after it.
 *
 * A statement, not an expression: the body is wrapped so that
 * `if (cond) VKM_OPENGL_CHECK(glFoo());` binds the check to the branch instead of
 * running it unconditionally. A call whose result is needed assigns inside the
 * macro rather than around it:
 *
 * @code
 * uint32_t id = 0;
 * VKM_OPENGL_CHECK(id = glCreateShader(type));
 * @endcode
 *
 * Built without VKM_OPENGL_CHECKS, the check is dropped but the statement keeps
 * its shape, so control flow cannot differ between configurations. The check
 * follows that flag alone, never NDEBUG: each one is a round trip to a driver that
 * may run on a thread of its own.
 *
 * @param GL_CALL The OpenGL call to execute and check.
 */
#ifdef VKM_OPENGL_CHECKS
    #define VKM_OPENGL_CHECK(GL_CALL)                                      \
        do {                                                               \
            GL_CALL;                                                       \
            ::Vkm::OpenGL::checkGLError(__FILE__, __LINE__, __FUNCTION__); \
        } while (0)
#else
    #define VKM_OPENGL_CHECK(GL_CALL) \
        do {                          \
            GL_CALL;                  \
        } while (0)
#endif
