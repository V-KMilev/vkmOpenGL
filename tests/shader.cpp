// The shader preprocessor: the prelude, a variant's defines and includes, with no GL context.

#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>

#include "vkm_log/logger.h"

#include "vkm_opengl/core/version.h"
#include "vkm_opengl/shader/shader_preprocess.h"
#include "vkm_opengl/shader/shader_reload.h"

#include "harness/check.h"
#include "harness/scratch.h"

namespace {

namespace fs = std::filesystem;

using Vkm::OpenGL::preprocessShaderSource;
using Vkm::OpenGL::setShaderPrelude;
using Vkm::Test::scratch;
using Vkm::Test::writeText;

std::string sourceOf(const std::string& name) { return preprocessShaderSource((scratch() / name).string()); }

bool has(const std::string& text, const std::string& what) { return text.find(what) != std::string::npos; }

} // namespace

TEST("the prelude's #version is GLSL_VERSION, the GLSL of the OpenGL the library needs, until it is set") {
    CHECK(Vkm::OpenGL::GLSL_VERSION == Vkm::OpenGL::MAJOR_VERSION * 100 + Vkm::OpenGL::MINOR_VERSION * 10);
    writeText(scratch() / "plain.glsl", "void main() {}\n");
    const std::string version = "#version " + std::to_string(Vkm::OpenGL::GLSL_VERSION) + " core\n";
    CHECK(sourceOf("plain.glsl").rfind(version, 0) == 0u);
}

TEST("a stage is its #version, then the prelude's constants, then the source as written") {
    writeText(scratch() / "plain.glsl", "void main() {}\n");
    setShaderPrelude(430);
    CHECK(sourceOf("plain.glsl") == "#version 430 core\nvoid main() {}\n");

    setShaderPrelude(460, "const int ANSWER = 42;\n");
    CHECK(sourceOf("plain.glsl") == "#version 460 core\nconst int ANSWER = 42;\nvoid main() {}\n");

    setShaderPrelude(430);
    CHECK(!has(sourceOf("plain.glsl"), "ANSWER"));
}

TEST("a variant's defines are declared after the prelude, in order, before the source") {
    writeText(scratch() / "plain.glsl", "void main() {}\n");
    setShaderPrelude(450, "const int N = 1;\n");
    const std::string path    = (scratch() / "plain.glsl").string();
    const std::string prelude = "#version 450 core\nconst int N = 1;\n";
    const std::string defines = "#define SHADOWS\n#define SKINNED\n";
    CHECK(preprocessShaderSource(path, {"SHADOWS", "SKINNED"}) == prelude + defines + "void main() {}\n");
}

TEST("an include is inlined where it stands, relative to its own file, the directive kept as a comment") {
    writeText(scratch() / "helper.glsl", "float helper() { return 1.0; }\n");
    writeText(scratch() / "uses.glsl", "#include \"helper.glsl\"\nvoid main() {}\n");
    const std::string uses = sourceOf("uses.glsl");
    CHECK(has(uses, "// #include \"helper.glsl\"\nfloat helper() { return 1.0; }\nvoid main() {}\n"));

    // Nested, each resolved from the folder of the file that names it, never the process's.
    writeText(scratch() / "sub" / "leaf.glsl", "int leaf;\n");
    writeText(scratch() / "sub" / "inner.glsl", "#include \"leaf.glsl\"\n");
    writeText(scratch() / "outer.glsl", "#include \"sub/inner.glsl\"\n");
    CHECK(has(sourceOf("outer.glsl"), "int leaf;"));

    // A directive in a comment is not one.
    writeText(scratch() / "commented.glsl", "// #include \"helper.glsl\"\nint only;\n");
    CHECK(!has(sourceOf("commented.glsl"), "float helper()"));
}

TEST("an include of a file already included is skipped, so a cycle ends and the rest is read") {
    writeText(scratch() / "cycle.glsl", "#include \"cycle.glsl\"\nint after;\n");
    const std::string cycle = sourceOf("cycle.glsl");
    CHECK(has(cycle, "skipped duplicate include"));
    CHECK(has(cycle, "int after;"));

    writeText(scratch() / "a.glsl", "#include \"b.glsl\"\nint a;\n");
    writeText(scratch() / "b.glsl", "#include \"a.glsl\"\nint b;\n");
    const std::string pair = sourceOf("a.glsl");
    CHECK(has(pair, "int b;\nint a;\n"));
}

TEST("a missing file gives a marker in the source and an error, never a throw") {
    const std::string missing = sourceOf("nope.glsl");
    CHECK(has(missing, "failed to open"));
    CHECK(has(Vkm::Test::logText(), "cannot open"));
}

TEST("every line the library logs carries VKM-OPENGL and its category, the reload's too") {
    writeText(scratch() / "shaders" / "bad.glsl", "#include helper.glsl\n");
    preprocessShaderSource((scratch() / "nope.glsl").string());
    preprocessShaderSource((scratch() / "shaders" / "bad.glsl").string());

    // The first poll only notes where it starts; a newer write is a change, with no shader live to rebuild.
    const std::string shaders = (scratch() / "shaders").string();
    CHECK(Vkm::OpenGL::reloadChangedShaders(shaders) == 0u);
    writeText(scratch() / "shaders" / "new.glsl", "int x;\n");
    const fs::file_time_type later = fs::file_time_type::clock::now() + std::chrono::hours(1);
    fs::last_write_time(scratch() / "shaders" / "new.glsl", later);
    CHECK(Vkm::OpenGL::reloadChangedShaders(shaders) == 0u);

    std::istringstream log(Vkm::Test::logText());
    int lines   = 0;
    bool reload = false;
    for (std::string text; std::getline(log, text);) {
        const auto line = Vkm::Log::parseLine(text);
        REQUIRE(line.has_value());
        ++lines;
        CHECK(line->suffix == "VKM-OPENGL");
        CHECK(line->category == "SHADER");
        if (line->message.find("Reloaded 0 shader") != std::string_view::npos) reload = true;
    }
    CHECK(lines == 3);
    CHECK(reload);
}
