add_library(performance-provider
    SHARED
    PerformanceTestEchoProvider.h
    PerformanceTestEchoProvider.cpp
)

target_link_libraries(performance-provider
    performance-generated
)

AddClangFormat(performance-provider)

