function(add_option_bool option_name default_value message)
    set(${option_name} ${default_value} CACHE BOOL "${message}")
endfunction()

function(add_option_string option_name default_value message)
    set(${option_name} ${default_value} CACHE STRING "${message}")
endfunction()

function(add_option_string_force option_name default_value message)
    set(${option_name} ${default_value} CACHE STRING "${message}" FORCE)
endfunction()

function(add_option_choice option_name default_value values_list message)
    add_option_string(${option_name} ${default_value} "${message}, options are: ${values_list}")
    set_property(CACHE ${option_name} PROPERTY STRINGS ${values_list})
endfunction()

function(add_option_choice_force option_name default_value values_list message)
    add_option_string_force(${option_name} ${default_value} "${message}, options are: ${values_list}")
    set_property(CACHE ${option_name} PROPERTY STRINGS ${values_list})
endfunction()

# function(add_option_internal option_name default_value)
# add_option_string(${option_name} ${default_value} "INTERNAL CONFIG VARIABLE DO NOT SET MANUALLY")
# endfunction()
function(set_index the_list the_value the_output)
    set(index 0)

    foreach(string ${the_list})
        if(${string} STREQUAL ${the_value})
            set(${the_output} ${index} CACHE STRING "INTERNAL CONFIG VARIABLE DO NOT SET MANUALLY" FORCE)
            mark_as_advanced(${the_output})
        endif()

        math(EXPR index "${index}+1")
    endforeach()
endfunction()

function(add_option_numbered_choice option_name default_value values_list message)
    add_option_string(${option_name} ${default_value} "${message}, options are: ${values_list}")
    set_property(CACHE ${option_name} PROPERTY STRINGS ${values_list})

    # add_option_internal(${option_name}_VALUE 0)
    set_index("${values_list}" "${${option_name}}" ${option_name}_VALUE)
endfunction()

function(check_optional_library library_name)
    string(TOUPPER ${library_name} library_name_upper)

    if(${library_name})
        # Library found
        message(STATUS "Library ${library_name} found!")
        set(CONFIG_${library_name_upper}_USE ON CACHE BOOL "INTERNAL CONFIG VARIABLE DO NOT SET MANUALLY" FORCE)
        mark_as_advanced(CONFIG_${library_name_upper}_USE)
    else()
        set(CONFIG_${library_name_upper}_USE OFF CACHE BOOL "INTERNAL CONFIG VARIABLE DO NOT SET MANUALLY" FORCE)
        mark_as_advanced(CONFIG_${library_name_upper}_USE)

        if(CONFIG_${library_name_upper}_REQUIRED)
            message(FATAL_ERROR "REQUIRED OpenCL library not found!")
        endif()
    endif()
endfunction()

function(message_library library_name)
    if(${library_name})
        message(STATUS "LIBRARY ${library_name} FOUND!")
    else()
        message(STATUS "LIBRARY ${library_name} NOT FOUND!")
    endif()
endfunction()

function(target_optional_link_library target library_name)
    string(TOUPPER ${library_name} library_name_upper)

    if(${library_name})
        if(CONFIG_${library_name_upper}_USE)
            target_link_libraries(${target} ${${library_name}})
        endif()
    endif()
endfunction()
