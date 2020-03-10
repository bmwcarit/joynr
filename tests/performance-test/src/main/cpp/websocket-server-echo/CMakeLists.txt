cmake_minimum_required(VERSION 3.10)

option(
    USE_PLATFORM_WEBSOCKETPP
    "Resolve dependency to WebSocket++ from the system?"
    ON
)
message(STATUS "option USE_PLATFORM_WEBSOCKETPP=" ${USE_PLATFORM_WEBSOCKETPP})

### Add websocketpp ###########################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../cpp/CMake")
#include(AddWebSocketPP)
if(NOT TARGET websocketpp::websocketpp)
    find_package(websocketpp REQUIRED)
endif()

include(AddClangFormat)

find_package(Boost REQUIRED COMPONENTS system thread program_options)

add_executable(websocket-server-echo
    WebSocketServerEcho.cpp
)
add_dependencies(websocket-server-echo websocketpp)

target_include_directories(
    websocket-server-echo
    PUBLIC
        "$<BUILD_INTERFACE:${WEBSOCKETPP_INCLUDE_DIR}>"
)

target_link_libraries(
    websocket-server-echo
    ${Boost_LIBRARIES}
)

AddClangFormat(websocket-server-echo)
