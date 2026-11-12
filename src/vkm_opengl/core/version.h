#pragma once

namespace Vkm::OpenGL {

/**
 * @brief The OpenGL this library needs, core profile: the oldest context it calls into.
 *
 * Stated once, here. A consumer asks its window or device for this version, and the shader
 * preprocessor's default `#version` follows it.
 */
constexpr int MAJOR_VERSION = 4;
constexpr int MINOR_VERSION = 5;  ///< See MAJOR_VERSION.

/// The GLSL version that goes with it, in GLSL's integer form: 450 for 4.5.
constexpr int GLSL_VERSION = MAJOR_VERSION * 100 + MINOR_VERSION * 10;

} // namespace Vkm::OpenGL
