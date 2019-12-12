project(HttpNetworking)

set(SOURCES
    CurlHandlePool.cpp
    CurlHandlePool.h
    DefaultHttpRequest.cpp
    DefaultHttpRequest.h
    HttpNetworking.cpp
    HttpRequestBuilder.cpp
    HttpRequestBuilder.h
    HttpResult.cpp

    # These are kind of public interface
    HttpNetworking.h
    HttpResult.h
)

add_library(${PROJECT_NAME} OBJECT ${SOURCES})
add_library(JoynrCCLib::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    PUBLIC "$<BUILD_INTERFACE:${EXPORT_HEADER_INCLUDE_DIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC Joynr::JoynrLib
    PRIVATE CURL::libcurl
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
