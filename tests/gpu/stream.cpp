// The frame's stream on a real context: where its spans land, and when a buffer it outgrew goes.

#include <cstdint>
#include <vector>

#include <GL/glew.h>

#include "vkm_opengl/buffer/stream_ring.h"

#include "gpu/context.h"
#include "harness/check.h"

using namespace Vkm::OpenGL;

namespace {

/// @p count bytes counting up from @p first.
std::vector<uint8_t> bytesFrom(uint8_t first, uint32_t count) {
    std::vector<uint8_t> bytes(count);
    for (uint32_t i = 0; i < count; ++i) bytes[i] = static_cast<uint8_t>(first + i);
    return bytes;
}

/// What @p span's buffer holds over it.
std::vector<uint8_t> held(const BufferSpan& span) {
    std::vector<uint8_t> bytes(span.bytes);
    glGetNamedBufferSubData(span.buffer, span.offset, span.bytes, bytes.data());
    return bytes;
}

} // namespace

TEST("a stream's spans hold what was written, each at an offset any block may bind, a frame's region its own") {
    Vkm::Test::requireContext();
    GLint storage = 0;
    GLint uniform = 0;
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &storage);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform);

    StreamRing ring;
    const std::vector<uint8_t> a = bytesFrom(1, 100);
    const std::vector<uint8_t> b = bytesFrom(7, 36);
    const BufferSpan first  = ring.write(a.data(), static_cast<uint32_t>(a.size()));
    const BufferSpan second = ring.write(b.data(), static_cast<uint32_t>(b.size()));
    CHECK(second.offset >= first.offset + first.bytes);
    for (const BufferSpan& span : {first, second}) {
        CHECK(span.offset % static_cast<uint32_t>(storage) == 0u);
        CHECK(span.offset % static_cast<uint32_t>(uniform) == 0u);
    }
    CHECK(held(first) == a);
    CHECK(held(second) == b);
    CHECK(!ring.write(a.data(), 0));

    // The next frame writes past what this one wrote, in a region of its own.
    ring.endFrame();
    const BufferSpan next = ring.write(b.data(), static_cast<uint32_t>(b.size()));
    CHECK(next.offset > second.offset + second.bytes);
    CHECK(held(first) == a);
    CHECK(held(next) == b);
}

TEST("a stream outgrown moves to a larger buffer, and the old one goes once no frame can read it") {
    Vkm::Test::requireContext();
    StreamRing ring;
    const std::vector<uint8_t> small = bytesFrom(3, 64);
    const BufferSpan before = ring.write(small.data(), static_cast<uint32_t>(small.size()));
    ring.endFrame();

    // More than a region holds: a new buffer, its regions at least that large.
    const std::vector<uint8_t> large = bytesFrom(5, 3u << 20);
    const BufferSpan grown = ring.write(large.data(), static_cast<uint32_t>(large.size()));
    CHECK(grown.buffer != before.buffer);
    CHECK(held(grown) == large);
    CHECK(glIsBuffer(before.buffer) == GL_TRUE);

    for (uint32_t frame = 0; frame < StreamRing::FRAMES; ++frame) {
        ring.endFrame();
        CHECK(glIsBuffer(before.buffer) == GL_TRUE);
        ring.write(small.data(), static_cast<uint32_t>(small.size()));
    }
    ring.endFrame();
    ring.write(small.data(), static_cast<uint32_t>(small.size()));
    CHECK(glIsBuffer(before.buffer) == GL_FALSE);
}
