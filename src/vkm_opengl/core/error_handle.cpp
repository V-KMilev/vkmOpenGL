#define VKM_LOG_CATEGORY "DRIVER"

#include "vkm_opengl/core/error_handle.h"

#include "vkm_log/assert.h"
#include "vkm_log/logger.h"

void checkGLError(
    const char* file,
    int line,
    const char* function
) {
    // GL holds a queue, not a single flag: reading one error clears only that
    // entry. Draining it here keeps an error attributed to the call that
    // actually produced it instead of the next checked call downstream.
    // Bounded because a lost context keeps returning GL_CONTEXT_LOST for every
    // query, which would spin here forever.
    constexpr int MAX_DRAIN = 32;

    bool failed = false;
    for (int i = 0; i < MAX_DRAIN; ++i) {
        const GLenum error = glGetError();
        if (error == GL_NO_ERROR) break;
        LOG_ERROR("VKM_OPENGL_CHECK failed at %s:%d in %s: GL error 0x%X", file, line, function, error);
        failed = true;
    }

    if (failed) Vkm::Log::stopProgram();
}
