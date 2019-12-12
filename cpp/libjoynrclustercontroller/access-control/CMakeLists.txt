project(AccessControl)

set(SOURCES
    AccessControlAlgorithm.cpp
    AccessControlAlgorithm.h
    AccessControlListEditor.cpp
    AccessControlListEditor.h
    AccessControlUtils.h
    AccessController.cpp
    AccessController.h
    LocalDomainAccessController.cpp
    LocalDomainAccessController.h
    LocalDomainAccessStore.cpp
    LocalDomainAccessStore.h
    RadixTree.h
    TrustLevelComparator.cpp
    TrustLevelComparator.h
    Validator.h
    WildcardStorage.h
)

add_library(${PROJECT_NAME} OBJECT ${SOURCES})
add_library(JoynrCCLib::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    # CallContext
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
    # common
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
    PUBLIC "$<BUILD_INTERFACE:${EXPORT_HEADER_INCLUDE_DIR}>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PUBLIC Joynr::JoynrLib
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
