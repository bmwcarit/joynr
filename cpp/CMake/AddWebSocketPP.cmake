if(USE_PLATFORM_WEBSOCKETPP)
    find_package(websocketpp REQUIRED)
else(USE_PLATFORM_WEBSOCKETPP)
    include(AddExternalProject)
    AddExternalProject(
        websocketpp
        GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
        GIT_TAG 0.8.1
        TIMEOUT 10
        # Disable configure step
        CONFIGURE_COMMAND ""
        # Disable build step
        BUILD_COMMAND ""
    )
    ExternalProject_Get_Property(websocketpp SOURCE_DIR)
    set(WEBSOCKETPP_INCLUDE_DIR "${SOURCE_DIR}")
endif(USE_PLATFORM_WEBSOCKETPP)
