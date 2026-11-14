// The indirect draw's record, laid out as glMultiDrawElementsIndirect reads it.

#include <cstddef>

#include "vkm_opengl/vertex/vertex_array.h"

#include "harness/check.h"

TEST("an indirect draw's record is 20 bytes, its five words where glMultiDrawElementsIndirect reads them") {
    using Command = Vkm::OpenGL::DrawElementsIndirectCommand;
    CHECK(sizeof(Command) == 20u);
    CHECK(offsetof(Command, count) == 0u);
    CHECK(offsetof(Command, instanceCount) == 4u);
    CHECK(offsetof(Command, firstIndex) == 8u);
    CHECK(offsetof(Command, baseVertex) == 12u);
    CHECK(offsetof(Command, baseInstance) == 16u);
}
