add_executable(performance-provider-app-ws
    PerformanceProviderApplication.cpp
)

add_executable(performance-provider-app-cc
    PerformanceProviderApplication.cpp
)

target_link_libraries(performance-provider-app-ws
    performance-generated
    performance-provider
    Joynr::JoynrWsRuntime
    ${Boost_LIBRARIES}
    dummyKeychain
)

target_link_libraries(performance-provider-app-cc
    performance-generated
    performance-provider
    Joynr::JoynrClusterControllerRuntime
    ${Boost_LIBRARIES}
    dummyKeychain
)

install(
    TARGETS
        performance-provider-app-ws
        performance-provider-app-cc
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

AddClangFormat(performance-provider-app-ws)
AddClangFormat(performance-provider-app-cc)
