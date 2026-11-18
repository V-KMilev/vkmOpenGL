// What runs with no GL context: the error check's shape with the checks off, load() refusing, and a
// precondition stopping.

#include "vkm_opengl/core/error_handle.h"
#include "vkm_opengl/core/load.h"
#include "vkm_opengl/texture/texture_2d_array.h"

#include "harness/check.h"
#include "harness/scratch.h"

TEST("VKM_OPENGL_CHECK runs its call exactly once, and not at all under a branch not taken") {
#ifdef VKM_OPENGL_CHECKS
    // On, it then calls glGetError, and with no context current what that answers is undefined.
    Vkm::Test::skip("the checks are on, and read GL, which needs a context: gpu/objects.cpp checks it");
#else
    int calls = 0;
    VKM_OPENGL_CHECK(++calls);
    CHECK(calls == 1);
    if (calls == 0) VKM_OPENGL_CHECK(++calls);
    CHECK(calls == 1);
#endif
}

TEST("load with no current context answers no, says why, and throws nothing") {
    const Vkm::OpenGL::Loaded loaded = Vkm::OpenGL::load();
    CHECK(!loaded.ok);
    CHECK(!loaded.reason.empty());
}

TEST("with the library's asserts on, a broken precondition stops the program") {
    // Off, the flags check sees no assert in the library.
    if (!VKM_TEST_LIBRARY_ASSERTS) Vkm::Test::skip("the library's asserts are off in this build");
    // No layers, no size: refused before any GL call.
    const Vkm::Test::Ended ended = Vkm::Test::inChild([] { Vkm::OpenGL::Texture2DArray("empty", {}); });
    CHECK(ended == Vkm::Test::Ended::Stopped);
}
