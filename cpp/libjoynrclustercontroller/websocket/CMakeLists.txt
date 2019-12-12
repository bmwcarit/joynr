project(CcWebsocket)

set(SOURCES
    WebSocketCcMessagingSkeleton.h
    WebSocketCcMessagingSkeletonNonTLS.h
    WebSocketCcMessagingSkeletonTLS.cpp
    WebSocketCcMessagingSkeletonTLS.h
)

add_library(${PROJECT_NAME} OBJECT ${SOURCES})
add_library(JoynrCCLib::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    # Not good... Needed by WebSocketCcMessagingSkeleton.h and used also by JoynrClusterControllerRuntime
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../libjoynr/websocket>"
    # common
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
    PUBLIC "$<BUILD_INTERFACE:${EXPORT_HEADER_INCLUDE_DIR}>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC Joynr::JoynrLib
    PRIVATE MoCOCrW::mococrw
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
