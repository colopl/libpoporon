# libpoporon valgrind support
# Copyright (c) 2025 Go Kudo
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Go Kudo <zeriyoshi@gmail.com>

if (VALGRIND)
  message(STATUS "Found Valgrind: ${VALGRIND}")
    
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -fno-inline -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fno-inline -fno-omit-frame-pointer")
endif()
