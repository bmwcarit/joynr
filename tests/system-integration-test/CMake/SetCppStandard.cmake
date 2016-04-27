# The c++14 flag will be set automatically by CMake based on "CMAKE_CXX_STANDARD 14" starting with version 3.1.0.
# However, as CMake 3.1.x does not recognize compiler flags correctly for OS X, we use CMAKE_CXX_STANDARD
# from 3.2.0 upwards. For older versions, this flag is set manually for gcc and clang:
if("${CMAKE_VERSION}" VERSION_LESS 3.2.0)
    if( (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    endif( (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
else()
    # set required C++ standard
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif("${CMAKE_VERSION}" VERSION_LESS 3.2.0)
