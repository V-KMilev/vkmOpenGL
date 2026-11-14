#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "vkm_opengl/shader/shader_base.h"

namespace Vkm::OpenGL {

/**
 * @brief Holds the source code for graphics shader stages (vertex, fragment, geometry).
 *
 * Constructed from a directory, whose stage files it loads. Validates shader source file presence.
 * Geometry shader is optional.
 */
struct GraphicsShaderSource {
    ~GraphicsShaderSource() = default;

    /**
     * @brief Load shader source files from a directory, resolving `#include` and declaring @p defines.
     *
     * Reads vertex.shader, fragment.shader, and optionally geometry.shader,
     * running each through preprocessShaderSource so `#include "rel.glsl"`
     * directives are inlined (relative to the including file). A stage with no
     * `#include` is read verbatim but for the prelude and @p defines.
     *
     * @param path    The directory the stages are in.
     * @param defines Declared at the top of every stage: which variant of the program this is.
     */
    explicit GraphicsShaderSource(const std::string& path, std::vector<std::string> defines = {});

    /**
     * @brief Check existence of the shader source files.
     *
     * If vertex/fragment is missing, returns false. Geometry is optional; if missing, returns true
     * and clears the path.
     *
     * @param vertexShaderName   Full path to the vertex shader file.
     * @param fragmentShaderName Full path to the fragment shader file.
     * @param geometryShaderName Full path to the geometry shader file (modified if file missing).
     * @return True if all required paths are valid, false otherwise.
     */
    bool validatePaths(
        const std::string& vertexShaderName,
        const std::string& fragmentShaderName,
        std::string& geometryShaderName
    );

    std::string              m_path;          ///< Path to the containing directory of shader files
    std::string              vertexShader;    ///< Source code for vertex shader
    std::string              fragmentShader;  ///< Source code for fragment shader
    std::string              geometryShader;  ///< Source code for geometry shader (optional)
    std::vector<std::string> defines;         ///< Declared in every stage; kept so a reload declares them
                                              ///< again
};

/**
 * @brief OpenGL graphics shader program wrapper (vertex + fragment + optional geometry).
 *
 * Handles compilation, linkage, binding, and uniform value setting for graphics shaders.
 * Automatically loads shaders from directory containing vertex.shader, fragment.shader,
 * and optionally geometry.shader.
 *
 * Non-copyable, movable (inherits move semantics from ShaderBase).
 */
class Shader : public ShaderBase {
    public:
        Shader() = delete;
        ~Shader() override = default;

        Shader(const Shader& other) = delete;
        Shader& operator=(const Shader& other) = delete;

        Shader(Shader && other) noexcept = default;
        Shader& operator=(Shader && other) noexcept = default;

        /**
         * @brief Construct and load graphics shaders from the directory at path, declaring @p defines.
         * @param path Path to the directory containing vertex.shader, fragment.shader, and optionally
         *        geometry.shader.
         * @param defines Declared in every stage after the prelude, so one directory makes each variant of a
         *                program; the program's label names them after the directory's.
         * @throws std::runtime_error if shader source validation or compilation fails.
         */
        explicit Shader(const std::string& path, std::vector<std::string> defines = {});

    protected:
        /**
         * @brief Creates the graphics shader program (vertex + fragment + optional geometry).
         */
        void createProgram() override;

        /**
         * @brief Reloads shader sources from disk.
         */
        void reloadSource() override;

    private:
        GraphicsShaderSource m_source;  ///< The stages as last read, and the defines a reload declares.
};

} // namespace Vkm::OpenGL
