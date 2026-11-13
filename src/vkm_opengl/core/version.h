#pragma once

namespace Vkm::OpenGL {

constexpr int MAJOR_VERSION = 4;  ///< With MINOR_VERSION, the OpenGL this library needs, core profile: the
                                  ///< oldest context it calls into, stated here alone. A consumer asks its
                                  ///< window or device for it, and the preprocessor's default `#version`
                                  ///< follows it.
constexpr int MINOR_VERSION = 5;  ///< See MAJOR_VERSION.

constexpr int GLSL_VERSION = MAJOR_VERSION * 100 + MINOR_VERSION * 10;  ///< In GLSL's form: 450 for 4.5.

} // namespace Vkm::OpenGL
