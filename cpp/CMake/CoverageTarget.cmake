#[rst
# CoverageTarget
# ==============
#
# Sets up a ``coverage`` target that automates the generation of a code
# coverage HTML report for C/C++. When compiling with GCC, this module
# uses ``lcov`` and ``htmlgen``. When compiling with Clang then `llvm-cov`
# is used instead. Due to the performance gain, generating coverage with
# Clang is highly recommended.
#
# Usage
# -----
#
# Add the following lines to your project's ``CMakeLists.txt``:
#
# .. code-block:: cmake
#
#  if(CMAKE_BUILD_TYPE STREQUAL Coverage)
#      include(CoverageTarget)
#  endif()
#
# Then execute CMake with:
#
# .. code-block:: sh
#
#  cmake -DCMAKE_BUILD_TYPE=Coverage $SOURCE_DIR
#
# and generate the coverage report for CTest based tests with:
#
# .. code-block:: sh
#
#  cmake --build . --target coverage
#
# If necessary CTest parameters can be passed in the ARGS env variable:
#
# .. code-block:: sh
#
#  ARGS="-VV -L unit" cmake --build . --target coverage
#
# If Clang is used for compilation a coverage report can be generated based
# on the currently generated instrumentation files:
#
# .. code-block: sh
#
#  ctest -R TestFilter
#  cmake --build . --target build_coverage_report
#
# Configuration
# -------------
#
# This module reads the following configuration variables:
#
# ``COVERAGE_DIR``
#  Working directory where output is generated to.
#
# ``COVERAGE_BASE_DIR``
#  Coverage base directory (defaults to ``${CMAKE_SOURCE_DIR}``).
#
# ``COVERAGE_EXCLUDES``
#  List of additional exclude patterns.
#  Some common patterns are excluded automatically.
#
# ``COVERAGE_EXTRACTS``
#  List of extract patterns. (LCOV only)
#
# ``COVERAGE_BRANCH_COVERAGE``
#  Generate branch coverage. (LCOV only, always enabled for LLVM-COV)
#
# ``COVERAGE_THRESHOLD_LINE``
#  Minimal line coverage in percent.
#  A lower coverage results in build failure.
#
# ``COVERAGE_THRESHOLD_FUNCTION``
#  Minimal function coverage in percent.
#  A lower coverage results in build failure.
#
# ``COVERAGE_THRESHOLD_BRANCH``
#  Minimal branch coverage in percent.
#  A lower coverage results in build failure.
#
# ``COVERAGE_THRESHOLD_REGION``
#  Minimal region coverage in percent. (LLVM-COV only)
#  A lower coverage results in build failure.
#]rst

include(CommonCMakeUtils)
include(FindPackageHandleStandardArgs)

# Set default directories.
if(NOT COVERAGE_DIR)
    set(COVERAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/coverage)
endif()

if(NOT COVERAGE_BASE_DIR)
    set(COVERAGE_BASE_DIR ${CMAKE_SOURCE_DIR})
endif()

if(NOT COVERAGE_EXCLUDES)
    set(COVERAGE_EXCLUDES)
endif()
set(_coverage_excludes ${COVERAGE_EXCLUDES} mocs_*.cpp moc_*.cpp *.moc */src-gen/*)

if(COVERAGE_BRANCH_COVERAGE)
    set(_coverage_enable_branch 1)
else()
    set(_coverage_enable_branch 0)
endif()

if(NOT COVERAGE_THRESHOLD_LINE)
    set(COVERAGE_THRESHOLD_LINE 0)
endif()
if(NOT COVERAGE_THRESHOLD_FUNCTION)
    set(COVERAGE_THRESHOLD_FUNCTION 0)
endif()
if(NOT COVERAGE_THRESHOLD_REGION)
    set(COVERAGE_THRESHOLD_REGION 0)
endif()
if(NOT COVERAGE_THRESHOLD_BRANCH)
    set(COVERAGE_THRESHOLD_BRANCH 0)
endif()

get_filename_component(_coverage_script_folder ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
set(_coverage_script_folder ${CMAKE_CURRENT_LIST_DIR}/${_coverage_script_folder})

# Include the compiler dependent coverage target definition.
if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    include(${_coverage_script_folder}/CoverageTargetLLVMCOV.cmake)
else()
    include(${_coverage_script_folder}/CoverageTargetLCOV.cmake)
endif()

unset(_coverage_enable_branch)
unset(_coverage_base_dirs)
unset(_coverage_excludes)
unset(_coverage_extracts)
unset(_coverage_script_folder)
