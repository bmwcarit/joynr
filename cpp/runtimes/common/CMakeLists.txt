project(RuntimeCommon)

# This is used also in JoynrRuntime.h
set(JoynrClusterControllerRuntime_EXPORT_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/include/joynr/JoynrClusterControllerRuntimeExport.h"
)

set(SOURCES
    JoynrRuntimeImpl.cpp
)

set(PUBLIC_HEADERS
    "${JoynrClusterControllerRuntime_EXPORT_HEADER}"

    include/joynr/JoynrRuntime.h
    include/joynr/JoynrRuntimeImpl.h
)

add_library(${PROJECT_NAME} OBJECT)
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
    ${PUBLIC_HEADERS}
    ${SOURCES}
)
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC Joynr::JoynrLib
)

generate_export_header(
    ${PROJECT_NAME}
    EXPORT_FILE_NAME "${JoynrClusterControllerRuntime_EXPORT_HEADER}"
    BASE_NAME JoynrClusterControllerRuntime
)

install(
    DIRECTORY include/ "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
