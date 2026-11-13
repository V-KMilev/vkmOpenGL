#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "vkm_opengl/shader/shader_base.h"

namespace Vkm::OpenGL {

/**
 * @brief Holds the source code for graphics shader stages (vertex, fragment, geometry).
 *
 * Can be constructed with a path to automatically load shader source files from disk.
 * Validates shader source file presence. Geometry shader is optional.
 */
struct GraphicsShaderSource {
    GraphicsShaderSource();
    ~GraphicsShaderSource() = default;

    /**
     * @brief Load shader source files from a directory, resolving `#include` and declaring @p defines.
     *
     * Reads vertex.shader, fragment.shader, and optionally geometry.shader,
     * running each through preprocessShaderSource so `#include "rel.glsl"`
     * directives are inlined (relative to the including file). A stage with no
     * #include is read verbatim but for the prelude and @p defines. For other
     * preprocessing construct the struct directly with prepared source strings.
     *
     * @param path    The directory the stages are in.
     * @param defines Declared at the top of every stage: which variant of the program this is.
     */
    explicit GraphicsShaderSource(const std::string& path, std::vector<std::string> defines = {});

    /**
     * @brief Direct construction with already-prepared source strings.
     *
     * Used when the caller has run its own preprocessing (e.g., resolved
     * `#include` directives) and wants to hand the final source straight
     * to compilation, bypassing the from-disk file reads.
     *
     * @param path Logical directory the source was loaded from. Used only
     *             for diagnostics and hot-reload reloadSource() lookups.
     * @param vertexShader   Final vertex shader source string.
     * @param fragmentShader Final fragment shader source string.
     * @param geometryShader Optional geometry shader source (empty = none).
     */
    GraphicsShaderSource(
        std::string path,
        std::string vertexShader,
        std::string fragmentShader,
        std::string geometryShader = {}
    );

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
 * Handles compilation, linkage, binding/unbinding, and uniform value setting for graphics shaders.
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

        /**
         * @brief Construct from already-prepared source strings.
         *
         * Skips the from-disk file read; the source is taken as-is. The
         * @p path is still recorded so reloadSource() and diagnostics work;
         * a derived class is expected to override reloadSource() if hot
         * reload should re-run the caller's preprocessor.
         */
        explicit Shader(GraphicsShaderSource source);

    protected:
        /**
         * @brief Creates the graphics shader program (vertex + fragment + optional geometry).
         */
        void createProgram() override;

        /**
         * @brief Reloads shader sources from disk.
         */
        void reloadSource() override;

    protected:
        GraphicsShaderSource m_source;  ///< Visible to subclasses so they can override reloadSource() and
                                        ///< install a freshly-prepared source (e.g. re-running an include
                                        ///< preprocessor) without going through the base path-based loader.
};

} // namespace Vkm::OpenGL
