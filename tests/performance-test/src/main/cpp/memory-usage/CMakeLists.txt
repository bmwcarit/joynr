add_executable(memory-usage-consumer-app
    MemoryUsageConsumerApplication.cpp
)

target_link_libraries(memory-usage-consumer-app
    performance-generated
    Joynr::JoynrWsRuntime
    ${Boost_LIBRARIES}
)

AddClangFormat(memory-usage-consumer-app)
