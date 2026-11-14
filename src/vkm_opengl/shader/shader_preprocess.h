#pragma once

#include <string>
#include <vector>

namespace Vkm::OpenGL {

/**
 * @brief Set the text prepended to every shader stage.
 *
 * The `#version` is GLSL_VERSION (version.h); the constants are the caller's, GLSL
 * declarations of values its own code holds (array capacities, grid dimensions), so no
 * shader file copies them.
 *
 * Applies to graphics and compute stages alike, since both go through
 * preprocessShaderSource. Call once at startup, before any shader is built.
 *
 * @param constants GLSL declarations inserted after the `#version` line.
 */
void setShaderPrelude(std::string constants);

/**
 * @brief Load a shader stage from disk with `#version` prepended, @p defines
 *        declared and `#include "rel.glsl"` directives resolved.
 *
 * GLSL has neither natively. Includes are inlined relative to the including
 * file's own directory, each file once per stage: a later include of it is skipped,
 * so a cycle ends. The directive itself is kept as a comment, so a compile error
 * inside an inlined body stays navigable.
 *
 * @param filePath Path to the shader stage file on disk.
 * @param defines  Names declared by `#define` after the prelude, in order: so
 *                 one stage serves each variant of a program.
 * @return The stage source, ready to compile.
 */
std::string preprocessShaderSource(const std::string& filePath, const std::vector<std::string>& defines = {});

} // namespace Vkm::OpenGL
