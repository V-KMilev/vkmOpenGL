#define VKM_LOG_CATEGORY "DRIVER"

#include "vkm_opengl/core/debug.h"

#include <string>

#include "vkm_log/logger.h"

#include "vkm_opengl/core/error_handle.h"

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

namespace Vkm::OpenGL {
namespace {
    void GLAPIENTRY glDebugCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
    ) {
        (void)length;
        (void)userParam;

        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            return;
        }

        const char* srcStr = "OTHER";
        switch (source) {
            case GL_DEBUG_SOURCE_API:             srcStr = "API"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   srcStr = "WINDOW_SYSTEM"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: srcStr = "SHADER_COMPILER"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     srcStr = "THIRD_PARTY"; break;
            case GL_DEBUG_SOURCE_APPLICATION:     srcStr = "APPLICATION"; break;
            case GL_DEBUG_SOURCE_OTHER:           srcStr = "OTHER"; break;
        }

        const char* typeStr = "OTHER";
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               typeStr = "ERROR"; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "DEPRECATED_BEHAVIOR"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "UNDEFINED_BEHAVIOR"; break;
            case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "PORTABILITY"; break;
            case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "PERFORMANCE"; break;
            case GL_DEBUG_TYPE_MARKER:              typeStr = "MARKER"; break;
            case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "PUSH_GROUP"; break;
            case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "POP_GROUP"; break;
            case GL_DEBUG_TYPE_OTHER:               typeStr = "OTHER"; break;
        }

        // A performance note is mostly the driver describing its own work - a program recompiled for
        // the state it is drawn in, say - rarely something to act on: logged for whoever profiles.
        if (type == GL_DEBUG_TYPE_PERFORMANCE && severity != GL_DEBUG_SEVERITY_HIGH) {
            LOG_DEBUG("[%s/%s][ID:%u] %s", srcStr, typeStr, id, message);
            return;
        }

        // Severity maps to log level so HIGH GL errors log as ERROR and MEDIUM as WARNING.
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                LOG_ERROR("[%s/%s/HIGH][ID:%u] %s", srcStr, typeStr, id, message);
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                LOG_WARNING("[%s/%s/MED][ID:%u] %s", srcStr, typeStr, id, message);
                break;
            case GL_DEBUG_SEVERITY_LOW:
                LOG_INFO("[%s/%s/LOW][ID:%u] %s", srcStr, typeStr, id, message);
                break;
            default:
                LOG_VERBOSE("[%s/%s][ID:%u] %s", srcStr, typeStr, id, message);
                break;
        }
    }

} // namespace

void enableGLDebugLogging() {
    VKM_OPENGL_CHECK(glEnable(GL_DEBUG_OUTPUT));
    VKM_OPENGL_CHECK(glDebugMessageCallback(glDebugCallback, nullptr));
    LOG_INFO("GL debug output enabled");
}

} // namespace Vkm::OpenGL

