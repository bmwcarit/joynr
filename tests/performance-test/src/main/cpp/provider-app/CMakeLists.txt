add_executable(performance-provider-app
    PerformanceProviderApplication.cpp
)

target_link_libraries(performance-provider-app
    performance-generated
    performance-provider
    ${Joynr_LIB_WS_LIBRARIES}
    ${Boost_LIBRARIES}
    dummyKeychain
)

install(
    TARGETS performance-provider-app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

AddClangFormat(performance-provider-app)
