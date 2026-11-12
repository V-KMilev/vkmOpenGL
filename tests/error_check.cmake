# VKM_OPENGL_CHECK reads GL's error queue only where VKM_OPENGL_CHECKS is on: off, no object of the
# library calls checkGLError, whatever else is on, VKM_OPENGL_ASSERTS included; on, those that
# check call it (cmake -P):
#
#   -DOBJECTS=<a|b|...>  the library's object files
#   -DNM=<nm>            the toolchain's nm
#   -DCHECKS=<bool>      VKM_OPENGL_CHECKS

string(REPLACE "|" ";" _objects "${OBJECTS}")
set(_calling "")
foreach(_object IN LISTS _objects)
    execute_process(COMMAND "${NM}" -u "${_object}" OUTPUT_VARIABLE _undefined RESULT_VARIABLE _status)
    if(NOT _status EQUAL 0)
        message(FATAL_ERROR "${NM} -u ${_object} failed")
    endif()
    if(_undefined MATCHES "checkGLError")
        list(APPEND _calling "${_object}")
    endif()
endforeach()

if(NOT CHECKS AND _calling)
    string(REPLACE ";" "\n  " _calling "${_calling}")
    message(FATAL_ERROR "VKM_OPENGL_CHECKS is off, yet these call checkGLError:\n  ${_calling}")
elseif(CHECKS AND NOT _calling)
    message(FATAL_ERROR "VKM_OPENGL_CHECKS is on, yet no object calls checkGLError")
endif()
