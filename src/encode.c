/*
 * libpoporon - encode.c
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "internal/common.h"
#include "internal/ldpc.h"

#if POPORON_USE_SIMD
#include "internal/simd.h"
#endif

static bool rs_encode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity)
{
    poporon_rs_t *rs = pprn->ctx.rs.rs;

#if POPORON_USE_SIMD && defined(POPORON_SIMD_AVX2)
    __m256i parity_vec, xor_vec;
    uint16_t i, j, fb, num_roots_aligned, k;
    uint8_t xor_values[32];

    num_roots_aligned = (rs->num_roots - 1) & ~31;
    pmemset(parity, 0, rs->num_roots * sizeof(uint8_t));

    for (i = 0; i < size; i++) {
        fb = rs->gf->exp2log[(((uint16_t)data[i]) & ((uint16_t)rs->gf->field_size)) ^ parity[0]];

        if (fb != rs->gf->field_size) {
            for (j = 1; j <= num_roots_aligned && j + 31 < rs->num_roots; j += 32) {
                parity_vec = _mm256_loadu_si256((__m256i *)&parity[j]);

                for (k = 0; k < 32; k++) {
                    xor_values[k] =
                        rs->gf->log2exp[gf_mod(rs->gf, fb + rs->generator_polynomial[rs->num_roots - (j + k)])];
                }

                xor_vec = _mm256_loadu_si256((__m256i *)xor_values);
                parity_vec = _mm256_xor_si256(parity_vec, xor_vec);
                _mm256_storeu_si256((__m256i *)&parity[j], parity_vec);
            }

            for (; j < rs->num_roots; j++) {
                parity[j] ^= rs->gf->log2exp[gf_mod(rs->gf, fb + rs->generator_polynomial[rs->num_roots - j])];
            }
        }

        pmemmove(&parity[0], &parity[1], sizeof(uint8_t) * (rs->num_roots - 1));

        if (fb != rs->gf->field_size) {
            parity[rs->num_roots - 1] = rs->gf->log2exp[gf_mod(rs->gf, fb + rs->generator_polynomial[0])];
        } else {
            parity[rs->num_roots - 1] = 0;
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

    num_roots_aligned = (rs->num_roots - 1) & ~15;
    pmemset(parity, 0, rs->num_roots * sizeof(uint8_t));

    for (i = 0; i < size; i++) {
        fb = rs->gf->exp2log[(((uint16_t)data[i]) & ((uint16_t)rs->gf->field_size)) ^ parity[0]];

        if (fb != rs->gf->field_size) {
            for (j = 1; j <= num_roots_aligned && j + 15 < rs->num_roots; j += 16) {
#if defined(POPORON_SIMD_NEON)
                parity_vec = vld1q_u8(&parity[j]);

                for (k = 0; k < 16; k++) {
                    xor_values[k] =
                        rs->gf->log2exp[gf_mod(rs->gf, fb + rs->generator_polynomial[rs->num_roots - (j + k)])];
                }

                xor_vec = vld1q_u8(xor_values);
                parity_vec = veorq_u8(parity_vec, xor_vec);
                vst1q_u8(&parity[j], parity_vec);
#elif defined(POPORON_SIMD_WASM)
                parity_vec = wasm_v128_load(&parity[j]);

                for (k = 0; k < 16; k++) {
                    xor_values[k] =
                        rs->gf->log2exp[gf_mod(rs->gf, fb + rs->generator_polynomial[rs->num_roots - (j + k)])];
                }

                xor_vec = wasm_v128_load(xor_values);
                parity_vec = wasm_v128_xor(parity_vec, xor_vec);
                wasm_v128_store(&parity[j], parity_vec);
#endif
            }

            for (; j < rs->num_roots; j++) {
                parity[j] ^= rs->gf->log2exp[gf_mod(rs->gf, fb + rs->generator_polynomial[rs->num_roots - j])];
            }
        }

        pmemmove(&parity[0], &parity[1], sizeof(uint8_t) * (rs->num_roots - 1));

        if (fb != rs->gf->field_size) {
            parity[rs->num_roots - 1] = rs->gf->log2exp[gf_mod(rs->gf, fb + rs->generator_polynomial[0])];
        } else {
            parity[rs->num_roots - 1] = 0;
        }
    }

    return true;

#else
    uint16_t i, j, fb;

    pmemset(parity, 0, rs->num_roots * sizeof(uint8_t));

    for (i = 0; i < size; i++) {
        fb = rs->gf->exp2log[(((uint16_t)data[i]) & ((uint16_t)rs->gf->field_size)) ^ parity[0]];

        if (fb != rs->gf->field_size) {
            for (j = 1; j < rs->num_roots; j++) {
                parity[j] ^= rs->gf->log2exp[gf_mod(rs->gf, fb + rs->generator_polynomial[rs->num_roots - j])];
            }
        }

        pmemmove(&parity[0], &parity[1], sizeof(uint8_t) * (rs->num_roots - 1));

        if (fb != rs->gf->field_size) {
            parity[rs->num_roots - 1] = rs->gf->log2exp[gf_mod(rs->gf, fb + rs->generator_polynomial[0])];
        } else {
            parity[rs->num_roots - 1] = 0;
        }
    }

    return true;
#endif
}

static bool ldpc_encode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity)
{
    uint8_t *encode_input;
    poporon_ldpc_t *ldpc = pprn->ctx.ldpc.ldpc;
    uint8_t *outer_buf, *codeword, *interleaved;
    size_t i;

    if (size != ldpc->info_bytes) {
        return false;
    }

    if (ldpc->config.use_outer_interleave && ldpc->outer_interleaver.forward) {
        outer_buf = ldpc->temp_outer;
        if (!outer_buf) {
            return false;
        }

        pmemset(outer_buf, 0, ldpc->info_bytes);

        for (i = 0; i < ldpc->info_bytes; i++) {
            outer_buf[ldpc->outer_interleaver.forward[i]] = data[i];
        }

        pmemcpy(data, outer_buf, ldpc->info_bytes);
        encode_input = data;
    } else {
        encode_input = data;
    }

    if (!poporon_ldpc_encode(ldpc, encode_input, parity)) {
        return false;
    }

    if (ldpc->config.use_inner_interleave && ldpc->interleaver.forward) {
        codeword = ldpc->temp_codeword;
        pmemcpy(codeword, encode_input, ldpc->info_bytes);
        pmemcpy(codeword + ldpc->info_bytes, parity, ldpc->parity_bytes);

        interleaved = ldpc->temp_interleaved;
        if (!interleaved) {
            return false;
        }

        poporon_ldpc_interleave(ldpc, codeword, interleaved);

        pmemcpy(data, interleaved, ldpc->info_bytes);
        pmemcpy(parity, interleaved + ldpc->info_bytes, ldpc->parity_bytes);
    }

    return true;
}

static bool bch_encode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity)
{
    poporon_bch_t *bch = pprn->ctx.bch.bch;
    uint32_t data_val = 0, codeword = 0, parity_val;
    uint16_t i, data_len, codeword_len, parity_bits, data_bytes, parity_bytes;

    data_len = poporon_bch_get_data_length(bch);
    codeword_len = poporon_bch_get_codeword_length(bch);
    parity_bits = codeword_len - data_len;
    data_bytes = (data_len + 7) / 8;
    parity_bytes = (parity_bits + 7) / 8;

    if (size < data_bytes) {
        return false;
    }

    for (i = 0; i < data_bytes && i < 4; i++) {
        data_val |= ((uint32_t)data[i]) << (8 * (data_bytes - 1 - i));
    }

    if (data_len < 32) {
        data_val &= ((uint32_t)1 << data_len) - 1;
    }

    if (!poporon_bch_encode(bch, data_val, &codeword)) {
        return false;
    }

    parity_val = codeword & (((uint32_t)1 << parity_bits) - 1);
    pmemset(parity, 0, parity_bytes);
    for (i = 0; i < parity_bytes && i < 4; i++) {
        parity[parity_bytes - 1 - i] = (uint8_t)(parity_val >> (8 * i));
    }

    return true;
}

extern bool poporon_encode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity)
{
    if (!pprn || !data || !parity) {
        return false;
    }

    switch (pprn->fec_type) {
    case PPLN_FEC_RS:
        return rs_encode(pprn, data, size, parity);
    case PPLN_FEC_LDPC:
        return ldpc_encode(pprn, data, size, parity);
    case PPLN_FEC_BCH:
        return bch_encode(pprn, data, size, parity);
    default:
        return false;
    }
}
