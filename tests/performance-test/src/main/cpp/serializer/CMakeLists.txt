add_executable(performance-serializer
    SerializerPerformanceTest.h
    ../common/PerformanceTest.h
    SerializerTestApplication.cpp
)

target_link_libraries(performance-serializer
    performance-generated
)

AddClangFormat(performance-serializer)
