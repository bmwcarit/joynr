message(STATUS "variable Boost_FOUND=${Boost_FOUND}")
message(STATUS "variable Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")
message(STATUS "variable Boost_LIBRARY_DIRS=${Boost_LIBRARY_DIRS}")
message(STATUS "variable Boost_LIBRARIES=${Boost_LIBRARIES}")
message(STATUS "variable Boost_VERSION=${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")

if(NOT TARGET Boost::boost)
    message(STATUS "Boost::boost target not defined. Creating IMPORTED target.")
    message(STATUS "variable Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")
    add_library(Boost::boost UNKNOWN IMPORTED)
    set_target_properties(Boost::boost PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
    )
endif(NOT TARGET Boost::boost)

if(NOT TARGET Boost::system)
    message(STATUS "Boost::system target not defined. Creating IMPORTED target.")
    message(STATUS "variable Boost_SYSTEM_LIBRARY=${Boost_SYSTEM_LIBRARY}")
    add_library(Boost::system SHARED IMPORTED)
    set_target_properties(Boost::system PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${Boost_SYSTEM_LIBRARY}"
    )
endif(NOT TARGET Boost::system)

if(NOT TARGET Boost::thread)
    message(STATUS "Boost::thread target not defined. Creating IMPORTED target.")
    message(STATUS "variable Boost_THREAD_LIBRARY=${Boost_THREAD_LIBRARY}")
    add_library(Boost::thread SHARED IMPORTED)
    set_target_properties(Boost::thread PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${Boost_THREAD_LIBRARY}"
        INTERFACE_LINK_LIBRARIES Boost::system
    )
endif(NOT TARGET Boost::thread)
