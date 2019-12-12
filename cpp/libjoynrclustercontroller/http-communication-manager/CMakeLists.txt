project(HttpCommunicationManager)

set(SOURCES
    HttpMessagingSkeleton.cpp
    HttpMessagingSkeleton.h
    HttpReceiver.cpp
    HttpReceiver.h
    HttpSender.cpp
    HttpSender.h
    LongPollingMessageReceiver.cpp
    LongPollingMessageReceiver.h
)

add_library(${PROJECT_NAME} OBJECT ${SOURCES})
add_library(JoynrCCLib::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    # common
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
    PUBLIC "$<BUILD_INTERFACE:${EXPORT_HEADER_INCLUDE_DIR}>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC Joynr::JoynrLib
)
target_link_objlibraries(${PROJECT_NAME}
    PRIVATE JoynrCCLib::HttpNetworking
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
