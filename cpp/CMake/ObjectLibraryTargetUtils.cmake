#===================================================================================================
# Copyright (C) 2020, BMW Car IT GmbH
# Author: Mátyás Forstner <matyas.forstner@bmw.de>
#===================================================================================================
# This module provides utility functions for handling OBJECT libraries:
#
# objlibrary_target_link_libraries(<target>
#                                  <PRIVATE|PUBLIC|INTERFACE> <item>...
#                                  [<PRIVATE|PUBLIC|INTERFACE> <item>...]...)
#
# For CMake older than 3.12 target_link_libraries cannot be called for OBJECT libraries.
# This is a compatibility wrapper.
#
#
# target_link_objlibraries(<target>
#                          [EXCLUDE_REGEX <regex>]
#                          [<PRIVATE|PUBLIC|INTERFACE> <item>...]...)
#
# This function copies the INTERFACE_xxx properties from OBJECT library targets to that of
# specified target (first argument).
# Property values matching <regex> are filtered out.
# The behavior is otherwise analogous with target_link_libraries.
#
#===================================================================================================

include_guard(GLOBAL)

set(OLTU_DEBUGLOG OFF)

function(_oltu_debug msg)
    if(OLTU_DEBUGLOG)
        message(STATUS "OLTU: ${msg}")
    endif()
endfunction()

# _copy_property_value(FROM <target>
#                       TO <target>
#                       PROPERTY <property>
#                       [TO_PROPERTY <property>]
#                       [EXCLUDE_LIBS <lib>...]
#                       [EXCLUDE_REGEX <regex>]
#                       [REMOVE_DUPLICATES]
#                      )
function(_copy_property_value)
    cmake_parse_arguments(PARSE_ARGV 0 _ARGS
        "REMOVE_DUPLICATES" "FROM;TO;PROPERTY;TO_PROPERTY;EXCLUDE_REGEX" "EXCLUDE_LIBS"
    )
    if(NOT _ARGS_FROM)
        message(FATAL_ERROR "'FROM' not specified")
    endif()
    if(NOT _ARGS_TO)
        message(FATAL_ERROR "'TO' not specified")
    endif()
    if(NOT _ARGS_PROPERTY)
        message(FATAL_ERROR "'PROPERTY' not specified")
    endif()
    if(NOT _ARGS_TO_PROPERTY)
        set(_ARGS_TO_PROPERTY ${_ARGS_PROPERTY})
    endif()
    get_property(propValue TARGET ${_ARGS_FROM} PROPERTY ${_ARGS_PROPERTY})
    if(propValue)
        get_property(ownPropValue TARGET ${_ARGS_TO} PROPERTY ${_ARGS_TO_PROPERTY})
        set(msg "to: ${_ARGS_TO}.${_ARGS_TO_PROPERTY} <- from: ${_ARGS_FROM}.${_ARGS_PROPERTY}")
        string(APPEND msg " # (${ownPropValue}) + ")
        set(msg2a "${propValue}")
        set(msg2b "")

        if(${_ARGS_PROPERTY} STREQUAL "INTERFACE_LINK_LIBRARIES"
                AND _ARGS_EXCLUDE_LIBS)
            string(APPEND msg2b " - (${_ARGS_EXCLUDE_LIBS})")
            list(REMOVE_ITEM propValue ${_ARGS_EXCLUDE_LIBS})
        endif()
        if(_ARGS_EXCLUDE_REGEX)
            string(APPEND msg2b " - '${_ARGS_EXCLUDE_REGEX}'")
            list(FILTER propValue EXCLUDE REGEX "${_ARGS_EXCLUDE_REGEX}")
        endif()
        if(msg2b)
            string(APPEND msg "(${msg2a}${msg2b})")
        else()
            string(APPEND msg "${msg2a}")
        endif()
        list(APPEND ownPropValue ${propValue})
        if(_ARGS_REMOVE_DUPLICATES)
            string(APPEND msg " - DUPLICATES")
            list(REMOVE_DUPLICATES ownPropValue)
        endif()
        string(APPEND msg " = ${ownPropValue}")
        _oltu_debug("${msg}")
        set_property(TARGET ${_ARGS_TO} PROPERTY ${_ARGS_TO_PROPERTY} "${ownPropValue}")
    endif()
endfunction()

# _copy_property_values(FROM <target>
#                       TO <target>
#                       [FROM_PROP_PREFIX <prefix>]   -- e.g. INTERFACE_ or empty
#                       [TO_PROP_PREFIX <prefix>]     -- e.g. INTERFACE_ or empty
#                       [EXCLUDE_LIBS <lib>...]
#                       [EXCLUDE_REGEX <regex>]
#                      )
function(_copy_property_values)
    cmake_parse_arguments(
        PARSE_ARGV 0 _ARGS ""
        "FROM;TO;FROM_PROP_PREFIX;TO_PROP_PREFIX;EXCLUDE_REGEX" "EXCLUDE_LIBS")
    if(NOT _ARGS_FROM)
        message(FATAL_ERROR "'FROM' not specified")
    endif()
    if(NOT _ARGS_TO)
        message(FATAL_ERROR "'TO' not specified")
    endif()

    foreach(property
            COMPILE_DEFINITIONS COMPILE_FEATURES COMPILE_OPTIONS
            INCLUDE_DIRECTORIES SYSTEM_INCLUDE_DIRECTORIES
            LINK_DEPENDS LINK_DIRECTORIES PRECOMPILE_HEADERS
            # SOURCES
    )
        _copy_property_value(FROM ${_ARGS_FROM}
            TO ${_ARGS_TO}
            PROPERTY "${_ARGS_FROM_PROP_PREFIX}${property}"
            TO_PROPERTY "${_ARGS_TO_PROP_PREFIX}${property}"
            EXCLUDE_REGEX "${_ARGS_EXCLUDE_REGEX}"
            REMOVE_DUPLICATES
        )
    endforeach()
    foreach(property
            LINK_LIBRARIES LINK_OPTIONS
    )
        _copy_property_value(FROM ${_ARGS_FROM}
            TO ${_ARGS_TO}
            PROPERTY "${_ARGS_FROM_PROP_PREFIX}${property}"
            TO_PROPERTY "${_ARGS_TO_PROP_PREFIX}${property}"
            EXCLUDE_LIBS ${_ARGS_EXCLUDE_LIBS}
            EXCLUDE_REGEX "${_ARGS_EXCLUDE_REGEX}"
            # For group of options, hopefully "SHELL:a b c" is used, so remove duplicates also here.
            REMOVE_DUPLICATES
        )
    endforeach()
endfunction()

# This function copies the INTERFACE_xxx properties from OBJECT_LIBRARY targets to that of target.
# Arguments:
#   <target>
#   EXCLUDE_REGEX <regex> - Skip property values matching this regex.
#   [<PRIVATE|PUBLIC|INTERFACE> <module>...] - Copy property values from these modules.
function(target_link_objlibraries target)
    cmake_parse_arguments(PARSE_ARGV 1 _ARGS "" "EXCLUDE_REGEX" "")
    get_target_property(targetType ${target} TYPE)
    if(NOT "${targetType}" MATCHES "LIBRARY|EXECUTABLE")
        message(FATAL_ERROR
            "Target ${target} is not a LIBRARY or EXECUTABLE. Maybe use objlibrary_target_link_libraries?")
    endif()

    set(_linkLibs)
    set(_interfaceLibs)
    set(_scope PUBLIC)
    foreach(objlib ${_ARGS_UNPARSED_ARGUMENTS})
        if(objlib MATCHES "^(PRIVATE|PUBLIC|INTERFACE)$")
            set(_scope ${objlib})
            continue()
        endif()
        get_target_property(objlibTargetType ${objlib} TYPE)
        if(NOT "${objlibTargetType}" STREQUAL "OBJECT_LIBRARY")
            message(FATAL_ERROR
                "Target ${objlib} is not an OBJECT library. Use target_link_libraries(${target} ${objlib}).")
        endif()
        if("${targetType}" STREQUAL "OBJECT_LIBRARY")
            _oltu_debug("Warning: target_link_objlibraries() will not propagate the object files from ${objlib} into ${target}, as it is an OBJECT library.")
        else()
            # Addig submodules as objects.
            # Adding them always as PRIVATE because PUBLIC would require exporting also them.
            target_sources(${target} PRIVATE
                $<TARGET_OBJECTS:${objlib}>
            )
        endif()
        if(_scope MATCHES "^(PRIVATE|PUBLIC)$")
            list(APPEND _linkLibs ${objlib})
        endif()
        if(_scope MATCHES "^(PUBLIC|INTERFACE)$")
            list(APPEND _interfaceLibs ${objlib})
        endif()
    endforeach()

    # EXCLUDE_LIBS: This is relevant for the LINK_LIBRARIES:
    # remove dependencies to object libs which are added anyway, otherwise exporting
    # ${target} wouldn't be possible, as the object libs have no export.
    foreach(objlib ${_linkLibs})
        _copy_property_values(FROM ${objlib}
                              TO ${target}
                              FROM_PROP_PREFIX "INTERFACE_"
                              TO_PROP_PREFIX ""
                              EXCLUDE_LIBS ${_linkLibs}
                              EXCLUDE_REGEX "${_ARGS_EXCLUDE_REGEX}"
                             )
    endforeach()
    foreach(objlib ${_interfaceLibs})
        _copy_property_values(FROM ${objlib}
                              TO ${target}
                              FROM_PROP_PREFIX "INTERFACE_"
                              TO_PROP_PREFIX "INTERFACE_"
                              EXCLUDE_LIBS ${_interfaceLibs}
                              EXCLUDE_REGEX "${_ARGS_EXCLUDE_REGEX}"
                             )
    endforeach()
endfunction()

# For CMake older than 3.12 target_link_libraries cannot be called for OBJECT libraries.
# This is a compatibility wrapper.
function(objlibrary_target_link_libraries target)
    get_target_property(targetType ${target} TYPE)
    if(NOT "${targetType}" STREQUAL "OBJECT_LIBRARY")
        message(FATAL_ERROR "Target ${target} is not an OBJECT library. Use target_link_[obj]libraries(${target} ${ARGN}).")
    endif()
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
        target_link_libraries(${target} ${ARGN})
        return()
    endif()
    set(_scope PUBLIC)
    foreach(arg ${ARGN})
        if(arg MATCHES "^(PRIVATE|PUBLIC|INTERFACE)$")
            set(_scope ${arg})
            continue()
        endif()
        if(arg MATCHES "^(debug|optimized)$")
            message(FATAL_ERROR "Target ${target}: Not supported: '${arg}'.")
        endif()
        if(arg STREQUAL "general")
            continue()
        endif()
        if(TARGET "${arg}")
            get_target_property(targetType ${arg} TYPE)
            if("${targetType}" STREQUAL "OBJECT_LIBRARY")
                message(FATAL_ERROR "Target ${arg} is an OBJECT library. Use target_link_objlibraries(${target} ${arg}).")
            endif()
        endif()
        if(_scope MATCHES "^(PRIVATE|PUBLIC)$")
            # link dependencies
            set_property(TARGET ${target}
                         APPEND PROPERTY "LINK_LIBRARIES" ${arg})
            if(_scope STREQUAL "PRIVATE")
                # usage requirements
                get_property(propValue TARGET ${arg} PROPERTY "INTERFACE_LINK_LIBRARIES")
                set_property(TARGET ${target}
                    APPEND PROPERTY "INTERFACE_LINK_LIBRARIES" "\$<LINK_ONLY:${arg}>")
            endif()
        endif()
        if(_scope MATCHES "^(PUBLIC|INTERFACE)$")
            # link interface
            set_property(TARGET ${target}
                         APPEND PROPERTY "INTERFACE_LINK_LIBRARIES" ${arg})
        endif()
    endforeach()
endfunction()
