if(NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio")
	#Disable all effc++ and security warnings in libs-directory.
	remove_definitions(-Weffc++)
	remove_definitions(-Wformat-security)
	add_definitions(-Wno-format-security)
endif(NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${JOYNR_BINARY_DIR}/lib/qjson")
add_subdirectory(qjson-0.7.1)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${JOYNR_BINARY_DIR}/lib/log4qt")
add_subdirectory(log4qt-0.3/src)
