add_executable(performance-consumer-app
    ../common/Enum.h
    PerformanceConsumerApplication.cpp
    PerformanceConsumer.h
)

target_link_libraries(performance-consumer-app
    performance-generated
    ${Joynr_LIB_WS_LIBRARIES}
    ${Boost_LIBRARIES}
    dummyKeychain
)

install(
    TARGETS performance-consumer-app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

AddClangFormat(performance-consumer-app)

