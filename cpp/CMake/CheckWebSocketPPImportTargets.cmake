
if(NOT TARGET websocketpp)
    message(STATUS "websocketpp target not defined. Creating IMPORTED target.")

    include(AddExternalProject)
    AddExternalProject(
        websocketpp
        GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
        GIT_TAG 378437aecdcb1dfe62096ffd5d944bf1f640ccc3 # git tag 0.7.0
        TIMEOUT 10
        # Disable configure step
        CONFIGURE_COMMAND ""
        # Disable build step
        BUILD_COMMAND ""
    )

    ExternalProject_Get_Property(websocketpp SOURCE_DIR)
    set(WEBSOCKETPP_INCLUDE_DIR "${SOURCE_DIR}")
endif(NOT TARGET websocketpp)

