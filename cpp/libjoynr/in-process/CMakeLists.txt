project(InProcessMessaging)

set(SOURCES
    InProcessMessagingAddress.cpp
    InProcessMessagingSkeleton.cpp
    InProcessMessagingStub.cpp
    InProcessMessagingStubFactory.cpp
)

set(PRIVATE_HEADERS
    InProcessMessagingSkeleton.h
    InProcessMessagingStub.h
    InProcessMessagingStubFactory.h
)

add_library(${PROJECT_NAME} OBJECT ${PRIVATE_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::Messaging
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
