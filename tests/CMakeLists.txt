# vkm_opengl's own tests: TEST("what works") in tests/<topic>.cpp (harness/check.h). Each runs in a
# process of its own and is a ctest of its own, named "vkm_opengl: <what works>" and labeled
# vkm_opengl, so `ctest -L vkm_opengl` runs the suite in a consumer's tree too, with -C <config> under
# a multi-config generator. Beside them, checks of the library as a consumer builds it: its includes,
# its CMake, its debug flags.
#
# What runs without a GL context runs anywhere; what needs one is in gpu/, below.

file(GLOB VKM_OPENGL_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(vkm_opengl_tests ${CMAKE_CURRENT_SOURCE_DIR}/harness/main.cpp ${VKM_OPENGL_TEST_SOURCES})
target_link_libraries(vkm_opengl_tests PRIVATE ${PROJECT_NAME})
target_include_directories(vkm_opengl_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Windows finds the DLLs a program needs beside it: the library's, glew's and vkm_log's, each built in a
# folder of its own, are copied beside each suite.
function(_vkm_opengl_dlls_beside target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}>
                $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endfunction()
_vkm_opengl_dlls_beside(vkm_opengl_tests)
target_compile_definitions(vkm_opengl_tests PRIVATE
    VKM_TEST_SUITE="vkm_opengl"
    VKM_TEST_LOG
    # The library's asserts flag, which check.h's VKM_TEST_LIBRARY_ASSERTS reads beside the file's NDEBUG.
    VKM_TEST_ASSERTS_FLAG=$<BOOL:${VKM_OPENGL_ASSERTS}>
)
target_compile_options(vkm_opengl_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:${_vkm_opengl_warnings}>
    $<$<CXX_COMPILER_ID:Clang>:-Werror=documentation>
)

# Every test the binary lists becomes a ctest when ctest runs: a test added is a test run, with no
# list to keep.
include(${CMAKE_CURRENT_SOURCE_DIR}/harness/discover.cmake)
vkm_discover_tests(vkm_opengl_tests vkm_opengl)

# What needs a context, in a binary of its own: each test makes an OpenGL context through EGL's device
# platform (gpu/context.h), so it runs on a GPU with no display, and is skipped, saying why, on a
# machine with none (or fails where VKM_GPU_REQUIRED=1). Built only where EGL is; labeled gpu as well.
find_package(OpenGL QUIET COMPONENTS EGL)
if(TARGET OpenGL::EGL)
    file(GLOB VKM_OPENGL_GPU_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gpu/*.cpp)
    add_executable(vkm_opengl_gpu_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/harness/main.cpp
        ${VKM_OPENGL_GPU_TEST_SOURCES}
    )
    target_link_libraries(vkm_opengl_gpu_tests PRIVATE ${PROJECT_NAME} OpenGL::EGL)
    target_include_directories(vkm_opengl_gpu_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(vkm_opengl_gpu_tests PRIVATE
        VKM_TEST_SUITE="vkm_opengl"
        VKM_TEST_LOG
        VKM_TEST_ASSERTS_FLAG=$<BOOL:${VKM_OPENGL_ASSERTS}>
    )
    target_compile_options(vkm_opengl_gpu_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:${_vkm_opengl_warnings}>
        $<$<CXX_COMPILER_ID:Clang>:-Werror=documentation>
    )

    _vkm_opengl_dlls_beside(vkm_opengl_gpu_tests)
    vkm_discover_tests(vkm_opengl_gpu_tests vkm_opengl gpu)
endif()

# The library as a consumer builds it.
set(_vkm_opengl_checks
    "vkm_opengl: every include is its own, the standard library's, glew's, glm's or vkmLog's, and no window library's"
    "vkm_opengl: a consumer adds it after glm and vkm_log, and without either it stops and says which"
)
add_test(NAME "vkm_opengl: every include is its own, the standard library's, glew's, glm's or vkmLog's, and no window library's"
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${PROJECT_SOURCE_DIR}/src -P ${CMAKE_CURRENT_SOURCE_DIR}/includes.cmake
)
add_test(NAME "vkm_opengl: a consumer adds it after glm and vkm_log, and without either it stops and says which"
    COMMAND ${CMAKE_COMMAND} -DLIBRARY_DIR=${PROJECT_SOURCE_DIR} -DREQUIRES=glm|vkm_log
        -DOUT=${CMAKE_CURRENT_BINARY_DIR}/consumer -DGENERATOR=${CMAKE_GENERATOR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/harness/consumer.cmake
)
if(CMAKE_NM)
    # The library's compile commands, which the check reads, whether or not the consumer exports its own.
    set_property(TARGET ${PROJECT_NAME} PROPERTY EXPORT_COMPILE_COMMANDS ON)
    list(APPEND _vkm_opengl_checks
        "vkm_opengl: its asserts follow VKM_OPENGL_ASSERTS whatever the build type"
        "vkm_opengl: GL's error queue is read after a call only where VKM_OPENGL_CHECKS is on, whatever else is"
    )
    add_test(NAME "vkm_opengl: its asserts follow VKM_OPENGL_ASSERTS whatever the build type"
        COMMAND ${CMAKE_COMMAND} -DLIBRARY=${PROJECT_NAME} -DSOURCE_DIR=${PROJECT_SOURCE_DIR}/src
            -DARCHIVE=$<TARGET_FILE:${PROJECT_NAME}> -DNM=${CMAKE_NM} -DASSERTS=${VKM_OPENGL_ASSERTS}
            -DPROFILER=OFF -DCONFIG=$<CONFIG> -DCOMPILE_COMMANDS=${CMAKE_BINARY_DIR}/compile_commands.json
            -P ${CMAKE_CURRENT_SOURCE_DIR}/harness/flags.cmake
    )
    add_test(NAME "vkm_opengl: GL's error queue is read after a call only where VKM_OPENGL_CHECKS is on, whatever else is"
        COMMAND ${CMAKE_COMMAND} "-DOBJECTS=$<JOIN:$<TARGET_OBJECTS:${PROJECT_NAME}>,|>" -DNM=${CMAKE_NM}
            -DCHECKS=${VKM_OPENGL_CHECKS} -P ${CMAKE_CURRENT_SOURCE_DIR}/error_check.cmake
    )
endif()
set_tests_properties(${_vkm_opengl_checks} PROPERTIES LABELS vkm_opengl)
