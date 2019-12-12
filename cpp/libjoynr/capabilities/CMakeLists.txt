project(Capabilities)

set(SOURCES
    CapabilitiesRegistrar.cpp
    CapabilityUtils.cpp
    LocalDiscoveryAggregator.cpp
    ParticipantIdStorage.cpp
)

set(PUBLIC_HEADERS
    include/joynr/CapabilitiesRegistrar.h
    include/joynr/CapabilityUtils.h
    include/joynr/ILocalCapabilitiesCallback.h
    include/joynr/LocalDiscoveryAggregator.h
    include/joynr/ParticipantIdStorage.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::BaseModel
)

install(
    DIRECTORY include/
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
