#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Vkm::OpenGL {

/**
 * @brief The way a cube face looks out of the cube's center, and which way is up in it.
 */
struct CubeFace {
    glm::vec3 dir;  ///< Out through the face's center.
    glm::vec3 up;   ///< Up in the face, as GL samples it.
};

/**
 * @brief Each face's view, in OpenGL's face order (+X, -X, +Y, -Y, +Z, -Z): face @c i is
 *        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i.
 *
 * Drawing into a cube's faces must use the basis GL samples with: the specification's
 * face-selection table picks a face by a direction's major axis, and the face's t grows
 * along the axis given here as up. A face drawn with any other up is sampled turned.
 */
inline const CubeFace CUBE_FACES[6] = {
    {{ 1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}},
    {{-1.0f,  0.0f,  0.0f}, {0.0f, -1.0f,  0.0f}},
    {{ 0.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}},
    {{ 0.0f, -1.0f,  0.0f}, {0.0f,  0.0f, -1.0f}},
    {{ 0.0f,  0.0f,  1.0f}, {0.0f, -1.0f,  0.0f}},
    {{ 0.0f,  0.0f, -1.0f}, {0.0f, -1.0f,  0.0f}},
};

/**
 * @brief The view matrix looking down face @p face from @p eye. The projection is the
 *        caller's: a 90-degree square frustum, at a near and far of its own.
 *
 * @param face The cube face, 0-5 in GL's order.
 * @param eye  Where the cube is seen from: a probe's position, or the origin.
 * @return The view matrix.
 */
inline glm::mat4 cubeFaceView(int face, const glm::vec3& eye) {
    return glm::lookAt(eye, eye + CUBE_FACES[face].dir, CUBE_FACES[face].up);
}

} // namespace Vkm::OpenGL
