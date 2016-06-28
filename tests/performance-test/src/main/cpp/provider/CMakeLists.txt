add_library(performance-provider
    SHARED
    PerformanceTestEchoProvider.h
    PerformanceTestEchoProvider.cpp
)

target_link_libraries(performance-provider
    performance-generated
)

install(
    TARGETS performance-provider
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

AddClangFormat(performance-provider)

