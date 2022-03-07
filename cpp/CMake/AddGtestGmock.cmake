option(
    USE_PLATFORM_GTEST_GMOCK
    "Resolve dependency to gtest and gmock from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_GTEST_GMOCK=" ${USE_PLATFORM_GTEST_GMOCK})

include(AddExternalProject)

if (USE_PLATFORM_GTEST_GMOCK)

    find_package(GTest REQUIRED)
    find_package(GMock REQUIRED)

else (USE_PLATFORM_GTEST_GMOCK)
    ### clone googletest git repository ###########################################################

    # Version is provided in the top level CMakeList.txt in googletest-repo;
    # however since we do not use it, but build from subdirectory CMakeList.txt files
    # we have to provide this ourselves
    set(GOOGLETEST_VERSION 1.11.0)
    set(DISABLE_PTHREAD_CMAKE_ARG OFF)

    # force detection of compiler
    # this is required if this file is included from e.g. inter-language-test
    # since otherwise no compiler is available for the cmake invoked through the
    # AddExternalProject specification.
    include(CMakeDetermineCCompiler)

    get_property(EP_PREFIX DIRECTORY PROPERTY EP_PREFIX)

    # Note that CMAKE_CURRENT_LIST_DIR contains the directory where this file is stored when it
    # gets processed by a cmake include command (contrary to the location of the file that contains
    # the include command or the current working directory at the time where cmake or make is called).
    # This info is used to determine a relative path to the thirdparty directory where googletest
    # is stored.

    # this repository contains both googletest and googlemock
    AddExternalProject(
        googletest-repo
        DOWNLOAD_DIR "googletest-repo"
        URL "file://${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/testing/googletest"
        CONFIGURE_COMMAND "" # Disable configuring
        BUILD_COMMAND "" # Disable building
    )

    ### Add google test ###########################################################

    # build googletest library
    AddExternalProject(
        googletest
        DOWNLOAD_COMMAND "" # already downloaded by googletest-repo
        SOURCE_DIR "${EP_PREFIX}/src/googletest-repo/googletest"
        BUILD_BYPRODUCTS "${EP_PREFIX}/src/googletest-build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
        CMAKE_ARGS -Dgtest_force_shared_crt=ON
                   -Dgtest_disable_pthreads=${DISABLE_PTHREAD_CMAKE_ARG}
                   -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                   -DGOOGLETEST_VERSION=${GOOGLETEST_VERSION}
    )

    ExternalProject_Get_Property(googletest binary_dir)
    set(googletest_binary_dir ${binary_dir})
    message(STATUS "variable googletest_binary_dir=${googletest_binary_dir}")

    # build googletest AFTER downloading the sources
    add_dependencies(googletest googletest-repo)

    # Specify include dir
    ExternalProject_Get_Property(googletest source_dir)
    set(googletest_source_dir ${source_dir})
    message(STATUS "variable googletest_source_dir=${googletest_source_dir}")
    set(GTEST_INCLUDE_DIRS ${googletest_source_dir}/include)
    set(GTEST_LIBRARIES ${googletest_binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})

    add_library(gtestLib STATIC IMPORTED)
    add_dependencies(gtestLib googletest)
    set_target_properties(gtestLib
        PROPERTIES IMPORTED_LOCATION ${GTEST_LIBRARIES}
    )

    ### Add google mock ###########################################################

    # build googlemock library
    AddExternalProject(
        googlemock
        DOWNLOAD_COMMAND "" # already downloaded by googletest-repo
        SOURCE_DIR "${EP_PREFIX}/src/googletest-repo/googlemock"
        BUILD_BYPRODUCTS "${EP_PREFIX}/src/googlemock-build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX}"
        CMAKE_ARGS -Dgtest_force_shared_crt=ON
                   -Dgtest_disable_pthreads=${DISABLE_PTHREAD_CMAKE_ARG}
                   -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                   -DGOOGLETEST_VERSION=${GOOGLETEST_VERSION}
    )

    ExternalProject_Get_Property(googlemock binary_dir)
    set(googlemock_binary_dir ${binary_dir})
    message(STATUS "variable googlemock_binary_dir=${googlemock_binary_dir}")

    # build googlemock AFTER downloading the sources
    add_dependencies(googlemock googletest-repo)

    # Specify include dir
    ExternalProject_Get_Property(googlemock source_dir)
    set(googlemock_source_dir ${source_dir})
    message(STATUS "variable googlemock_source_dir=${googlemock_source_dir}")
    set(GMOCK_INCLUDE_DIRS ${googlemock_source_dir}/include)
    set(GMOCK_LIBRARIES ${googlemock_binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX})

    add_library(gmockLib STATIC IMPORTED)
    add_dependencies(gmockLib googlemock)
    set_target_properties(gmockLib
        PROPERTIES IMPORTED_LOCATION ${GMOCK_LIBRARIES}
    )

    # required to get gtest_discover_tests
    include(GoogleTest REQUIRED)

endif(USE_PLATFORM_GTEST_GMOCK)

function(AddTest TARGET)
    add_executable(
        ${TARGET}
        ${ARGN}
    )
#    set_target_properties(
#        ${TARGET}
#        PROPERTIES
#        COMPILE_FLAGS "-Wno-effc++ -Wno-unused-parameter"
#    )
    if(NOT USE_PLATFORM_GTEST_GMOCK)
        add_dependencies(${TARGET} googletest)
        add_dependencies(${TARGET} googlemock)
    endif(NOT USE_PLATFORM_GTEST_GMOCK)

    target_include_directories(
        ${TARGET}
        SYSTEM
        PRIVATE
        ${GTEST_INCLUDE_DIRS}
        ${GMOCK_INCLUDE_DIRS}
        ${MOSQUITTO_INCLUDE_DIRS}
    )
endfunction(AddTest)
