#include "vkm_opengl/core/load.h"

#include <GL/glew.h>

#include "vkm_opengl/core/version.h"

namespace Vkm::OpenGL {

Loaded load() {
    // Not glewExperimental: it marks an extension present whenever its entry points resolve, and a
    // loader resolves a name the driver never offered, so a flag the library reads could claim
    // what the driver lacks. glew reads a core context's extensions without it.
    const GLenum status = glewInit();
    if (status != GLEW_OK && status != GLEW_ERROR_NO_GLX_DISPLAY) {
        return {false, std::string("glew could not load OpenGL: ")
            + reinterpret_cast<const char*>(glewGetErrorString(status))};
    }

    // A glew that asks a core context for what it no longer has leaves GL_INVALID_ENUM queued.
    // Bounded, as a lost context answers every read with an error.
    constexpr int MAX_DRAIN = 32;
    for (int i = 0; i < MAX_DRAIN && glGetError() != GL_NO_ERROR; ++i) {}

    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major < MAJOR_VERSION || (major == MAJOR_VERSION && minor < MINOR_VERSION)) {
        const GLubyte* version = glGetString(GL_VERSION);
        return {false, "the context is OpenGL "
            + std::string(version ? reinterpret_cast<const char*>(version) : "of no version")
            + ", older than the " + std::to_string(MAJOR_VERSION) + "." + std::to_string(MINOR_VERSION)
            + " core this library needs"};
    }
    return {true, {}};
}

} // namespace Vkm::OpenGL
