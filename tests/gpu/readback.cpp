// What a readback buffer copies off the GPU, read on a context with a default framebuffer.

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <GL/glew.h>

#include "vkm_opengl/buffer/readback_buffer.h"

#include "gpu/context.h"
#include "harness/check.h"
#include "harness/scratch.h"

using namespace Vkm::OpenGL;

namespace {

constexpr int      SIDE  = 4;
constexpr uint32_t BYTES = SIDE * SIDE * 4;

/// How many of the readback's RGBA8 pixels are @p red, @p green, @p blue, opaque.
int pixelsOf(const ReadbackBuffer& readback, uint8_t red, uint8_t green, uint8_t blue) {
    const auto* pixel = static_cast<const uint8_t*>(readback.data());
    int matching = 0;
    for (size_t i = 0; i < BYTES; i += 4) {
        const bool same = pixel[i] == red && pixel[i + 1] == green && pixel[i + 2] == blue;
        if (same && pixel[i + 3] == 255) ++matching;
    }
    return matching;
}

/// Clear the default framebuffer to an opaque color.
void clearDefault(float red, float green, float blue) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(red, green, blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace

TEST("captureDefault reads the default framebuffer, and puts back the read and pack buffers bound") {
    Vkm::Test::requireDefaultFramebuffer(SIDE, SIDE);
    clearDefault(1.0f, 0.0f, 1.0f);

    // Another framebuffer bound for reading and a buffer for packing, as a frame may leave them.
    GLuint framebuffer = 0;
    GLuint pack        = 0;
    glCreateFramebuffers(1, &framebuffer);
    glCreateBuffers(1, &pack);
    glNamedBufferData(pack, BYTES, nullptr, GL_STREAM_READ);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pack);

    ReadbackBuffer readback(BYTES);
    readback.captureDefault(0, 0, SIDE, SIDE, GL_RGBA, GL_UNSIGNED_BYTE);
    GLint reading = 0;
    GLint packing = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &reading);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packing);
    CHECK(static_cast<GLuint>(reading) == framebuffer);
    CHECK(static_cast<GLuint>(packing) == pack);

    glFinish();
    REQUIRE(readback.arrived());
    CHECK(pixelsOf(readback, 255, 0, 255) == SIDE * SIDE);
}

TEST("wait returns once the capture in flight has landed, and none is in flight after") {
    Vkm::Test::requireDefaultFramebuffer(SIDE, SIDE);
    // Work queued ahead of the capture, so it has not landed when wait() is called.
    GLuint target = 0;
    GLuint busy   = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &target);
    glTextureStorage2D(target, 1, GL_RGBA16F, 4096, 4096);
    glCreateFramebuffers(1, &busy);
    glNamedFramebufferTexture(busy, GL_COLOR_ATTACHMENT0, target, 0);
    const float gray[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    for (int i = 0; i < 50; ++i) glClearNamedFramebufferfv(busy, GL_COLOR, 0, gray);
    clearDefault(0.0f, 1.0f, 0.0f);

    ReadbackBuffer readback(BYTES);
    readback.captureDefault(0, 0, SIDE, SIDE, GL_RGBA, GL_UNSIGNED_BYTE);
    REQUIRE(readback.pending());
    readback.wait();
    CHECK(!readback.pending());
    CHECK(pixelsOf(readback, 0, 255, 0) == SIDE * SIDE);
    glDeleteFramebuffers(1, &busy);
    glDeleteTextures(1, &target);
}

TEST("a readback buffer moved from holds nothing") {
    Vkm::Test::requireContext();
    ReadbackBuffer from(BYTES);
    ReadbackBuffer to(std::move(from));
    CHECK(from.size() == 0u);
    CHECK(from.data() == nullptr);
    CHECK(to.size() == BYTES);

    ReadbackBuffer assigned(16);
    assigned = std::move(to);
    CHECK(to.size() == 0u);
    CHECK(assigned.size() == BYTES);
}

TEST("with the library's asserts on, a capture empty or past the buffer's size, rows padded, stops the program") {
    if (!VKM_TEST_LIBRARY_ASSERTS) Vkm::Test::skip("the library's asserts are off in this build");
    // A context of its own in each child, made after the fork: one made before is not the child's.
    const auto capture = [](uint32_t bytes, int32_t width, GLenum format, GLint align, int32_t height = 2) {
        return Vkm::Test::inChild([=] {
            Vkm::Test::requireDefaultFramebuffer(SIDE, SIDE);
            glPixelStorei(GL_PACK_ALIGNMENT, align);
            ReadbackBuffer readback(bytes);
            readback.captureDefault(0, 0, width, height, format, GL_UNSIGNED_BYTE);
        });
    };
    const Vkm::Test::Ended fits = capture(SIDE * 2 * 4, SIDE, GL_RGBA, 4);
    if (fits == Vkm::Test::Ended::Otherwise) Vkm::Test::missing("no context a child can make");
    REQUIRE(fits == Vkm::Test::Ended::Returned);
    CHECK(capture(SIDE * 2 * 4 - 1, SIDE, GL_RGBA, 4) == Vkm::Test::Ended::Stopped);

    // Three RGB pixels a row, rows eight-aligned: each row takes 16 bytes, the last too.
    CHECK(capture(32, 3, GL_RGB, 8) == Vkm::Test::Ended::Returned);
    CHECK(capture(31, 3, GL_RGB, 8) == Vkm::Test::Ended::Stopped);
    CHECK(capture(32, 0, GL_RGB, 8) == Vkm::Test::Ended::Stopped);
    CHECK(capture(32, 3, GL_RGB, 8, 0) == Vkm::Test::Ended::Stopped);
}

TEST("a wait or a check with no context current says it failed, and the fence is let go") {
    Vkm::Test::requireDefaultFramebuffer(SIDE, SIDE);
    ReadbackBuffer waited(BYTES);
    ReadbackBuffer checked(BYTES);
    waited.captureDefault(0, 0, SIDE, SIDE, GL_RGBA, GL_UNSIGNED_BYTE);
    checked.captureDefault(0, 0, SIDE, SIDE, GL_RGBA, GL_UNSIGNED_BYTE);
    Vkm::Test::dropContext();

    waited.wait();
    CHECK(!waited.pending());
    CHECK(!checked.arrived());
    CHECK(!checked.pending());
    int said = 0;
    for (size_t at = Vkm::Test::logText().find("may not have finished"); at != std::string::npos;) {
        ++said;
        at = Vkm::Test::logText().find("may not have finished", at + 1);
    }
    CHECK(said == 2);
}
