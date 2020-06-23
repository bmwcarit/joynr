project(Uds)

set(SOURCES
    UdsClient.cpp
    UdsFrameBufferV1.cpp
    UdsLibJoynrMessagingSkeleton.cpp
    UdsMessagingStub.cpp
    UdsMessagingStubFactory.cpp
    UdsServer.cpp
    UdsSettings.cpp
)

set(PRIVATE_HEADERS
    UdsFrameBufferV1.h
    UdsLibJoynrMessagingSkeleton.h
    UdsMessagingStub.h
    UdsMessagingStubFactory.h
    UdsSendQueue.h
)

set(PUBLIC_HEADERS
    include/joynr/UdsClient.h
    include/joynr/UdsServer.h
    include/joynr/UdsSettings.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)

target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::Interface
    PUBLIC Joynr::Messaging
)

install(
    DIRECTORY include/
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
