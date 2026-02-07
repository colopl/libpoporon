# libpoporon test

include(CTest)
include(FetchContent)
include(CheckIncludeFile)

enable_testing()

FetchContent_Declare(unity SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/unity)
FetchContent_MakeAvailable(unity)

set(POPORON_TEST_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
add_definitions(-DPOPORON_TEST_ASSETS_DIR="${POPORON_TEST_ASSETS_DIR}")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

set(POPORON_TEST_LINK_LIBRARIES poporon unity)

if(POPORON_ENABLE_COVERAGE)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/coverage)
  add_custom_target(coverage
    COMMAND ${LCOV} --initial --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage/base.info
    COMMAND ${CMAKE_CTEST_COMMAND} -C ${CMAKE_BUILD_TYPE}
    COMMAND ${LCOV} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage/test.info
    COMMAND ${LCOV} --add-tracefile ${CMAKE_BINARY_DIR}/coverage/base.info --add-tracefile ${CMAKE_BINARY_DIR}/coverage/test.info --output-file ${CMAKE_BINARY_DIR}/coverage/total.info
    COMMAND ${LCOV} --remove ${CMAKE_BINARY_DIR}/coverage/total.info '/usr/*' --output-file ${CMAKE_BINARY_DIR}/coverage/filtered.info
    COMMAND ${GENHTML} --demangle-cpp --legend --title "poporon Coverage Report" --output-directory ${CMAKE_BINARY_DIR}/coverage/html ${CMAKE_BINARY_DIR}/coverage/filtered.info
    COMMENT "Generating coverage report with lcov/gcov"
  )

  message(STATUS "Coverage report generation target 'coverage' is now available")
endif()

file(GLOB TEST_SOURCES "tests/test_*.c")

CHECK_INCLUDE_FILE(fec.h HAVE_FEC_H)

if(HAVE_FEC_H)
  message(STATUS "Found fec.h header, enabling FEC compatibility tests")
  file(GLOB FEC_TEST_SOURCES "tests/fec_*.c")
  list(APPEND TEST_SOURCES ${FEC_TEST_SOURCES})
  list(APPEND POPORON_TEST_LINK_LIBRARIES fec)
else()
  message(STATUS "fec.h header not found, skipping FEC compatibility tests")
endif()

foreach(TEST_SOURCE ${TEST_SOURCES})
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
  set(TEST_NAME "poporon_${TEST_NAME}")
  
  add_executable(${TEST_NAME} ${TEST_SOURCE})
  if(HAVE_FEC_H)
    target_link_libraries(${TEST_NAME} PRIVATE poporon unity fec)
  else()
    target_link_libraries(${TEST_NAME} PRIVATE poporon unity)
  endif()
    
  target_include_directories(${TEST_NAME} PRIVATE ${unity_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)
 
  set_target_properties(${TEST_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  )
  
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
  
  if(POPORON_ENABLE_VALGRIND)
    add_test(
      NAME ${TEST_NAME}_valgrind
      COMMAND ${VALGRIND} 
        "--tool=memcheck"
        "--leak-check=full"
        "--show-leak-kinds=all"
        "--track-origins=yes"
        "--error-exitcode=1"
      $<TARGET_FILE:${TEST_NAME}>)
  endif()
endforeach()
