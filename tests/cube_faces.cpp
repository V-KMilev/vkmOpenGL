// The cube map's faces, as the specification's face-selection table gives them.

#include <glm/glm.hpp>

#include "vkm_opengl/texture/cube_faces.h"

#include "harness/check.h"

using Vkm::OpenGL::CUBE_FACES;

TEST("the six faces are in GL_TEXTURE_CUBE_MAP_POSITIVE_X + i order, each up as the specification's table") {
    // On face i, t grows with -ry for the X and Z faces, with +rz on +Y and -rz on -Y.
    const glm::vec3 directions[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const glm::vec3 ups[6]        = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};
    for (int i = 0; i < 6; ++i) {
        CHECK(CUBE_FACES[i].dir == directions[i]);
        CHECK(CUBE_FACES[i].up == ups[i]);
        CHECK(glm::dot(CUBE_FACES[i].dir, CUBE_FACES[i].up) == 0.0f);
        CHECK(glm::length(CUBE_FACES[i].dir) == 1.0f);
        CHECK(glm::length(CUBE_FACES[i].up) == 1.0f);
    }
}

TEST("cubeFaceView looks down its face from the eye, its up up") {
    const glm::vec3 eye(2.0f, -3.0f, 5.0f);
    for (int i = 0; i < 6; ++i) {
        const glm::mat4 view = Vkm::OpenGL::cubeFaceView(i, eye);
        CHECK_NEAR(glm::vec3(view * glm::vec4(eye + CUBE_FACES[i].dir, 1.0f)), glm::vec3(0, 0, -1), 1e-6f);
        CHECK_NEAR(glm::vec3(view * glm::vec4(CUBE_FACES[i].up, 0.0f)), glm::vec3(0, 1, 0), 1e-6f);
    }
}
