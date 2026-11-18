// What vkmOpenGL does on a real context: what it counts, labels and asks the driver.

#include <cstdint>
#include <string>

#include <GL/glew.h>

#include "vkm_opengl/buffer/buffer.h"
#include "vkm_opengl/buffer/readback_buffer.h"
#include "vkm_opengl/core/context.h"
#include "vkm_opengl/core/debug.h"
#include "vkm_opengl/core/error_handle.h"
#include "vkm_opengl/texture/texture_3d.h"

#include "gpu/context.h"
#include "harness/check.h"
#include "harness/scratch.h"

using namespace Vkm::OpenGL;

TEST("with the checks on, VKM_OPENGL_CHECK runs its call once, and not at all under a branch not taken") {
#ifndef VKM_OPENGL_CHECKS
    Vkm::Test::skip("VKM_OPENGL_CHECKS is off: core.cpp checks the macro with no context");
#else
    Vkm::Test::requireContext();
    int calls = 0;
    VKM_OPENGL_CHECK(++calls);
    CHECK(calls == 1);
    if (calls == 0) VKM_OPENGL_CHECK(++calls);
    CHECK(calls == 1);
#endif
}

TEST("a readback buffer's delete is counted as every buffer's is") {
    Vkm::Test::requireContext();
    const uint64_t before = deletedBuffers();
    {
        const ReadbackBuffer readback(256);
    }
    CHECK(deletedBuffers() == before + 1);
}

TEST("a 3D texture carries the name it is made with as its label") {
    Vkm::Test::requireContext();
    Texture3DParams params;
    params.width  = 4;
    params.height = 4;
    params.depth  = 4;
    const Texture3D volume("probe volume", params);

    char    label[64] = {};
    GLsizei length    = 0;
    glGetObjectLabel(GL_TEXTURE, volume.getID(), sizeof(label), &length, label);
    CHECK(std::string(label, static_cast<size_t>(length)) == "probe volume");
}

TEST("maxTextureSize is the driver's own GL_MAX_TEXTURE_SIZE") {
    Vkm::Test::requireContext();
    GLint driver = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &driver);
    const Context context;
    CHECK(context.maxTextureSize() == driver);
}

TEST("enableGLDebugLogging turns the driver's debug output on") {
    Vkm::Test::requireContext();
    enableGLDebugLogging();
    CHECK(glIsEnabled(GL_DEBUG_OUTPUT) == GL_TRUE);
}
