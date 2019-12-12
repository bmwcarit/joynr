project(Serializer)

set(SOURCES
    ByteBuffer.cpp
)

set(PUBLIC_HEADERS
    include/joynr/ByteBuffer.h
    include/joynr/serializer/JsonDeserializable.h
    include/joynr/serializer/Serializable.h
    include/joynr/serializer/SerializationPlaceholder.h
    include/joynr/serializer/SerializerTraits.h
    include/joynr/serializer/Serializer.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC muesli::muesli
    PUBLIC smrf::smrf
)

install(
    DIRECTORY include/
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
