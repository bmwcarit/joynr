cmake_minimum_required(VERSION 3.10)

project(websocket-client-EchoString)

option(
    USE_PLATFORM_WEBSOCKETPP
    "Resolve dependency to WebSocket++ from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_WEBSOCKETPP=" ${USE_PLATFORM_WEBSOCKETPP})

### Add websocketpp ###########################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../cpp/CMake")
if(NOT TARGET websocketpp::websocketpp)
    find_package(websocketpp REQUIRED)
endif()

include(AddClangFormat)

find_package(Boost 1.65.0 REQUIRED COMPONENTS system thread program_options)
find_package(OpenSSL REQUIRED)
find_package (Threads)

add_executable(websocket-client-echo
    WebSocketClientEcho.cpp
)
add_dependencies(websocket-client-echo websocketpp::websocketpp)

target_include_directories(
    websocket-client-echo
    PUBLIC
        "$<BUILD_INTERFACE:${WEBSOCKETPP_INCLUDE_DIR}>"
)

target_link_libraries(
    websocket-client-echo
    ${OPENSSL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
)

AddClangFormat(websocket-client-echo)
