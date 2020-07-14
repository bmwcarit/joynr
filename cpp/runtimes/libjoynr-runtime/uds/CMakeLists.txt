outofdir_target_sources(${PROJECT_NAME} PRIVATE
    #JoynrRuntime.cpp Not included here since there are two projects, one with and one without the runtime, whereas the latter one is used for UT.
    LibJoynrUdsRuntime.cpp
    LibJoynrUdsRuntime.h
)
