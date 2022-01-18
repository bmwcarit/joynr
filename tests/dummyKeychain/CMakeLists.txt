cmake_minimum_required(VERSION 3.10)
project(dummyKeychain)
set(dummyKeychain_VERSION 0.0.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/CMake")
include(GNUInstallDirs)
include(AddClangFormat)

if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()

message(STATUS "option BUILD_SHARED_LIBS=" ${BUILD_SHARED_LIBS})

# need a dependency to joynr for joynr::IKeyChain interface
# depending on joynr will allow to use joynr utils as well
find_package(Joynr 1.18.2 REQUIRED HINTS "${CMAKE_BINARY_DIR}/../joynr/bin")

set(
    dummyKeychain_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/joynr/tests/DummyKeychainImpl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/joynr/tests/DummyKeyChainParameters.h"
)

set(
    dummyKeychain_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/DummyKeychainImpl.cpp"
)

add_library(
    dummyKeychain
    ${dummyKeychain_HEADERS}
    ${dummyKeychain_SOURCES}
)

AddClangFormat(dummyKeychain ${dummyKeychain_HEADERS})

target_link_libraries(dummyKeychain
    Joynr::JoynrLib
    MoCOCrW::mococrw
)

target_include_directories(dummyKeychain
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/dummyKeychainConfigVersion.cmake"
  VERSION ${dummyKeychain_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(TARGETS dummyKeychain EXPORT dummyKeychainConfig
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT dummyKeychainConfig DESTINATION share/dummyKeychain/cmake)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dummyKeychainConfigVersion.cmake" DESTINATION share/dummyKeychain/cmake)

export(TARGETS dummyKeychain FILE dummyKeychainConfig.cmake)
export(PACKAGE dummyKeychain)
