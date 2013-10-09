##### Probably don't want to edit below this line #####

SET( QT_USE_QTTEST TRUE )

# Use it
INCLUDE( ${QT_USE_FILE} )

INCLUDE(AddFileDependencies)

# Include the library include directories, and the current build directory (moc)
INCLUDE_DIRECTORIES(
  ../src
  ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_EXECUTABLE(
  cmdline_tester
  cmdline_tester.cpp
)

SET (qjson_test_support_SRCS person.cpp)

ADD_LIBRARY (qjson_test_support STATIC ${qjson_test_support_SRCS})

#ADD_FILE_DEPENDENCIES(cmdline_tester.cpp ${MOC_SOURCE})
TARGET_LINK_LIBRARIES(
  cmdline_tester
  ${QT_LIBRARIES}
  qjson
)

SET( UNIT_TESTS
  testparser
 	testqobjecthelper
  testserializer
)

# Build the tests
FOREACH(test ${UNIT_TESTS})
  MESSAGE(STATUS "Building ${test}")
  ADD_EXECUTABLE(
    ${test}
    ${test}.cpp
  )

  ADD_FILE_DEPENDENCIES(${test}.cpp)
  TARGET_LINK_LIBRARIES(
    ${test}
    ${QT_LIBRARIES}
    qjson
    qjson_test_support
  )
  if (QJSON_TEST_OUTPUT STREQUAL "xml")
    # produce XML output
    add_test( ${test} ${test} -xml -o ${test}.tml )
  else (QJSON_TEST_OUTPUT STREQUAL "xml")
    add_test( ${test} ${test} )
  endif (QJSON_TEST_OUTPUT STREQUAL "xml")
ENDFOREACH()
