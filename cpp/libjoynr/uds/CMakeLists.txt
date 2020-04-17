project(Uds)

set(SOURCES
    UdsLibJoynrMessagingSkeleton.cpp
    UdsMessagingStub.cpp
    # UdsMessagingStubFactory.cpp
    # UdsClient.cpp
    UdsSettings.cpp
)

set(PRIVATE_HEADERS
    # IUdsClient.h
    UdsLibJoynrMessagingSkeleton.h
    UdsMessagingStub.h
    # UdsMessagingStubFactory.h
    # UdsClient.h
    # UdsReceiver.h
    # UdsSender.h
)

set(PUBLIC_HEADERS
    include/joynr/UdsSettings.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    #PRIVATE udspp::udspp
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
