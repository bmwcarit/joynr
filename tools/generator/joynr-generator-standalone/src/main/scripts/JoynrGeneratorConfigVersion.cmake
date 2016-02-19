# This file is derived from a CMake tutorial (http://www.cmake.org/Wiki/CMake/)
# that is available under Creative Commons Attribution 2.5 Generic (CC BY 2.5, http://creativecommons.org/licenses/by/2.5/). 
# There are no attributions specified by the author.

set(PACKAGE_VERSION "${project.version}")

# Check whether the requested PACKAGE_FIND_VERSION is compatible
if("${PACKAGE_VERSION}" VERSION_LESS "${PACKAGE_FIND_VERSION}")
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if ("${PACKAGE_VERSION}" VERSION_EQUAL "${PACKAGE_FIND_VERSION}")
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
