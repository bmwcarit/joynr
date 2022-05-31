#[rst
# ThreadSanitizerTarget
# =====================
#
# Sets up an ``tsan`` target that automates executing tests with
# `ThreadSanitizer <https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual>`_
# (TSAN) - Data race detector for C/C++ .
#
# Usage
# -----
#
# Add the following lines to your project's ``CMakeLists.txt``:
#
# .. code-block:: cmake
#
#  if(CMAKE_BUILD_TYPE STREQUAL TSAN)
#      include(ThreadSanitizerTarget)
#  endif()
#
# Then execute CMake with:
#
# .. code-block:: sh
#
#  CXX=clang++ cmake -DCMAKE_BUILD_TYPE=TSAN $SOURCE_DIR
#
# and generate the TSAN report for CTest based tests with:
#
# .. code-block:: sh
#
#  cmake --build . --target tsan
#
# If necessary CTest parameters can be passed in the ARGS env variable:
#
# .. code-block:: sh
#
#  ARGS="-VV --repeat-until-fail 10" cmake --build . --target tsan
#
# Configuration
# -------------
#
# This module reads the following configuration variables:
#
# ``TSAN_OPTIONS``
#  `Run-time flags <https://github.com/google/sanitizers/wiki/ThreadSanitizerFlags#thread-sanitizer-flags>`__
#  for the ThreadSanitizer.
#
#
#]rst

if(NOT CMAKE_BUILD_TYPE STREQUAL TSAN)
    message(FATAL_ERROR "ThreadSanitizerTarget.cmake requires CMake to be "
                        "called with -DCMAKE_BUILD_TYPE=TSAN")
endif()

# TSAN build type
set(_TSAN_FLAGS "-fsanitize=thread -fno-omit-frame-pointer")
set(_TSAN_FLAGS "${_TSAN_FLAGS} -g -O2")
set(CMAKE_CXX_FLAGS_TSAN ${_TSAN_FLAGS} CACHE STRING
    "Flags used by the C++ compiler during TSAN builds." FORCE
)
set(CMAKE_C_FLAGS_TSAN ${_TSAN_FLAGS} CACHE STRING
    "Flags used by the C compiler during TSAN builds." FORCE
)
set(CMAKE_EXE_LINKER_FLAGS_TSAN ${_TSAN_FLAGS} CACHE STRING
    "Flags used for linking binaries during TSAN builds." FORCE
)
set(CMAKE_SHARED_LINKER_FLAGS_TSAN ${_TSAN_FLAGS} CACHE STRING
    "Flags used for linking shared libraries during TSAN builds." FORCE
)
mark_as_advanced(
    CMAKE_CXX_FLAGS_TSAN CMAKE_C_FLAGS_TSAN CMAKE_EXE_LINKER_FLAGS_TSAN
    CMAKE_SHARED_LINKER_FLAGS_TSAN CMAKE_STATIC_LINKER_FLAGS_TSAN
)
unset(_TSAN_FLAGS)

# Add tsan target.
add_custom_target(tsan
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}

    # working escaping for make: \${ARGS} \$\${ARGS}
    # working escaping for ninja: \$\${ARGS}
    # No luck with VERBATIM option.
    #
    COMMAND ${CMAKE_COMMAND} -E env
                TSAN_OPTIONS=${TSAN_OPTIONS}
            ${CMAKE_CTEST_COMMAND} --output-on-failure "\$\${ARGS}"

    COMMENT "Generate TSAN report"
    USES_TERMINAL # Ensure ninja outputs to stdout.
)
