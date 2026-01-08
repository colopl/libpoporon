/*
 * libpoporon - encode.c
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "internal/common.h"

#if POPORON_USE_SIMD
#include "internal/simd.h"
#endif

extern bool poporon_encode_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity)
{
#if POPORON_USE_SIMD && defined(POPORON_SIMD_AVX2)
    __m256i parity_vec, xor_vec;
    uint16_t i, j, fb, num_roots_aligned, k;
    uint8_t xor_values[32];

    if (!pprn || !data || !parity) {
        return false;
    }

    num_roots_aligned = (pprn->rs->num_roots - 1) & ~31;
    pmemset(parity, 0, pprn->rs->num_roots * sizeof(uint8_t));

    for (i = 0; i < size; i++) {
        fb = pprn->rs->gf->exp2log[(((uint16_t)data[i]) & ((uint16_t)pprn->rs->gf->field_size)) ^ parity[0]];

        if (fb != pprn->rs->gf->field_size) {
            for (j = 1; j <= num_roots_aligned && j + 31 < pprn->rs->num_roots; j += 32) {
                parity_vec = _mm256_loadu_si256((__m256i *)&parity[j]);

                for (k = 0; k < 32; k++) {
                    xor_values[k] = pprn->rs->gf->log2exp[gf_mod(
                        pprn->rs->gf, fb + pprn->rs->generator_polynomial[pprn->rs->num_roots - (j + k)])];
                }

                xor_vec = _mm256_loadu_si256((__m256i *)xor_values);
                parity_vec = _mm256_xor_si256(parity_vec, xor_vec);
                _mm256_storeu_si256((__m256i *)&parity[j], parity_vec);
            }

            /* Handle remaining elements */
            for (; j < pprn->rs->num_roots; j++) {
                parity[j] ^=
                    pprn->rs->gf
                        ->log2exp[gf_mod(pprn->rs->gf, fb + pprn->rs->generator_polynomial[pprn->rs->num_roots - j])];
            }
        }

        pmemmove(&parity[0], &parity[1], sizeof(uint8_t) * (pprn->rs->num_roots - 1));

        if (fb != pprn->rs->gf->field_size) {
            parity[pprn->rs->num_roots - 1] =
                pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, fb + pprn->rs->generator_polynomial[0])];
        } else {
            parity[pprn->rs->num_roots - 1] = 0;
        }
    }

    return true;

#elif POPORON_USE_SIMD && (defined(POPORON_SIMD_NEON) || defined(POPORON_SIMD_WASM))
    uint16_t i, j, fb, num_roots_aligned, k;
    uint8_t xor_values[16];
#if defined(POPORON_SIMD_NEON)
    uint8x16_t parity_vec, xor_vec;
#elif defined(POPORON_SIMD_WASM)
    v128_t parity_vec, xor_vec;
#endif

    if (!pprn || !data || !parity) {
        return false;
    }

    num_roots_aligned = (pprn->rs->num_roots - 1) & ~15;
    pmemset(parity, 0, pprn->rs->num_roots * sizeof(uint8_t));

    for (i = 0; i < size; i++) {
        fb = pprn->rs->gf->exp2log[(((uint16_t)data[i]) & ((uint16_t)pprn->rs->gf->field_size)) ^ parity[0]];

        if (fb != pprn->rs->gf->field_size) {
            for (j = 1; j <= num_roots_aligned && j + 15 < pprn->rs->num_roots; j += 16) {
#if defined(POPORON_SIMD_NEON)
                parity_vec = vld1q_u8(&parity[j]);

                for (k = 0; k < 16; k++) {
                    xor_values[k] = pprn->rs->gf->log2exp[gf_mod(
                        pprn->rs->gf, fb + pprn->rs->generator_polynomial[pprn->rs->num_roots - (j + k)])];
                }

                xor_vec = vld1q_u8(xor_values);
                parity_vec = veorq_u8(parity_vec, xor_vec);
                vst1q_u8(&parity[j], parity_vec);
#elif defined(POPORON_SIMD_WASM)
                parity_vec = wasm_v128_load(&parity[j]);

                for (k = 0; k < 16; k++) {
                    xor_values[k] = pprn->rs->gf->log2exp[gf_mod(
                        pprn->rs->gf, fb + pprn->rs->generator_polynomial[pprn->rs->num_roots - (j + k)])];
                }

                xor_vec = wasm_v128_load(xor_values);
                parity_vec = wasm_v128_xor(parity_vec, xor_vec);
                wasm_v128_store(&parity[j], parity_vec);
#endif
            }

            /* Handle remaining elements */
            for (; j < pprn->rs->num_roots; j++) {
                parity[j] ^=
                    pprn->rs->gf
                        ->log2exp[gf_mod(pprn->rs->gf, fb + pprn->rs->generator_polynomial[pprn->rs->num_roots - j])];
            }
        }

        pmemmove(&parity[0], &parity[1], sizeof(uint8_t) * (pprn->rs->num_roots - 1));

        if (fb != pprn->rs->gf->field_size) {
            parity[pprn->rs->num_roots - 1] =
                pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, fb + pprn->rs->generator_polynomial[0])];
        } else {
            parity[pprn->rs->num_roots - 1] = 0;
        }
    }

    return true;

#else
    uint16_t i, j, fb;

    if (!pprn || !data || !parity) {
        return false;
    }

    pmemset(parity, 0, pprn->rs->num_roots * sizeof(uint8_t));

    for (i = 0; i < size; i++) {
        fb = pprn->rs->gf->exp2log[(((uint16_t)data[i]) & ((uint16_t)pprn->rs->gf->field_size)) ^ parity[0]];

        if (fb != pprn->rs->gf->field_size) {
            for (j = 1; j < pprn->rs->num_roots; j++) {
                parity[j] ^=
                    pprn->rs->gf
                        ->log2exp[gf_mod(pprn->rs->gf, fb + pprn->rs->generator_polynomial[pprn->rs->num_roots - j])];
            }
        }

        pmemmove(&parity[0], &parity[1], sizeof(uint8_t) * (pprn->rs->num_roots - 1));

        if (fb != pprn->rs->gf->field_size) {
            parity[pprn->rs->num_roots - 1] =
                pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, fb + pprn->rs->generator_polynomial[0])];
        } else {
            parity[pprn->rs->num_roots - 1] = 0;
        }
    }

    return true;
#endif
}
