# vkm_opengl built alone, as CI builds it (.github/workflows/ci.yml): a consumer that adds what the
# README asks for first, glm and vkm_log, at the commits vkmEngine pins, then the library with its
# suite.
#
#   cmake -S ci -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   ctest --test-dir build --output-on-failure
cmake_minimum_required(VERSION 3.25)
project(vkm_opengl_ci LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        4962d27b37628027b7d641762e3d6a9b50778fe0
    SYSTEM
)
FetchContent_Declare(vkmLog
    GIT_REPOSITORY https://github.com/V-KMilev/vkmLog.git
    GIT_TAG        70cba6eb423c4ac6e3e314ff4ecb4ab6d36eb8cc
)
FetchContent_MakeAvailable(glm vkmLog)

enable_testing()
set(VKM_OPENGL_ASSERTS     ON)
set(VKM_OPENGL_CHECKS      ON)
set(VKM_OPENGL_BUILD_TESTS ON)
add_subdirectory(.. vkm_opengl)

# A warning in the library or its suite fails the build, as CONTRIBUTING.md asks; one in what it
# vendors or fetches does not.
foreach(_dir .. ../tests)
    get_property(_targets DIRECTORY ${_dir} PROPERTY BUILDSYSTEM_TARGETS)
    set_property(TARGET ${_targets} PROPERTY COMPILE_WARNING_AS_ERROR ON)
endforeach()
