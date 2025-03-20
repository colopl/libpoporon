# libpoporon
# Copyright (c) 2025 Go Kudo
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Go Kudo <zeriyoshi@gmail.com>

cmake_minimum_required(VERSION 3.25)
project(poporon
  VERSION 1.0.0
  LANGUAGES C
  DESCRIPTION "Polynomial Operations Providing Optimal Reed-Solomon Organized Numerics - POPORON"
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

option(POPORON_USE_VALGRIND "Use Valgrind if available" ON)
option(POPORON_USE_COVERAGE "Use coverage if available" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(POPORON_TESTING ON)
endif()

if(POPORON_TESTING AND NOT EMSCRIPTEN)
  message(STATUS "Enabled testing")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -fno-inline -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fno-inline -fno-omit-frame-pointer")

  find_program(VALGRIND "valgrind")
  if(POPORON_USE_VALGRIND AND VALGRIND)
    message(STATUS "Found Valgrind: ${VALGRIND}")
    set(POPORON_ENABLE_VALGRIND ON)
  else()
    set(POPORON_ENABLE_VALGRIND OFF)
  endif()

  if(POPORON_USE_COVERAGE)
    find_program(LCOV lcov)
    find_program(GENHTML genhtml)
    
    if(CMAKE_C_COMPILER_ID MATCHES "GNU")
      if(LCOV AND GENHTML)
        set(POPORON_ENABLE_COVERAGE ON)

        if(CMAKE_C_COMPILER_ID MATCHES "GNU")
          link_libraries(gcov)
        endif()
      else()
        message(WARNING "lcov or genhtml not found, disabling coverage")
        set(POPORON_ENABLE_COVERAGE OFF)
      endif()
    else()
      message(WARNING "Coverage is supported only with GCC compilers")
      set(POPORON_ENABLE_COVERAGE OFF)
    endif()
  else()
    set(POPORON_ENABLE_COVERAGE OFF)
  endif()
endif()

file(GLOB SOURCES "src/*.c")
file(GLOB_RECURSE HEADERS "include/*.h")

if(EMSCRIPTEN)
  include(cmake/emscripten.cmake)
endif()

add_library(poporon-obj OBJECT ${SOURCES})
target_include_directories(poporon-obj PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(POPORON_ENABLE_COVERAGE)
  target_compile_options(poporon-obj PRIVATE "--coverage")
  target_link_options(poporon-obj PRIVATE "--coverage")
endif()

add_library(poporon STATIC $<TARGET_OBJECTS:poporon-obj>)

if(POPORON_ENABLE_COVERAGE)
  target_link_options(poporon PRIVATE "--coverage")
endif()

install(
  FILES ${HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/poporon
)

install(
  TARGETS poporon
  EXPORT poporon-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(POPORON_TESTING)
  include(cmake/test.cmake)
endif()
