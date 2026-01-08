/*
 * libpoporon - simd.h
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_INTERNAL_SIMD_H
#define POPORON_INTERNAL_SIMD_H

#include <stddef.h>
#include <stdint.h>

#if defined(POPORON_SIMD_AVX2)
#include <immintrin.h>
#elif defined(POPORON_SIMD_NEON)
#include <arm_neon.h>
#elif defined(POPORON_SIMD_WASM)
#include <wasm_simd128.h>
#endif

#endif /* POPORON_INTERNAL_SIMD_H */
