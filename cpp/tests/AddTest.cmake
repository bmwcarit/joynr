function(AddTest TARGET)
    add_executable(
        ${TARGET}
        ${ARGN}
    )
    set_target_properties(
        ${TARGET}
        PROPERTIES
        COMPILE_FLAGS "-Wno-effc++ -Wno-unused-parameter"
        AUTOMOC TRUE
    )
    if(NOT USE_PLATFORM_GTEST_GMOCK)
        add_dependencies(${TARGET} googletest)
        add_dependencies(${TARGET} googlemock)
    endif(NOT USE_PLATFORM_GTEST_GMOCK)

    if(NOT USE_PLATFORM_MOSQUITTO)
        add_dependencies(${TARGET} mosquitto::mosquitto)
    endif(NOT USE_PLATFORM_MOSQUITTO)

    target_include_directories(
        ${TARGET}
        SYSTEM
        PRIVATE
        ${GTEST_INCLUDE_DIRS}
        ${GMOCK_INCLUDE_DIRS}
        ${MOSQUITTO_INCLUDE_DIRS}
    )
endfunction(AddTest)
