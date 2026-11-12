// The vertex side's arithmetic: a buffer layout's stride and elements, and the indirect draw's record.

#include <cstddef>
#include <cstdint>
#include <utility>

#include "vkm_opengl/vertex/vertex_array.h"
#include "vkm_opengl/vertex/vertex_buffer_layout.h"

#include "harness/check.h"

using Vkm::OpenGL::VertexBufferElement;
using Vkm::OpenGL::VertexBufferLayout;

TEST("a layout's stride adds each push's bytes, and clear empties it") {
    VertexBufferLayout layout;
    CHECK(layout.getStride() == 0u);
    CHECK(layout.getElements().empty());

    layout.push<float>(3);
    CHECK(layout.getStride() == 12u);
    layout.push<float>(2);
    CHECK(layout.getStride() == 20u);
    layout.push<uint8_t>(4);
    CHECK(layout.getStride() == 24u);
    layout.push<uint16_t>(4);
    CHECK(layout.getStride() == 32u);
    CHECK(layout.getElements().size() == 4u);

    layout.clear();
    CHECK(layout.getStride() == 0u);
    CHECK(layout.getElements().empty());
}

TEST("bytes push normalized, so 0 to 255 reads as 0 to 1, and floats and shorts do not") {
    VertexBufferLayout layout;
    layout.push<float>(3);
    layout.push<uint8_t>(4);
    layout.push<uint16_t>(2);
    CHECK(layout.getElements()[0].normalized == GL_FALSE);
    CHECK(layout.getElements()[1].normalized == GL_TRUE);
    CHECK(layout.getElements()[2].normalized == GL_FALSE);
    CHECK(layout.getElements()[1].count == 4u);
}

TEST("a layout copies and moves like a value, the copy its own") {
    VertexBufferLayout source;
    source.push<float>(3);
    VertexBufferLayout copied = source;
    CHECK(copied.getStride() == 12u);
    CHECK(copied.getElements().size() == 1u);
    copied.push<float>(1);
    CHECK(source.getStride() == 12u);

    VertexBufferLayout moved = std::move(copied);
    CHECK(moved.getStride() == 16u);
}

TEST("each GL type's size is its bytes") {
    CHECK(VertexBufferElement::getSizeOfType(GL_FLOAT) == 4u);
    CHECK(VertexBufferElement::getSizeOfType(GL_UNSIGNED_INT) == 4u);
    CHECK(VertexBufferElement::getSizeOfType(GL_UNSIGNED_BYTE) == 1u);
    CHECK(VertexBufferElement::getSizeOfType(GL_SHORT) == 2u);
    CHECK(VertexBufferElement::getSizeOfType(GL_DOUBLE) == 8u);
}

TEST("an indirect draw's record is 20 bytes, its five words where glMultiDrawElementsIndirect reads them") {
    using Command = Vkm::OpenGL::DrawElementsIndirectCommand;
    CHECK(sizeof(Command) == 20u);
    CHECK(offsetof(Command, count) == 0u);
    CHECK(offsetof(Command, instanceCount) == 4u);
    CHECK(offsetof(Command, firstIndex) == 8u);
    CHECK(offsetof(Command, baseVertex) == 12u);
    CHECK(offsetof(Command, baseInstance) == 16u);
}
