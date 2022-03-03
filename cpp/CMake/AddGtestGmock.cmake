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

    # this repository contains both googletest and googlemock
    AddExternalProject(
        googletest-repo
        DOWNLOAD_DIR "googletest-repo"
        TIMEOUT 10
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG ddb8012e # there is no version tag on github; this is to make sure that a working version is used
        CONFIGURE_COMMAND "" # Disable configuring
        BUILD_COMMAND "" # Disable building
    )

    get_property(EP_PREFIX DIRECTORY PROPERTY EP_PREFIX)

    ### Add google test ###########################################################

    # build googletest library
    AddExternalProject(
        googletest
        DOWNLOAD_COMMAND "" # already downloaded by googletest-repo
        SOURCE_DIR "${EP_PREFIX}/src/googletest-repo/googletest"
        BUILD_BYPRODUCTS "${EP_PREFIX}/src/googletest-build/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
        CMAKE_ARGS -Dgtest_force_shared_crt=ON
                   -Dgtest_disable_pthreads=${DISABLE_PTHREAD_CMAKE_ARG}
                   -DCMAKE_POSITION_INDEPENDENT_CODE=ON
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
    set(GTEST_LIBRARIES ${googletest_binary_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})

    ### Add google mock ###########################################################

    # build googlemock library
    AddExternalProject(
        googlemock
        DOWNLOAD_COMMAND "" # already downloaded by googletest-repo
        SOURCE_DIR "${EP_PREFIX}/src/googletest-repo/googlemock"
        BUILD_BYPRODUCTS "${EP_PREFIX}/src/googlemock-build/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX}"
        CMAKE_ARGS -Dgtest_force_shared_crt=ON
                   -Dgtest_disable_pthreads=${DISABLE_PTHREAD_CMAKE_ARG}
                   -DCMAKE_POSITION_INDEPENDENT_CODE=ON
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
    set(GMOCK_LIBRARIES ${googlemock_binary_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX})
endif(USE_PLATFORM_GTEST_GMOCK)

function(RegisterToCtest TARGET ${ARGN})
    # required function parameters
    set(PARAMS "INCLUDES;LIBRARIES;SOURCES")
    cmake_parse_arguments(JOYNR "" "" "${PARAMS}" ${ARGN})

    add_executable(${TARGET} ${JOYNR_SOURCES})

    set(output_directory "${CMAKE_BINARY_DIR}/bin")
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${output_directory}
    )

    target_link_libraries(${TARGET} PRIVATE
        ${JOYNR_LIBRARIES}
    )

    target_include_directories(
        ${TARGET}
        PRIVATE ${JOYNR_INCLUDES}
    )

    gtest_discover_tests(${TARGET}
        WORKING_DIRECTORY ${output_directory}
    )
endfunction(RegisterToCtest)

function(AddTest TARGET)
    add_executable(
        ${TARGET}
        ${ARGN}
    )
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
