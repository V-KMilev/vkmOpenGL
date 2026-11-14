#define VKM_LOG_CATEGORY "SHADER"

#include "vkm_opengl/shader/shader_preprocess.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <unordered_set>

#include "vkm_log/logger.h"

#include "vkm_opengl/core/version.h"

namespace fs = std::filesystem;

namespace Vkm::OpenGL {

namespace {

// The application's own constants, inserted after the #version line. Empty
// until something sets them, which is what makes this module usable without.
// Behind an accessor: a function-local static initializes on first use, with no
// order against other translation units.
std::string& preludeConstants() {
    static std::string s_constants;
    return s_constants;
}

// Resolve `#include "relative/path"` directives, inlining referenced files
// relative to each including file's directory. Cycle-safe via `visited`; the
// directive is preserved as a comment so compile errors in the inlined body
// stay navigable.
std::string resolveIncludes(const fs::path& filePath, std::unordered_set<std::string>& visited) {
    std::error_code ec;
    const fs::path canonical = fs::weakly_canonical(filePath, ec);
    const std::string key    = ec ? filePath.string() : canonical.string();
    if (!visited.insert(key).second) {
        return "// (skipped duplicate include: " + filePath.string() + ")\n";
    }

    std::ifstream in(filePath);
    if (!in) {
        LOG_ERROR("Shader preprocessor: cannot open '%s'", filePath.string().c_str());
        return "// (failed to open: " + filePath.string() + ")\n";
    }

    const fs::path basedir = filePath.parent_path();
    std::ostringstream out;
    std::string line;
    while (std::getline(in, line)) {
        // Only a leading `#include` token is a directive (not one in a comment).
        size_t firstNonWS = 0;
        while (firstNonWS < line.size() && (line[firstNonWS] == ' ' || line[firstNonWS] == '\t')) {
            ++firstNonWS;
        }
        if (line.compare(firstNonWS, 8, "#include") == 0) {
            const size_t q1 = line.find('"', firstNonWS + 8);
            const size_t q2 = q1 != std::string::npos ? line.find('"', q1 + 1) : std::string::npos;
            if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1) {
                out << "// " << line << "\n";
                out << resolveIncludes(basedir / line.substr(q1 + 1, q2 - q1 - 1), visited);
                continue;
            }
            LOG_WARNING(
                "Shader preprocessor: malformed #include in '%s': %s",
                filePath.string().c_str(),
                line.c_str()
            );
        }
        out << line << "\n";
    }
    return out.str();
}

} // namespace

void setShaderPrelude(std::string constants) {
    preludeConstants() = std::move(constants);
}

std::string preprocessShaderSource(const std::string& filePath, const std::vector<std::string>& defines) {
    std::unordered_set<std::string> visited;
    std::string variant;
    for (const std::string& name : defines) variant += "#define " + name + "\n";
    // #version must be the first line, and #included files never carry one.
    return "#version " + std::to_string(GLSL_VERSION) + " core\n"
         + preludeConstants()
         + variant
         + resolveIncludes(fs::path(filePath), visited);
}

} // namespace Vkm::OpenGL
