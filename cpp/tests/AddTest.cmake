function(add_test TARGET)
    add_executable(
        ${TARGET}
        ${ARGN}
    )
    set_target_properties(
        ${TARGET}
        PROPERTIES
        COMPILE_FLAGS "-Wno-effc++ -Wno-unused-parameter"
    )
    add_dependencies(${TARGET} googletest)
    add_dependencies(${TARGET} googlemock)
    add_dependencies(${TARGET} jsmn)
endfunction(add_test)
