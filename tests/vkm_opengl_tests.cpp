// The vkm_opengl test suite. Deliberately covers only what runs without a GL context:
// vertex layout arithmetic, the shader preprocessor, the error check's shape, a load with
// no context, the cube's faces and the indirect draw's record. Anything needing a live
// context belongs in a rendering test, not here - this binary must stay runnable on a
// build machine with no GPU.

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include <glm/glm.hpp>

#include "vkm_log/logger.h"

#include "vkm_opengl/core/error_handle.h"
#include "vkm_opengl/core/load.h"
#include "vkm_opengl/core/version.h"
#include "vkm_opengl/shader/shader_preprocess.h"
#include "vkm_opengl/texture/cube_faces.h"
#include "vkm_opengl/vertex/vertex_array.h"
#include "vkm_opengl/vertex/vertex_buffer_layout.h"

namespace fs = std::filesystem;

namespace {

int g_failures = 0;

void check(const char* what, bool ok) {
    if (!ok) ++g_failures;
    std::printf("  %-62s %s\n", what, ok ? "ok" : "<-- FAILED");
}

void write(const fs::path& p, const std::string& text) {
    std::ofstream out(p);
    out << text;
}

void testVertexBufferLayout() {
    std::printf("VertexBufferLayout:\n");

    Vkm::OpenGL::VertexBufferLayout layout;
    check("a fresh layout has zero stride", layout.getStride() == 0);
    check("a fresh layout has no elements", layout.getElements().empty());

    layout.push<float>(3);
    check("vec3 of float is 12 bytes of stride", layout.getStride() == 12);

    layout.push<float>(2);
    check("stride accumulates across pushes", layout.getStride() == 20);

    layout.push<uint8_t>(4);
    check("byte attributes add one byte each", layout.getStride() == 24);
    check("three elements recorded", layout.getElements().size() == 3);

    layout.push<uint16_t>(4);
    check("short attributes add two bytes each", layout.getStride() == 32);

    // uint8_t is the one type pushed as normalized - 0..255 reads as 0..1.
    check("uint8 pushes normalized", layout.getElements()[2].normalized == GL_TRUE);
    check("uint16 pushes unnormalized", layout.getElements()[3].normalized == GL_FALSE);
    check("float pushes unnormalized", layout.getElements()[0].normalized == GL_FALSE);

    layout.clear();
    check("clear() resets the stride", layout.getStride() == 0);
    check("clear() drops the elements", layout.getElements().empty());

    // A layout describes vertex data and owns no GL handle, so it has to be
    // copyable and movable - it was deleted on both until 1.2.
    Vkm::OpenGL::VertexBufferLayout source;
    source.push<float>(3);
    Vkm::OpenGL::VertexBufferLayout copied = source;
    check("a layout can be copied", copied.getStride() == 12 && copied.getElements().size() == 1);
    check("the copy is independent", (copied.push<float>(1), source.getStride() == 12));

    Vkm::OpenGL::VertexBufferLayout moved = std::move(copied);
    check("a layout can be moved", moved.getStride() == 16);

    std::printf("VertexBufferElement::getSizeOfType:\n");
    using E = Vkm::OpenGL::VertexBufferElement;
    check("GL_FLOAT is 4",          E::getSizeOfType(GL_FLOAT) == 4);
    check("GL_UNSIGNED_INT is 4",   E::getSizeOfType(GL_UNSIGNED_INT) == 4);
    check("GL_UNSIGNED_BYTE is 1",  E::getSizeOfType(GL_UNSIGNED_BYTE) == 1);
    check("GL_SHORT is 2",          E::getSizeOfType(GL_SHORT) == 2);
    check("GL_DOUBLE is 8",         E::getSizeOfType(GL_DOUBLE) == 8);
}

void testShaderPreprocessor(const fs::path& dir) {
    std::printf("Shader preprocessor:\n");

    write(dir / "plain.glsl", "void main() {}\n");
    const std::string version = "#version " + std::to_string(Vkm::OpenGL::GLSL_VERSION) + " core";
    check("GLSL_VERSION is the GLSL of MAJOR_VERSION.MINOR_VERSION",
          Vkm::OpenGL::GLSL_VERSION == Vkm::OpenGL::MAJOR_VERSION * 100 + Vkm::OpenGL::MINOR_VERSION * 10);
    check("with no prelude set, the #version is GLSL_VERSION",
          Vkm::OpenGL::preprocessShaderSource((dir / "plain.glsl").string()).rfind(version, 0) == 0);

    Vkm::OpenGL::setShaderPrelude(430);
    const std::string plain = Vkm::OpenGL::preprocessShaderSource((dir / "plain.glsl").string());
    check("the #version is prepended", plain.rfind("#version 430 core", 0) == 0);
    check("a stage with no include is otherwise verbatim",
          plain.find("void main() {}") != std::string::npos);

    Vkm::OpenGL::setShaderPrelude(460);
    const std::string bumped = Vkm::OpenGL::preprocessShaderSource((dir / "plain.glsl").string());
    check("setShaderPrelude changes what is injected",
          bumped.rfind("#version 460 core", 0) == 0);

    Vkm::OpenGL::setShaderPrelude(430, "const int ANSWER = 42;\n");
    const std::string withConstants =
        Vkm::OpenGL::preprocessShaderSource((dir / "plain.glsl").string());
    check("the constants land after the #version and before the source",
          withConstants.find("const int ANSWER = 42;") <
          withConstants.find("void main() {}"));
    Vkm::OpenGL::setShaderPrelude(430);
    check("and a prelude with no constants adds none",
          Vkm::OpenGL::preprocessShaderSource((dir / "plain.glsl").string())
              .find("ANSWER") == std::string::npos);

    write(dir / "helper.glsl", "float helper() { return 1.0; }\n");
    write(dir / "uses.glsl",   "#include \"helper.glsl\"\nvoid main() {}\n");
    const std::string included = Vkm::OpenGL::preprocessShaderSource((dir / "uses.glsl").string());
    check("an included body is inlined",
          included.find("float helper()") != std::string::npos);
    check("the directive survives as a comment",
          included.find("// #include \"helper.glsl\"") != std::string::npos);

    // Includes resolve relative to the including file, not the process cwd.
    fs::create_directories(dir / "sub");
    write(dir / "sub" / "leaf.glsl",  "int leaf;\n");
    write(dir / "sub" / "inner.glsl", "#include \"leaf.glsl\"\n");
    write(dir / "outer.glsl",         "#include \"sub/inner.glsl\"\n");
    const std::string nested = Vkm::OpenGL::preprocessShaderSource((dir / "outer.glsl").string());
    check("includes nest, each relative to its own directory",
          nested.find("int leaf;") != std::string::npos);

    // The cycle guard: a file including itself must terminate, not recurse.
    write(dir / "cycle.glsl", "#include \"cycle.glsl\"\nint after;\n");
    const std::string cycle = Vkm::OpenGL::preprocessShaderSource((dir / "cycle.glsl").string());
    check("a self-include is skipped rather than looping",
          cycle.find("skipped duplicate include") != std::string::npos);
    check("parsing continues past the cycle",
          cycle.find("int after;") != std::string::npos);

    // A directive inside a comment is not a directive.
    write(dir / "commented.glsl", "// #include \"helper.glsl\"\nint only;\n");
    const std::string commented = Vkm::OpenGL::preprocessShaderSource((dir / "commented.glsl").string());
    check("a commented-out include is not resolved",
          commented.find("float helper()") == std::string::npos);

    const std::string missing = Vkm::OpenGL::preprocessShaderSource((dir / "nope.glsl").string());
    check("a missing file yields a marker instead of throwing",
          missing.find("failed to open") != std::string::npos);
}

void testErrorCheck() {
    std::printf("VKM_OPENGL_CHECK:\n");

    // Whether VKM_OPENGL_CHECKS is on or off: the call is the macro's, and runs once.
    int calls = 0;
    VKM_OPENGL_CHECK(++calls);
    check("runs its call exactly once", calls == 1);
    if (calls == 0) VKM_OPENGL_CHECK(++calls);
    check("binds to the branch it is written under", calls == 1);
}

void testLoad() {
    std::printf("load():\n");

    // No context is current here: refused, with the reason, and no throw.
    const Vkm::OpenGL::Loaded loaded = Vkm::OpenGL::load();
    check("with no current context, it answers no", !loaded.ok);
    check("and says why", !loaded.reason.empty());
}

bool alike(const glm::vec3& a, const glm::vec3& b) {
    return glm::all(glm::lessThan(glm::abs(a - b), glm::vec3(1e-6f)));
}

void testCubeFaces() {
    std::printf("Cube faces:\n");

    using Vkm::OpenGL::CUBE_FACES;
    // GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, and the specification's face-selection table: on face
    // i, t grows with -ry for the X and Z faces, with +rz on +Y and -rz on -Y.
    const glm::vec3 dirs[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const glm::vec3 ups[6]  = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};
    bool order = true;
    bool basis = true;
    bool views = true;
    for (int i = 0; i < 6; ++i) {
        order = order && CUBE_FACES[i].dir == dirs[i] && CUBE_FACES[i].up == ups[i];
        basis = basis && glm::dot(CUBE_FACES[i].dir, CUBE_FACES[i].up) == 0.0f
            && glm::length(CUBE_FACES[i].dir) == 1.0f && glm::length(CUBE_FACES[i].up) == 1.0f;
        // Seen from an eye off the origin, the face's center lies straight ahead and its up is up.
        const glm::vec3 eye(2.0f, -3.0f, 5.0f);
        const glm::mat4 view = Vkm::OpenGL::cubeFaceView(i, eye);
        views = views && alike(glm::vec3(view * glm::vec4(eye + CUBE_FACES[i].dir, 1.0f)), {0, 0, -1})
            && alike(glm::vec3(view * glm::vec4(CUBE_FACES[i].up, 0.0f)), {0, 1, 0});
    }
    check("six faces in GL's order, each up as the specification's table", order);
    check("each direction and up a unit axis, at right angles", basis);
    check("cubeFaceView looks down the face from the eye, its up up", views);
}

void testIndirectCommand() {
    std::printf("DrawElementsIndirectCommand:\n");

    using C = Vkm::OpenGL::DrawElementsIndirectCommand;
    check("is 20 bytes, five packed words", sizeof(C) == 20);
    check("in GL's order: count, instances, first index, base vertex, base instance",
          offsetof(C, count) == 0 && offsetof(C, instanceCount) == 4 && offsetof(C, firstIndex) == 8
              && offsetof(C, baseVertex) == 12 && offsetof(C, baseInstance) == 16);
}

} // namespace

int main() {
    // The preprocessor logs a warning for the deliberately-missing file below,
    // so the logger has to exist. ERROR level keeps the expected noise out of
    // the test output.
    const fs::path logPath = fs::temp_directory_path() / "vkm_opengl_tests.log";
    Vkm::Log::Logger::init(logPath.string(), "VKM_OPENGL-TESTS", Vkm::Log::Level::Error);

    const fs::path dir = fs::temp_directory_path() / "vkm_opengl_tests";
    fs::remove_all(dir);
    fs::create_directories(dir);

    testVertexBufferLayout();
    testShaderPreprocessor(dir);
    testErrorCheck();
    testLoad();
    testCubeFaces();
    testIndirectCommand();

    fs::remove_all(dir);

    std::printf(g_failures ? "\n%d FAILURE(S)\n" : "\nALL OK\n", g_failures);
    return g_failures ? 1 : 0;
}
