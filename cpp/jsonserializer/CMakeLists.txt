message(STATUS "#############################################")
message(STATUS "Configuring JoynrJsonSerializer...")
message(STATUS "#############################################\n")

### Add jsmn ###########################################################
if(USE_PLATFORM_JSMN)
    find_package(jsmn REQUIRED)
else(USE_PLATFORM_JSMN)
    set(jsmn_LIBRARY "${CMAKE_BINARY_DIR}/ThirdParty/src/jsmn/libjsmn.a")
    AddExternalProject(
        jsmn
        GIT_REPOSITORY https://github.com/zserge/jsmn
        TIMEOUT 10
        PATCH_COMMAND rm -f config.mk COMMAND git apply ${CMAKE_SOURCE_DIR}/CMake/jsmn-fpic.patch
        # Disable configure step
        CONFIGURE_COMMAND ""
        # Disable build step
        BUILD_COMMAND "make"
        BUILD_IN_SOURCE 1
        BUILD_BYPRODUCTS ${jsmn_LIBRARY}
    )

    ExternalProject_Get_Property(jsmn SOURCE_DIR)
    set(jsmn_INCLUDE_DIR ${SOURCE_DIR})

    set_target_properties(
        jsmn
        PROPERTIES
        PUBLIC_INCLUDE_DIRECTORY "${jsmn_INCLUDE_DIR}"
    )
endif(USE_PLATFORM_JSMN)

set(
    jsmn_INCLUDE_DIR
    "${jsmn_INCLUDE_DIR}"
    PARENT_SCOPE
)

get_target_property(JoynrCommon_PUBLIC_INCLUDE_DIRECTORY JoynrCommon PUBLIC_INCLUDE_DIRECTORY)

set(
    JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${jsmn_INCLUDE_DIR}
)

set(
    JoynrJsonSerializer_INCLUDE_DIRECTORIES
    ${JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY}
    "../"
    ${JoynrCommon_PUBLIC_INCLUDE_DIRECTORY}
    ${jsmn_INCLUDE_DIR}
)

file(
    GLOB_RECURSE JoynrJsonSerializer_PUBLIC_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/joynr/*.h"
)

file(
    GLOB JoynrJsonSerializer_PRIVATE_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.h"
    "exceptions/*.h"
)

set(
    JoynrJsonSerializer_HEADERS
    ${JoynrJsonSerializer_PUBLIC_HEADERS}
    ${JoynrJsonSerializer_PRIVATE_HEADERS}
)

file(
    GLOB_RECURSE JoynrJsonSerializer_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp"
)

add_library(
    JoynrJsonSerializer SHARED
    ${JoynrJsonSerializer_HEADERS}
    ${JoynrJsonSerializer_SOURCES}
)

include_directories(
    ${JoynrJsonSerializer_INCLUDE_DIRECTORIES}
)

AddIncludeWhatYouUse(JoynrJsonSerializer)

target_link_libraries(
    JoynrJsonSerializer
    JoynrCommon
)

target_link_libraries(
    JoynrJsonSerializer
    PRIVATE ${jsmn_LIBRARY}
)

set_target_properties(
    JoynrJsonSerializer
    PROPERTIES
    SOVERSION "${JOYNR_VERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrJsonSerializer_PUBLIC_INCLUDE_DIRECTORY}"
)

add_dependencies(JoynrJsonSerializer jsmn)

install(
    TARGETS JoynrJsonSerializer EXPORT JoynrTargets
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT library
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
    # this will flatten directory hierarchy of headder files
    # PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT development
)
# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${INSTALL_INCLUDE_DIR}"
    FILES_MATCHING PATTERN "*.h"
)
