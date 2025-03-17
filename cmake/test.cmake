# libpoporon test
# Copyright (c) 2025 Go Kudo
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Go Kudo <zeriyoshi@gmail.com>

if(POPORON_TESTING)
  include(CTest)
  include(FetchContent)
  include(CheckIncludeFile)
  
  enable_testing()

  FetchContent_Declare(unity SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/unity)
  FetchContent_MakeAvailable(unity)
  
  set(POPORON_TEST_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
  add_definitions(-DPOPORON_TEST_ASSETS_DIR="${POPORON_TEST_ASSETS_DIR}")

  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  
  set(POPORON_TEST_LINK_LIBRARIES ${CMAKE_PROJECT_NAME} unity)

  file(GLOB TEST_SOURCES "tests/test_*.c")

  CHECK_INCLUDE_FILE(fec.h HAVE_FEC_H)
  
  if(HAVE_FEC_H)
    message(STATUS "Found fec.h header, enabling FEC compatibility tests")
    file(GLOB FEC_TEST_SOURCES "tests/testfec_*.c")
    list(APPEND TEST_SOURCES ${FEC_TEST_SOURCES})
    list(APPEND POPORON_TEST_LINK_LIBRARIES fec)
  else()
    message(STATUS "fec.h header not found, skipping FEC compatibility tests")
  endif()
  
  foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
  
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    if(TEST_NAME MATCHES "^test_")
      target_link_libraries(${TEST_NAME} PRIVATE ${CMAKE_PROJECT_NAME} unity)
    elseif(TEST_NAME MATCHES "^testfec_")
      target_link_libraries(${TEST_NAME} PRIVATE ${CMAKE_PROJECT_NAME} unity fec)
    endif()
    
    target_include_directories(${TEST_NAME} PRIVATE ${unity_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)
  
    set_target_properties(${TEST_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
  
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
  
    if(POPORON_ENABLE_VALGRIND)
      add_test(
        NAME valgrind_${TEST_NAME}
        COMMAND ${VALGRIND} 
          "--tool=memcheck"
          "--leak-check=full"
          "--show-leak-kinds=all"
          "--track-origins=yes"
          "--error-exitcode=1"
        $<TARGET_FILE:${TEST_NAME}>)
    endif()
  endforeach()
endif()
