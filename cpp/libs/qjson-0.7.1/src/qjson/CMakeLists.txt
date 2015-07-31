# The source code needs to know which DLL is being built when compiling on Windows
if(${WIN32})
    if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
        add_definitions(/DBUILDING_QJSON)
    else()
        add_definitions(-DBUILDING_QJSON)
    endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")
endif(${WIN32})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(qjson_HEADERS
    json_parser.hh
    json_scanner.h
    location.hh
    parser.h
    parser_p.h
    parserrunnable.h
    position.hh
    qjson_debug.h
    qjson_export.h
    qobjecthelper.h
    serializer.h
    serializerrunnable.h
    stack.hh
)

set(qjson_SOURCES
    parser.cpp
    qobjecthelper.cpp
    json_scanner.cpp
    json_parser.cc
    parserrunnable.cpp
    serializer.cpp
    serializerrunnable.cpp
)

add_library(
    qjson SHARED
    ${qjson_HEADERS}
    ${qjson_SOURCES}
)
target_link_libraries(qjson Qt5::Core)

set_target_properties(
    qjson
    PROPERTIES
    PUBLIC_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../"
)

set_target_properties(
    qjson
    PROPERTIES
    VERSION ${QJSON_LIB_VERSION_STRING}
    SOVERSION ${QJSON_LIB_VERSION_STRING}
    DEFINE_SYMBOL QJSON_MAKEDLL
)

export(
    TARGETS qjson
    FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/qjson.cmake"
)

install(
    TARGETS qjson EXPORT JoynrTargets
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT runtime
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT library
    # this will flatten directory hierarchy of headder files
    # PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT development
)
# so we install header files manually to preserve directory hierarchy
install(
    DIRECTORY .
    DESTINATION "${INSTALL_INCLUDE_DIR}/qjson"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hh"
)

#install(
#    EXPORT qjson_EXPORT
#    FILE "qjson.cmake"
#    DESTINATION "lib"
#)
