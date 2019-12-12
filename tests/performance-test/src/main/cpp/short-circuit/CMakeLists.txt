add_executable(performance-short-circuit
    ShortCircuitApplication.cpp
    ShortCircuitRuntime.h
    ShortCircuitRuntime.cpp
    ShortCircuitTest.h
    ../common/PerformanceTest.h
)

target_link_libraries(performance-short-circuit
    performance-generated
    performance-provider
    ${Boost_LIBRARIES}
    Joynr::JoynrClusterControllerRuntime
)

target_include_directories(performance-short-circuit
    SYSTEM PRIVATE "../../../../../../cpp/"
)

AddClangFormat(performance-short-circuit)
