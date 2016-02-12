message(STATUS "#############################################")
message(STATUS "Configuring JoynrCommonAPI...")
message(STATUS "#############################################\n")

add_definitions(-DCOMMONAPI_INTERNAL_COMPILATION)

set(
    JoynrCommonAPI_PUBLIC_INCLUDE_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

set(
    JoynrCommonAPI_INCLUDE_DIRECTORIES
    ${JoynrCommonAPI_PUBLIC_INCLUDE_DIRECTORY}
)

include_directories(
    ${JoynrCommonAPI_INCLUDE_DIRECTORIES}
)

set(
    JoynrCommonAPI_TARGET_LIBRARIES
    ${DBUS1_LIBRARIES}
    ${COMMONAPI_LIBRARIES}
    ${COMMONAPI_DBUS_LIBRARIES}
)

file(
    GLOB_RECURSE JoynrCommonAPI_PRIVATE_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.h"
)

set(
    JoynrCommonAPI_HEADERS
    ${JoynrCommonAPI_PRIVATE_HEADERS}
)

file(
    GLOB_RECURSE JoynrCommonAPI_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp"
)

add_library(
    JoynrCommonAPI
    SHARED
    ${JoynrCommonAPI_HEADERS}
    ${JoynrCommonAPI_SOURCES}
)

AddClangFormat(
    JoynrCommonAPI
    ${JoynrCommonAPI_HEADERS}
    ${JoynrCommonAPI_SOURCES}
)

set_target_properties(
    JoynrCommonAPI
    PROPERTIES
    SOVERSION "${JOYNR_VERSION}"
    VERSION "${JOYNR_VERSION}"
    PUBLIC_INCLUDE_DIRECTORY "${JoynrCommonAPI_PUBLIC_INCLUDE_DIRECTORY}"
    # disable compiler warnings on CommonAPI generated code
    COMPILE_FLAGS "-Wno-effc++ -Wno-unused-parameter"
)

install(
    TARGETS JoynrCommonAPI
    EXPORT JoynrTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
)

# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

TARGET_LINK_LIBRARIES(JoynrCommonAPI ${JoynrCommonAPI_TARGET_LIBRARIES})
