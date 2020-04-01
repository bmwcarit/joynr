add_executable(performance-consumer-app-ws
    ../common/Enum.h
    PerformanceConsumerApplication.cpp
    PerformanceConsumer.h
)

add_executable(performance-consumer-app-cc
    ../common/Enum.h
    PerformanceConsumerApplication.cpp
    PerformanceConsumer.h
)

target_link_libraries(performance-consumer-app-ws
    Joynr::JoynrLib
    Joynr::JoynrWsRuntime
    ${Boost_LIBRARIES}
    dummyKeychain
)

target_link_libraries(performance-consumer-app-cc
    Joynr::JoynrLib
    Joynr::JoynrClusterControllerRuntime
    ${Boost_LIBRARIES}
    dummyKeychain
)

install(
    TARGETS
        performance-consumer-app-ws
        performance-consumer-app-cc
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

AddClangFormat(performance-consumer-app-ws)
AddClangFormat(performance-consumer-app-cc)

