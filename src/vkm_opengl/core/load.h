#pragma once

#include <string>

namespace Vkm::OpenGL {

/**
 * @brief What load() found: whether the current context can run this library, and why not.
 */
struct Loaded {
    bool        ok = false;  ///< Every entry point loaded, on a context of the version this library needs.
    std::string reason;      ///< Why not, when not; empty when ok.
};

/**
 * @brief Load OpenGL's entry points for the context current on this thread, and check its version.
 *
 * Call once a context is current, before the first call into this library. The context may
 * come from a window or from EGL: glew's "no GLX display", which an EGL context gives, is no
 * failure, as every entry point still loads. An error loading leaves in GL's queue is
 * drained, so the caller's first check reads its own. The context must be at least
 * MAJOR_VERSION.MINOR_VERSION (version.h).
 *
 * @return Whether it loaded, and why not. Never throws.
 */
Loaded load();

} // namespace Vkm::OpenGL
