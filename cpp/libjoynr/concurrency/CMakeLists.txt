project(Concurrency)

set(SOURCES
    BlockingQueue.cpp
    DelayedScheduler.cpp
    Runnable.cpp
    Semaphore.cpp
    SteadyTimer.cpp
    ThreadPool.cpp
    ThreadPoolDelayedScheduler.cpp
)

set(PUBLIC_HEADERS
    include/joynr/BlockingQueue.h
    include/joynr/DelayedRunnable.h
    include/joynr/DelayedScheduler.h
    include/joynr/Runnable.h
    include/joynr/Semaphore.h
    include/joynr/SteadyTimer.h
    include/joynr/ThreadPool.h
    include/joynr/ThreadPoolDelayedScheduler.h
)

add_library(${PROJECT_NAME} OBJECT ${PUBLIC_HEADERS} ${SOURCES})
add_library(Joynr::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<INSTALL_INTERFACE:${JOYNR_INSTALL_INCLUDEDIR}>"
)
target_link_objlibraries(${PROJECT_NAME}
    PUBLIC Joynr::Util
)
objlibrary_target_link_libraries(${PROJECT_NAME}
    PRIVATE Boost::system
)

install(
    DIRECTORY include/
    DESTINATION "${JOYNR_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

AddClangFormat(${PROJECT_NAME})

AddIncludeWhatYouUse(${PROJECT_NAME})

AddClangTidy(${PROJECT_NAME})
