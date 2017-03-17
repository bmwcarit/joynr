message(STATUS "variable Boost_FOUND=${Boost_FOUND}")
message(STATUS "variable Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")
message(STATUS "variable Boost_LIBRARY_DIRS=${Boost_LIBRARY_DIRS}")
message(STATUS "variable Boost_LIBRARIES=${Boost_LIBRARIES}")
message(STATUS "variable Boost_VERSION=${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")

if(NOT TARGET Boost::boost)
    message(STATUS "Boost::boost target not defined. Creating IMPORTED target.")
    message(STATUS "variable Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}")
    add_library(Boost::boost UNKNOWN IMPORTED GLOBAL)
    set_target_properties(Boost::boost PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
    )
endif(NOT TARGET Boost::boost)

if(NOT TARGET Boost::system)
    message(STATUS "Boost::system target not defined. Creating IMPORTED target.")
    message(STATUS "variable Boost_SYSTEM_LIBRARY=${Boost_SYSTEM_LIBRARY}")
    add_library(Boost::system SHARED IMPORTED GLOBAL)
    set_target_properties(Boost::system PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${Boost_SYSTEM_LIBRARY}"
    )
endif(NOT TARGET Boost::system)

if(NOT TARGET Boost::thread)
    message(STATUS "Boost::thread target not defined. Creating IMPORTED target.")
    message(STATUS "variable Boost_THREAD_LIBRARY=${Boost_THREAD_LIBRARY}")
    add_library(Boost::thread SHARED IMPORTED GLOBAL)
    set_target_properties(Boost::thread PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${Boost_THREAD_LIBRARY}"
        INTERFACE_LINK_LIBRARIES Boost::system
    )
endif(NOT TARGET Boost::thread)

if(NOT TARGET Boost::regex)
    if(Boost_REGEX_LIBRARY)
        message(STATUS "Boost::regex target not defined. Creating IMPORTED target.")
        message(STATUS "variable Boost_REGEX_LIBRARY=${Boost_REGEX_LIBRARY}")
        add_library(Boost::regex SHARED IMPORTED GLOBAL)
        set_target_properties(Boost::regex PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            IMPORTED_LOCATION "${Boost_REGEX_LIBRARY}"
        )
    endif(Boost_REGEX_LIBRARY)
endif(NOT TARGET Boost::regex)

if(NOT TARGET Boost::filesystem)
    if(Boost_FILESYSTEM_LIBRARY)
        message(STATUS "Boost::filesystem target not defined. Creating IMPORTED target.")
        message(STATUS "variable Boost_FILESYSTEM_LIBRARY=${Boost_FILESYSTEM_LIBRARY}")
        add_library(Boost::filesystem SHARED IMPORTED GLOBAL)
        set_target_properties(Boost::filesystem PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            IMPORTED_LOCATION "${Boost_FILESYSTEM_LIBRARY}"
            INTERFACE_LINK_LIBRARIES Boost::system
        )
    endif(Boost_FILESYSTEM_LIBRARY)
endif(NOT TARGET Boost::filesystem)
