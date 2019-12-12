option(ENABLE_MQTT_OCSP "Enable OCSP in mosquitto?" OFF)

project(Mqtt)

set(SOURCES
    MosquittoConnection.cpp
    MosquittoConnection.h
    MqttMessagingSkeleton.cpp
    MqttReceiver.cpp
    MqttSender.cpp
    MqttSender.h
    MqttTransportStatus.cpp
    MqttTransportStatus.h
)

add_library(${PROJECT_NAME} OBJECT ${SOURCES})
add_library(JoynrCCLib::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME}
    PRIVATE $<$<BOOL:${ENABLE_MQTT_OCSP}>:MQTT_OCSP_ENABLED>
)
target_include_directories(${PROJECT_NAME}
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    # common
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
    PUBLIC "$<BUILD_INTERFACE:${EXPORT_HEADER_INCLUDE_DIR}>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC Joynr::JoynrLib
    PRIVATE mosquitto::mosquitto
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
