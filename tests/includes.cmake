# Every #include under src/vkm_opengl is the library's own, the standard library's, glew's, glm's or
# vkmLog's, and no window library's (cmake -P, -DSOURCE_DIR=<src>).

file(GLOB_RECURSE _files "${SOURCE_DIR}/vkm_opengl/*.h" "${SOURCE_DIR}/vkm_opengl/*.cpp")
set(_problems "")
foreach(_file IN LISTS _files)
    file(STRINGS "${_file}" _includes REGEX "^[ \t]*#[ \t]*include")
    foreach(_line IN LISTS _includes)
        string(REGEX MATCH "[<\"]([^>\"]+)[>\"]" _ "${_line}")
        set(_header "${CMAKE_MATCH_1}")
        if(_header MATCHES "^vkm_opengl/" AND EXISTS "${SOURCE_DIR}/${_header}")
        elseif(_line MATCHES "<[a-z_]+>")
        elseif(_header MATCHES "^(glm|vkm_log)/" OR _header STREQUAL "GL/glew.h")
        else()
            string(APPEND _problems "  ${_file}: ${_line}\n")
        endif()
    endforeach()
endforeach()

if(_problems)
    message(FATAL_ERROR "Includes the library may not have:\n${_problems}")
endif()
