#pragma once

#include <string>

namespace Vkm::Test {

/**
 * @brief The context a GPU test runs in: an OpenGL context of the version vkmOpenGL needs, current on
 *        this thread, made through EGL's device platform with no window, no display and no surface.
 *
 * Made once a process, on first use. With no device that can give one, the test is skipped, saying
 * why, unless VKM_GPU_REQUIRED=1 asks the machine for one: then it fails.
 */
void requireContext();

} // namespace Vkm::Test
