project(WebSocket)

set(SOURCES
    WebSocketLibJoynrMessagingSkeleton.cpp
    WebSocketMessagingStub.cpp
    WebSocketMessagingStubFactory.cpp
    WebSocketPpClientTLS.cpp
    WebSocketSettings.cpp
)

set(PRIVATE_HEADERS
    IWebSocketPpClient.h
    WebSocketLibJoynrMessagingSkeleton.h
    WebSocketMessagingStub.h
    WebSocketMessagingStubFactory.h
    WebSocketPpClient.h
    WebSocketPpClientNonTLS.h
    WebSocketPpClientTLS.h
    WebSocketPpReceiver.h
    WebSocketPpSender.h
)

set(PUBLIC_HEADERS
    include/joynr/WebSocketSettings.h
    include/joynr/SingleThreadedIOService.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${PRIVATE_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
    PRIVATE "$<TARGET_PROPERTY:websocketpp::websocketpp,INTERFACE_INCLUDE_DIRECTORIES>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PRIVATE MoCOCrW::mococrw
)
target_link_objlibraries(${PROJECT_NAME}
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
