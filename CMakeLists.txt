cmake_minimum_required(VERSION 3.25.0)

set(PROJECT_NAME vkm_opengl)
set(PROJECT_VERSION_MAJOR 2)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

project(
  ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  LANGUAGES C CXX
)

# Set the C++ standard to 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message("[VKM_OPENGL] Building vkm_opengl v${PROJECT_VERSION}")

# Find OpenGL
find_package(OpenGL REQUIRED)

# Enumerated as one sweep rather than as a list: a list has to be edited to add
# a file, and the edit is invisible when it is forgotten - the new .cpp simply
# does not link, and the first sign is a missing symbol somewhere else.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# The public surface is not annotated with __declspec(dllexport), so on Windows
# CMake generates the export table from the object files instead - the same
# property vkm_core carries, and for the same reason. Without it an MSVC build
# links a DLL that exports nothing.
set_target_properties(${PROJECT_NAME} PROPERTIES
	WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# Warnings are on for our own sources only - the submodules under modules/ keep
# their own level. The set is the engine's, so a file reads the same on either
# side of the boundary. Clang reads the doc blocks: a @param naming no parameter
# is an error.
set(_vkm_opengl_warnings
	-Wall
	-Wextra
	-Wno-unused-parameter  # interface stubs, placeholders
	-Wshadow
	-Wunused-variable
	-Wunused-value
	-Wformat=2
	-Wnon-virtual-dtor
	-Woverloaded-virtual
)
target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:GNU,Clang>:${_vkm_opengl_warnings}>
	$<$<CXX_COMPILER_ID:Clang>:-Werror=documentation>
)

# The root above vkm_opengl/, so an include names the library and its folder:
# "vkm_opengl/texture/texture.h".
target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<INSTALL_INTERFACE:include>
)

# Debug features follow flags, each off unless the consumer turns it on; neither
# follows NDEBUG.
option(VKM_OPENGL_ASSERTS "Keep the library's VKM_ASSERTs on whatever the build type" OFF)
option(VKM_OPENGL_CHECKS  "Read GL's error queue after every call (VKM_OPENGL_CHECK)" OFF)

# VKM_ASSERT keys on NDEBUG, which a RelWithDebInfo or Release build defines: on,
# the library's own sources compile without it.
if(VKM_OPENGL_ASSERTS)
	target_compile_options(${PROJECT_NAME} PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/UNDEBUG,-UNDEBUG>)
endif()

# Public: the headers' inline calls are checked too, and every file that expands
# them must expand them alike.
if(VKM_OPENGL_CHECKS)
	target_compile_definitions(${PROJECT_NAME} PUBLIC VKM_OPENGL_CHECKS)
endif()

# Its log lines say whose they are.
target_compile_definitions(${PROJECT_NAME} PRIVATE
	VKM_LOG_SUFFIX="VKM-OPENGL"
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/modules)

# Tests cover only what runs without a GL context, so this binary stays usable
# on a build machine with no GPU. vkm_opengl is always a consumer's subdirectory
# (it needs the glm and vkm_log targets the consumer adds), so built only when the
# consumer asks for them.
option(VKM_OPENGL_BUILD_TESTS "Build the vkm_opengl test suite" OFF)

if(VKM_OPENGL_BUILD_TESTS)
	enable_testing()
	add_executable(vkm_opengl_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/vkm_opengl_tests.cpp)
	target_link_libraries(vkm_opengl_tests PRIVATE ${PROJECT_NAME})
	target_compile_options(vkm_opengl_tests PRIVATE
		$<$<CXX_COMPILER_ID:GNU,Clang>:${_vkm_opengl_warnings}>
		$<$<CXX_COMPILER_ID:Clang>:-Werror=documentation>
	)
	add_test(NAME vkm_opengl COMMAND vkm_opengl_tests)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
	OpenGL::GL
	glew
	glm
	vkm_log
)
