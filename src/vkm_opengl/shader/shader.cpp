#define VKM_LOG_CATEGORY "SHADER"

#include "vkm_opengl/shader/shader.h"

#include <filesystem>
#include <stdexcept>
#include <vector>

#include "vkm_log/assert.h"
#include "vkm_log/logger.h"

#include "vkm_opengl/core/error_handle.h"
#include "vkm_opengl/shader/shader_preprocess.h"

namespace fs = std::filesystem;

namespace Vkm::OpenGL {

GraphicsShaderSource::GraphicsShaderSource(const std::string& path, std::vector<std::string> defines_)
    : m_path(path)
    , vertexShader()
    , fragmentShader()
    , geometryShader()
    , defines(std::move(defines_)) {
    const std::string vertexShaderName   = (fs::path(m_path) / "vertex.shader").string();
    const std::string fragmentShaderName = (fs::path(m_path) / "fragment.shader").string();
    std::string geometryShaderName = (fs::path(m_path) / "geometry.shader").string();

    if (!validatePaths(vertexShaderName, fragmentShaderName, geometryShaderName)) {
        throw std::runtime_error("Graphics shader source validation failed");
    }

    // Resolve #include directives as each stage is loaded.
    vertexShader   = preprocessShaderSource(vertexShaderName, defines);
    fragmentShader = preprocessShaderSource(fragmentShaderName, defines);

    if (!geometryShaderName.empty()) {
        geometryShader = preprocessShaderSource(geometryShaderName, defines);
    }
}

bool GraphicsShaderSource::validatePaths(
    const std::string& vertexShaderName,
    const std::string& fragmentShaderName,
    std::string& geometryShaderName
) {
    if (!fs::exists(vertexShaderName)) {
        LOG_FATAL(
            "Graphics shader '%s' missing vertex shader at '%s'",
            m_path.c_str(),
            vertexShaderName.c_str()
        );
        return false;
    }
    if (!fs::exists(fragmentShaderName)) {
        LOG_FATAL(
            "Graphics shader '%s' missing fragment shader at '%s'",
            m_path.c_str(),
            fragmentShaderName.c_str()
        );
        return false;
    }
    if (!fs::exists(geometryShaderName)) {
        // Normal operation for nearly every program - not a warning.
        LOG_TRACE("Graphics shader '%s' has no geometry stage", m_path.c_str());
        geometryShaderName.clear();
    }
    return true;
}

Shader::Shader(const std::string& path, std::vector<std::string> defines)
    : ShaderBase(path)
    , m_source(path, std::move(defines)) {
    // One directory makes each variant, so a label names the defines too.
    for (const std::string& name : m_source.defines) m_name += "+" + name;
    createProgram();
}

void Shader::reloadSource() {
    // Re-read from disk, as the same variant.
    m_source = GraphicsShaderSource(m_path, m_source.defines);
}

void Shader::createProgram() {
    VKM_OPENGL_CHECK(m_id = glCreateProgram());
    VKM_ASSERT(m_id != 0);

    // Apply a debug label for easier GPU debugging.
    setLabel(m_name.c_str());

    std::vector<uint32_t> stages;
    stages.push_back(startStage(GL_VERTEX_SHADER, m_source.vertexShader));
    stages.push_back(startStage(GL_FRAGMENT_SHADER, m_source.fragmentShader));
    if (!m_source.geometryShader.empty()) {
        stages.push_back(startStage(GL_GEOMETRY_SHADER, m_source.geometryShader));
    }
    startLink(std::move(stages));
}

} // namespace Vkm::OpenGL
