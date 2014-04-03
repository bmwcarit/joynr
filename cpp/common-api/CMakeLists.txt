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
    ${PATCHED_DBUS1_LIBRARIES}
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
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
)

# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY include/
    DESTINATION "${INSTALL_INCLUDE_DIR}"
    FILES_MATCHING PATTERN "*.h"
)

TARGET_LINK_LIBRARIES(JoynrCommonAPI ${JoynrCommonAPI_TARGET_LIBRARIES})
