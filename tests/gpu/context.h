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

/**
 * @brief The context with a default framebuffer: a @p width x @p height RGBA8 pbuffer, made current with
 *        it and kept until the process ends.
 *
 * Skipped, or failed under VKM_GPU_REQUIRED=1, as requireContext() is, when the device gives no pbuffer.
 */
void requireDefaultFramebuffer(int width, int height);

/**
 * @brief Skip the test for want of what a GPU gives, or fail it where VKM_GPU_REQUIRED=1 asks the
 *        machine for it.
 *
 * @param why What is missing, for the skip's or the failure's line.
 */
[[noreturn]] void missing(const std::string& why);

} // namespace Vkm::Test
