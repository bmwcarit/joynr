project(BaseModelGenerated)

file(GLOB_RECURSE GENERATED_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/joynr/infrastructure/*.h"
    "include/joynr/system/*.h"
    "include/joynr/types/*.h"
)

file(GLOB_RECURSE GENERATED_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "communication-model/generated/*.cpp"
    "joynr-messaging/generated/*.cpp"
    "provider/generated/*.cpp"
    "proxy/generated/*.cpp"
)

add_library(${PROJECT_NAME} OBJECT
    ${GENERATED_HEADERS} ${GENERATED_SOURCES}
)
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    # Unfortunately we have circular include dependencies.
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::Interface
    PUBLIC Joynr::Concurrency
)

install(
    DIRECTORY include/
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)
