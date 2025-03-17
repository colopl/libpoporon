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

option(BUILD_TESTING "Build tests" ON)
option(POPORON_USE_VALGRIND "Use Valgrind if available" ON)
set(POPORON_TESTING ${BUILD_TESTING})

find_program(VALGRIND "valgrind")
if(POPORON_TESTING AND POPORON_USE_VALGRIND AND VALGRIND AND NOT EMSCRIPTEN)
  include(cmake/valgrind.cmake)

  set(POPORON_ENABLE_VALGRIND ON)
else()
  set(POPORON_ENABLE_VALGRIND OFF)
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

add_library(poporon STATIC $<TARGET_OBJECTS:poporon-obj>)

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
