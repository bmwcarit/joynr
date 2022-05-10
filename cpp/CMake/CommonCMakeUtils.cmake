#
# Copyright BMW Group, 2018-2022
#
#[rst
# CommonCMakeUtils
# ================
#
# Collection of miscellaneous functions to simplify CMake usage.
#
# Usage
# -----
#
# .. code-block:: cmake
#
#  include(CommonCMakeUtils)
#  #...
#  joynr_list_join(...)
#
#]rst

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
include_guard()

#[rst
#
# joynr_list_join()
# ---------------
#
# .. code-block:: cmake
#
#  joynr_list_join(
#    <output_variable>
#    <glue>
#    <list>
#  )
#
# Returns a string joining all ``list`` elements using the ``glue`` string.
#
# .. deprecated:: CMake-3.12
#  Use `list(JOIN) <https://cmake.org/cmake/help/v3.12/command/list.html#reading>`_
#  instead.
#
#]rst
function(joynr_list_join OUTPUT_VAR GLUE)
    string(REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" RESULT "${ARGN}")
    string(REGEX REPLACE "[\\](.)" "\\1" RESULT "${RESULT}")
    set(${OUTPUT_VAR} "${RESULT}" PARENT_SCOPE)
endfunction()

#[rst
#
# joynr_list_append_each()
# -----------------------
#
# .. code-block:: cmake
#
#  joynr_list_append_each(
#    <output_variable>
#    <append>
#    <list>
#  )
#
# Returns a list where ``append`` is inserted at the beginning of each
# ``list`` element.
#
# .. deprecated:: CMake-3.12
#  Use `list(TRANSFORM APPEND) <https://cmake.org/cmake/help/v3.12/command/list.html#modification>`_
#  instead.
#
#]rst
function(joynr_list_append_each OUTPUT_VAR APPEND)
    string(REGEX REPLACE "([^;]+)" "\\1${APPEND}" RESULT "${ARGN}")
    set(${OUTPUT_VAR} "${RESULT}" PARENT_SCOPE)
endfunction()

#[rst
#
# joynr_list_prepend_each()
# -----------------------
#
# .. code-block:: cmake
#
#  joynr_list_prepend_each(
#    <output_variable>
#    <prepend>
#    <list>
#  )
#
# Returns a list where ``prepend`` is inserted at the beginning of each
# ``list`` element.
#
# .. deprecated:: CMake-3.12
#  Use `list(TRANSFORM PREPEND) <https://cmake.org/cmake/help/v3.12/command/list.html#modification>`_
#  instead.
#
#]rst
function(joynr_list_prepend_each OUTPUT_VAR PREPEND)
    string(REGEX REPLACE "([^;]+)" "${PREPEND}\\1" RESULT "${ARGN}")
    set(${OUTPUT_VAR} "${RESULT}" PARENT_SCOPE)
endfunction()

#[rst
#
# joynr_fail_on_unrecognized_arguments()
# ------------------------------------
#
# .. code-block:: cmake
#
#  joynr_fail_on_unrecognized_arguments()
#
# Fails with a meaningful ``FATAL_ERROR`` if the last invocation of ``cmake_parse_arguments()``
# reported that some unparsed / unrecognized params were given.
#
# Example
# ^^^^^^^
#
# .. code-block:: cmake
#
#  cmake_parse_arguments(ARG "..." "..." "..." ${ARGN})
#  joynr_fail_on_unrecognized_arguments()
#
# .. note:: Requires that the first argument given to the prior invocation of ``cmake_parse_arguments()`` was ``ARG``.
#
#]rst
macro(joynr_fail_on_unrecognized_arguments)
    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unrecognized arguments given ('${ARG_UNPARSED_ARGUMENTS}')!")
    endif()
endmacro()

#[rst
#
# joynr_parse_arguments()
# ---------------------
#
# .. code-block:: cmake
#
#  joynr_parse_arguments(${ARGN}
#                      [OPTION_ARGS <option> [<option> ...]]
#                      [SINGLEVALUE_ARGS <keyword> [<keyword> ...]]
#                      [MULTIVALUE_ARGS <keyword> [<keyword> ...]]
#  )
#
# Convenience wrapper around ``cmake_parse_arguments()`` that provides a more explicit syntax and checks for unknown
# arguments. It also provides specific support for the common use case where options are forwarded to other CMake
# functions as a string.
#
# Option and keyword variables are prefixed with ``ARG_``. While single- and multi-value keyword variables contain the
# given values, option variables are modified so they contain their own name (without the prefix) as a string if set,
# and are otherwise empty. This means that one can expand them to pass them to other functions that expect a string.
# This differs from the built-in ``cmake_parse_arguments()``, which sets option variables to ``TRUE`` or ``FALSE``
# respectively, requiring extra handling for passing them on. One can still use those variables in an ``if`` condition
# as usual, as an empty string is interpreted as ``FALSE``, and any string that isn't a 'false constant' is interpreted
# as ``TRUE``.
#
# .. note:: The first argument given to ``joynr_parse_arguments()`` should be ``${ARGN}``, i.e. the list of arguments
#           to be parsed. ``${ARGN}`` must not contain ``OPTION_ARGS``, ``SINGLEVALUE_ARGS``, or ``MULTIVALUE_ARGS``,
#           otherwise parsing is confused.
#
# Example
# ^^^^^^^
#
# .. code-block:: cmake
#
#  function(my_function)
#      joynr_parse_arguments(${ARGN}
#          OPTION_ARGS THISOPTION THATOPTION
#          MULTIVALUE_ARGS VALUES
#      )
#      message(STATUS "THISOPTION=${ARG_THISOPTION} THATOPTION=${ARG_THATOPTION} VALUES=${ARG_VALUES}")
#
#      other_function(VALUES ${ARG_VALUES} ${ARG_THISOPTION} ${ARG_THATOPTION})
#  endfunction()
#
#  my_function(THATOPTION VALUES val1 val2)
#
# This prints ``THISOPTION= THATOPTION=THATOPTION VALUES=val1val2``, and calls ``other_function``
# with ``VALUES "val1;val2" THATOPTION``.
#
#]rst
function(joynr_parse_arguments)
    # Parse arguments for this function, defining the keywords to be used for second stage parsing
    cmake_parse_arguments(MPA "" "" "OPTION_ARGS;SINGLEVALUE_ARGS;MULTIVALUE_ARGS" ${ARGN})

    # MPA_UNPARSED_ARGUMENTS now contains the arguments that should be forwarded to second stage parsing
    cmake_parse_arguments(ARG "${MPA_OPTION_ARGS}" "${MPA_SINGLEVALUE_ARGS}" "${MPA_MULTIVALUE_ARGS}" ${MPA_UNPARSED_ARGUMENTS})
    joynr_fail_on_unrecognized_arguments()

    # Sanity check
    if(NOT MPA_OPTION_ARGS AND NOT MPA_SINGLEVALUE_ARGS AND NOT MPA_MULTIVALUE_ARGS)
        message(FATAL_ERROR "At least one of OPTION_ARGS, SINGLEVALUE_ARGS, or MULTIVALUE_ARGS must be given!")
    endif()

    # Rewrite option variables so they contain their name as a string, if set. This allows for expanding these variables
    # when passing them to subsequent calls to other functions.
    foreach(arg ${MPA_OPTION_ARGS})
        if(ARG_${arg})
            set(ARG_${arg} "${arg}")
        else()
            set(ARG_${arg} "")
        endif()
    endforeach()

    # Set all argument variables in the caller's scope
    foreach(arg ${MPA_OPTION_ARGS} ${MPA_SINGLEVALUE_ARGS} ${MPA_MULTIVALUE_ARGS})
        if(DEFINED ARG_${arg})
            set(ARG_${arg} "${ARG_${arg}}" PARENT_SCOPE)
        else()
            unset(ARG_${arg} PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

#[rst
#
# joynr_set_if_defined()
# --------------------
#
# .. code-block:: cmake
#
#  joynr_set_if_defined(
#    <variable>
#    <value_var>
#    <default_value>
#  )
#
# Sets ``variable`` to the value of ``value_var`` *iff* ``value_var`` is defined or to
# ``default_value`` otherwise. Useful when a default value can be overridden by params
# given to a function.
#
# Example
# ^^^^^^^
#
# .. code-block:: cmake
#
#  joynr_set_if_defined(_separator ARG_SEPARATOR "-")
#  # ... instead of ...
#  set(_separator "-")
#  if(DEFINED ARG_SEPARATOR)
#    set(_separator ${ARG_SEPARATOR})
#  endif()
#
#]rst
macro(joynr_set_if_defined variable value_var default_value)
    if (DEFINED ${value_var})
        set(${variable} ${${value_var}})
    else()
        set(${variable} ${default_value})
    endif()
endmacro()
