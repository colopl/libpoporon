# libpoporon Emscripten support
# Copyright (c) 2025 Go Kudo
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Go Kudo <zeriyoshi@gmail.com>

add_link_options(
  "-sWASM=1"
  "-sALLOW_MEMORY_GROWTH=1"
)
