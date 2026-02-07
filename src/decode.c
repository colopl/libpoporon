/*
 * libpoporon - decode.c
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

static inline bool error_correction_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity,
                                       uint16_t *syndrome_ptr, uint32_t erasure_count, uint32_t *erasure_positions,
                                       uint16_t *corrections, int16_t padding_length, size_t *errors_corrected)
{
    poporon_rs_t *rs = pprn->ctx.rs.rs;
    decoder_buffer_t *buffer = pprn->ctx.rs.buffer;
    uint32_t iteration_count, polynomial_degree;
    uint16_t error_locator_degree, error_evaluator_degree, polynomial_evaluation, temp_value, numerator_value,
        second_numerator, denominator_value, discrepancy, error_count;
    uint8_t poly_term;
    int32_t location_with_padding;
    int16_t i, j, k;
    bool success = true;

    pmemset(&buffer->error_locator[1], 0, rs->num_roots * sizeof(buffer->error_locator[0]));
    buffer->error_locator[0] = 1;

    if (erasure_count > 0) {
        buffer->error_locator[1] = rs->gf->log2exp[gf_mod(
            rs->gf, rs->primitive_element * (rs->gf->field_size - 1 - (erasure_positions[0] + padding_length)))];
        for (i = 1; i < erasure_count; i++) {
            poly_term = gf_mod(rs->gf, rs->primitive_element *
                                           (rs->gf->field_size - 1 - (erasure_positions[i] + padding_length)));
            for (j = i + 1; j > 0; j--) {
                temp_value = rs->gf->exp2log[buffer->error_locator[j - 1]];
                if (temp_value != rs->gf->field_size) {
                    buffer->error_locator[j] ^= rs->gf->log2exp[gf_mod(rs->gf, poly_term + temp_value)];
                }
            }
        }
    }

    for (i = 0; i < rs->num_roots + 1; i++) {
        buffer->coefficients[i] = rs->gf->exp2log[buffer->error_locator[i]];
    }

    iteration_count = erasure_count;
    polynomial_degree = erasure_count;
    while (++iteration_count <= rs->num_roots) {
        discrepancy = 0;
        for (i = 0; i < iteration_count; i++) {
            if ((buffer->error_locator[i] != 0) && (syndrome_ptr[iteration_count - i - 1] != rs->gf->field_size)) {
                discrepancy ^= rs->gf->log2exp[gf_mod(rs->gf, rs->gf->exp2log[buffer->error_locator[i]] +
                                                                  syndrome_ptr[iteration_count - i - 1])];
            }
        }
        discrepancy = rs->gf->exp2log[discrepancy];

        if (discrepancy == rs->gf->field_size) {
            pmemmove(&buffer->coefficients[1], buffer->coefficients, rs->num_roots * sizeof(buffer->coefficients[0]));
            buffer->coefficients[0] = rs->gf->field_size;
        } else {
            buffer->polynomial[0] = buffer->error_locator[0];

            for (i = 0; i < rs->num_roots; i++) {
                if (buffer->coefficients[i] != rs->gf->field_size) {
                    buffer->polynomial[i + 1] = buffer->error_locator[i + 1] ^
                                                rs->gf->log2exp[gf_mod(rs->gf, discrepancy + buffer->coefficients[i])];
                } else {
                    buffer->polynomial[i + 1] = buffer->error_locator[i + 1];
                }
            }

            if (2 * polynomial_degree <= iteration_count + erasure_count - 1) {
                polynomial_degree = iteration_count + erasure_count - polynomial_degree;
                for (i = 0; i <= rs->num_roots; i++) {
                    buffer->coefficients[i] = (buffer->error_locator[i] == 0)
                                                  ? rs->gf->field_size
                                                  : gf_mod(rs->gf, rs->gf->exp2log[buffer->error_locator[i]] -
                                                                       discrepancy + rs->gf->field_size);
                }
            } else {
                pmemmove(&buffer->coefficients[1], buffer->coefficients,
                         rs->num_roots * sizeof(buffer->coefficients[0]));
                buffer->coefficients[0] = rs->gf->field_size;
            }

            pmemcpy(buffer->error_locator, buffer->polynomial, (rs->num_roots + 1) * sizeof(buffer->polynomial[0]));
        }
    }

    error_locator_degree = 0;

    for (i = 0; i < rs->num_roots + 1; i++) {
        buffer->error_locator[i] = rs->gf->exp2log[buffer->error_locator[i]];

        if (buffer->error_locator[i] != rs->gf->field_size) {
            error_locator_degree = i;
        }
    }

    if (error_locator_degree == 0) {
        return false;
    }

    pmemcpy(&buffer->register_coefficients[1], &buffer->error_locator[1],
            rs->num_roots * sizeof(buffer->register_coefficients[0]));

    error_count = 0;

    for (i = 1, k = buffer->primitive_inverse - 1; i <= rs->gf->field_size;
         i++, k = gf_mod(rs->gf, k + buffer->primitive_inverse)) {
        polynomial_evaluation = 1;

        for (j = error_locator_degree; j > 0; j--) {
            if (buffer->register_coefficients[j] != rs->gf->field_size) {
                buffer->register_coefficients[j] = gf_mod(rs->gf, buffer->register_coefficients[j] + j);
                polynomial_evaluation ^= rs->gf->log2exp[buffer->register_coefficients[j]];
            }
        }

        if (polynomial_evaluation != 0) {
            continue;
        }

        if (k < padding_length) {
            return false;
        }

        buffer->error_roots[error_count] = i;
        buffer->error_locations[error_count] = k;
        if (++error_count == error_locator_degree) {
            break;
        }
    }

    if (error_locator_degree != error_count) {
        return false;
    }

    error_evaluator_degree = error_locator_degree - 1;
    for (i = 0; i <= error_evaluator_degree; i++) {
        temp_value = 0;

        for (j = i; j >= 0; j--) {
            if ((syndrome_ptr[i - j] != rs->gf->field_size) && (buffer->error_locator[j] != rs->gf->field_size)) {
                temp_value ^= rs->gf->log2exp[gf_mod(rs->gf, syndrome_ptr[i - j] + buffer->error_locator[j])];
            }
        }

        buffer->error_evaluator[i] = rs->gf->exp2log[temp_value];
    }
    *errors_corrected = 0;
    for (j = error_count - 1; j >= 0; j--) {
        numerator_value = 0;

        for (i = error_evaluator_degree; i >= 0; i--) {
            if (buffer->error_evaluator[i] != rs->gf->field_size) {
                numerator_value ^=
                    rs->gf->log2exp[gf_mod(rs->gf, buffer->error_evaluator[i] + i * buffer->error_roots[j])];
            }
        }

        if (numerator_value == 0) {
            buffer->coefficients[j] = 0;
            continue;
        }

        second_numerator = rs->gf->log2exp[gf_mod(rs->gf, buffer->error_roots[j] * (rs->first_consecutive_root - 1) +
                                                              rs->gf->field_size)];
        denominator_value = 0;

        for (i = (error_locator_degree < (rs->num_roots - 1) ? error_locator_degree : (rs->num_roots - 1)) & ~1; i >= 0;
             i -= 2) {
            if (buffer->error_locator[i + 1] != rs->gf->field_size) {
                denominator_value ^=
                    rs->gf->log2exp[gf_mod(rs->gf, buffer->error_locator[i + 1] + i * buffer->error_roots[j])];
            }
        }

        buffer->coefficients[j] =
            rs->gf->log2exp[gf_mod(rs->gf, rs->gf->exp2log[numerator_value] + rs->gf->exp2log[second_numerator] +
                                               rs->gf->field_size - rs->gf->exp2log[denominator_value])];
        (*errors_corrected)++;
    }

    for (i = 0; i < rs->num_roots; i++) {
        temp_value = 0;

        for (j = 0; j < error_count; j++) {
            if (buffer->coefficients[j] == 0) {
                continue;
            }

            k = (rs->first_consecutive_root + i) * rs->primitive_element *
                (rs->gf->field_size - buffer->error_locations[j] - 1);
            temp_value ^= rs->gf->log2exp[gf_mod(rs->gf, rs->gf->exp2log[buffer->coefficients[j]] + k)];
        }

        if (temp_value != rs->gf->log2exp[syndrome_ptr[i]]) {
            return false;
        }
    }

    if (corrections && erasure_positions) {
        for (i = 0; i < error_count; i++) {
            data[erasure_positions[i]] ^= buffer->coefficients[i];
        }
    } else if (data && parity) {
        for (i = 0; i < error_count; i++) {
            location_with_padding = (int32_t)buffer->error_locations[i] - (int32_t)padding_length;
            if (location_with_padding >= 0 && location_with_padding < (int32_t)size) {
                data[location_with_padding] ^= buffer->coefficients[i];
            } else if (location_with_padding >= (int32_t)size &&
                       location_with_padding < (int32_t)(size + rs->num_roots)) {
                parity[location_with_padding - size] ^= buffer->coefficients[i];
            } else {
                return false;
            }
        }
    }

    return true;
}

static inline bool calculate_syndrome_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity,
                                         uint16_t *syndrome)
{
    poporon_rs_t *rs = pprn->ctx.rs.rs;

#if POPORON_USE_SIMD && defined(POPORON_SIMD_AVX2)
    __m256i syndrome_vec, result_vec;
    uint16_t syndrome_error_flag, data_val, result[16], k;
    int16_t i, j, num_roots_aligned;

    syndrome_error_flag = 0;
    num_roots_aligned = (rs->num_roots) & ~15;

    for (i = 0; i < rs->num_roots; i++) {
        syndrome[i] = data[0] & ((uint16_t)rs->gf->field_size);
    }

    for (j = 1; j < size; j++) {
        data_val = data[j] & ((uint16_t)rs->gf->field_size);

        for (i = 0; i < num_roots_aligned; i += 16) {
            for (k = 0; k < 16; k++) {
                if (syndrome[i + k] == 0) {
                    result[k] = data_val;
                } else {
                    result[k] =
                        data_val ^
                        rs->gf
                            ->log2exp[gf_mod(rs->gf, rs->gf->exp2log[syndrome[i + k]] +
                                                         (rs->first_consecutive_root + i + k) * rs->primitive_element)];
                }
            }

            syndrome_vec = _mm256_loadu_si256((__m256i *)&syndrome[i]);
            result_vec = _mm256_loadu_si256((__m256i *)result);
            _mm256_storeu_si256((__m256i *)&syndrome[i], result_vec);
        }

        for (i = num_roots_aligned; i < rs->num_roots; i++) {
            if (syndrome[i] == 0) {
                syndrome[i] = data_val;
            } else {
                syndrome[i] =
                    data_val ^
                    rs->gf->log2exp[gf_mod(rs->gf, rs->gf->exp2log[syndrome[i]] +
                                                       (rs->first_consecutive_root + i) * rs->primitive_element)];
            }
        }
    }

    for (j = 0; j < rs->num_roots; j++) {
        for (i = 0; i < rs->num_roots; i++) {
            if (syndrome[i] == 0) {
                syndrome[i] = parity[j] & ((uint16_t)rs->gf->field_size);
            } else {
                syndrome[i] =
                    (parity[j] & ((uint16_t)rs->gf->field_size)) ^
                    rs->gf->log2exp[gf_mod(rs->gf, rs->gf->exp2log[syndrome[i]] +
                                                       (rs->first_consecutive_root + i) * rs->primitive_element)];
            }
        }
    }

    for (i = 0; i < rs->num_roots; i++) {
        syndrome_error_flag |= syndrome[i];
        syndrome[i] = rs->gf->exp2log[syndrome[i]];
    }

    return syndrome_error_flag != 0;

#elif POPORON_USE_SIMD && (defined(POPORON_SIMD_NEON) || defined(POPORON_SIMD_WASM))
    uint16_t syndrome_error_flag, data_val, result[8], k;
    int16_t i, j, num_roots_aligned;
#if defined(POPORON_SIMD_NEON)
    uint16x8_t result_vec;
#elif defined(POPORON_SIMD_WASM)
    v128_t result_vec;
#endif

    syndrome_error_flag = 0;
    num_roots_aligned = (rs->num_roots) & ~7;

    for (i = 0; i < rs->num_roots; i++) {
        syndrome[i] = data[0] & ((uint16_t)rs->gf->field_size);
    }

    for (j = 1; j < size; j++) {
        data_val = data[j] & ((uint16_t)rs->gf->field_size);

        for (i = 0; i < num_roots_aligned; i += 8) {
            for (k = 0; k < 8; k++) {
                if (syndrome[i + k] == 0) {
                    result[k] = data_val;
                } else {
                    result[k] =
                        data_val ^
                        rs->gf
                            ->log2exp[gf_mod(rs->gf, rs->gf->exp2log[syndrome[i + k]] +
                                                         (rs->first_consecutive_root + i + k) * rs->primitive_element)];
                }
            }

#if defined(POPORON_SIMD_NEON)
            result_vec = vld1q_u16(result);
            vst1q_u16(&syndrome[i], result_vec);
#elif defined(POPORON_SIMD_WASM)
            result_vec = wasm_v128_load(result);
            wasm_v128_store(&syndrome[i], result_vec);
#endif
        }

        for (i = num_roots_aligned; i < rs->num_roots; i++) {
            if (syndrome[i] == 0) {
                syndrome[i] = data_val;
            } else {
                syndrome[i] =
                    data_val ^
                    rs->gf->log2exp[gf_mod(rs->gf, rs->gf->exp2log[syndrome[i]] +
                                                       (rs->first_consecutive_root + i) * rs->primitive_element)];
            }
        }
    }

    for (j = 0; j < rs->num_roots; j++) {
        for (i = 0; i < rs->num_roots; i++) {
            if (syndrome[i] == 0) {
                syndrome[i] = parity[j] & ((uint16_t)rs->gf->field_size);
            } else {
                syndrome[i] =
                    (parity[j] & ((uint16_t)rs->gf->field_size)) ^
                    rs->gf->log2exp[gf_mod(rs->gf, rs->gf->exp2log[syndrome[i]] +
                                                       (rs->first_consecutive_root + i) * rs->primitive_element)];
            }
        }
    }

    for (i = 0; i < rs->num_roots; i++) {
        syndrome_error_flag |= syndrome[i];
        syndrome[i] = rs->gf->exp2log[syndrome[i]];
    }

    return syndrome_error_flag != 0;

#else
    int16_t i, j;
    uint16_t syndrome_error_flag = 0;

    for (i = 0; i < rs->num_roots; i++) {
        syndrome[i] = data[0] & ((uint16_t)rs->gf->field_size);
    }

    for (j = 1; j < size; j++) {
        for (i = 0; i < rs->num_roots; i++) {
            if (syndrome[i] == 0) {
                syndrome[i] = data[j] & ((uint16_t)rs->gf->field_size);
            } else {
                syndrome[i] =
                    (data[j] & ((uint16_t)rs->gf->field_size)) ^
                    rs->gf->log2exp[gf_mod(rs->gf, rs->gf->exp2log[syndrome[i]] +
                                                       (rs->first_consecutive_root + i) * rs->primitive_element)];
            }
        }
    }

    for (j = 0; j < rs->num_roots; j++) {
        for (i = 0; i < rs->num_roots; i++) {
            if (syndrome[i] == 0) {
                syndrome[i] = parity[j] & ((uint16_t)rs->gf->field_size);
            } else {
                syndrome[i] =
                    (parity[j] & ((uint16_t)rs->gf->field_size)) ^
                    rs->gf->log2exp[gf_mod(rs->gf, rs->gf->exp2log[syndrome[i]] +
                                                       (rs->first_consecutive_root + i) * rs->primitive_element)];
            }
        }
    }

    for (i = 0; i < rs->num_roots; i++) {
        syndrome_error_flag |= syndrome[i];
        syndrome[i] = rs->gf->exp2log[syndrome[i]];
    }

    return syndrome_error_flag != 0;
#endif
}

static inline int16_t calculate_padding_length(poporon_rs_t *rs, size_t size)
{
    int16_t padding_length;

    padding_length = rs->gf->field_size - rs->num_roots - size;

    if (padding_length < 0 || padding_length >= rs->gf->field_size - rs->num_roots) {
        return -1;
    }

    return padding_length;
}

static bool rs_decode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, size_t *corrected_num)
{
    poporon_rs_t *rs = pprn->ctx.rs.rs;
    poporon_erasure_t *eras;
    decoder_buffer_t *buffer = pprn->ctx.rs.buffer;
    uint16_t i, *syndrome;
    int16_t padding_length;
    size_t errors_corrected = 0;
    bool success = false;

    padding_length = calculate_padding_length(rs, size);
    if (padding_length < 0) {
        goto finish;
    }

    if (pprn->ctx.rs.ext_syndrome) {
        syndrome = pprn->ctx.rs.ext_syndrome;
        bool has_errors = false;
        for (i = 0; i < rs->num_roots; i++) {
            if (syndrome[i] != rs->gf->field_size) {
                has_errors = true;
                break;
            }
        }

        if (has_errors && !error_correction_u8(pprn, data, size, parity, syndrome, 0, NULL, NULL, padding_length,
                                               &errors_corrected)) {
            goto finish;
        }

        success = true;

        goto finish;
    }

    if (pprn->ctx.rs.erasure) {
        eras = pprn->ctx.rs.erasure;
        success = !calculate_syndrome_u8(pprn, data, size, parity, buffer->syndrome) ||
                  error_correction_u8(pprn, data, size, parity, buffer->syndrome, eras->erasure_count,
                                      eras->erasure_positions, eras->corrections, padding_length, &errors_corrected);

        goto finish;
    }

    success = !calculate_syndrome_u8(pprn, data, size, parity, buffer->syndrome) ||
              error_correction_u8(pprn, data, size, parity, buffer->syndrome, 0, NULL, NULL, padding_length,
                                  &errors_corrected);

finish:
    pprn->ctx.rs.last_corrected = errors_corrected;

    if (corrected_num) {
        *corrected_num = errors_corrected;
    }

    return success;
}

static bool ldpc_decode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, size_t *corrected_num)
{
    poporon_ldpc_t *ldpc = pprn->ctx.ldpc.ldpc;
    uint32_t iterations_used = 0;
    uint8_t *codeword, *temp;
    size_t i;
    bool ok;

    if (size != ldpc->info_bytes) {
        return false;
    }

    if (ldpc->config.use_inner_interleave && ldpc->temp_interleaved) {
        codeword = ldpc->temp_interleaved;
    } else {
        codeword = ldpc->temp_codeword;
    }
    pmemcpy(codeword, data, ldpc->info_bytes);
    pmemcpy(codeword + ldpc->info_bytes, parity, ldpc->parity_bytes);

    if (pprn->ctx.ldpc.use_soft_decode && pprn->ctx.ldpc.soft_llr) {
        ok = poporon_ldpc_decode_soft(ldpc, pprn->ctx.ldpc.soft_llr, codeword, pprn->ctx.ldpc.max_iterations,
                                      &iterations_used);
    } else {
        ok = poporon_ldpc_decode_hard(ldpc, codeword, pprn->ctx.ldpc.max_iterations, &iterations_used);
    }

    pprn->ctx.ldpc.last_iterations = iterations_used;

    if (!ok) {
        return false;
    }

    if (ldpc->config.use_outer_interleave && ldpc->outer_interleaver.inverse) {
        temp = ldpc->temp_outer;
        if (!temp) {
            return false;
        }
        for (i = 0; i < ldpc->info_bytes; i++) {
            temp[ldpc->outer_interleaver.inverse[i]] = codeword[i];
        }
        pmemcpy(data, temp, ldpc->info_bytes);
    } else {
        pmemcpy(data, codeword, ldpc->info_bytes);
    }

    if (corrected_num) {
        *corrected_num = iterations_used;
    }

    return true;
}

static bool bch_decode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, size_t *corrected_num)
{
    poporon_bch_t *bch = pprn->ctx.bch.bch;
    uint32_t data_val = 0, parity_val = 0, received = 0, corrected = 0, corrected_data;
    uint16_t i, data_len, codeword_len, parity_bits, data_bytes, parity_bytes_len;
    int32_t num_errors = 0;

    data_len = poporon_bch_get_data_length(bch);
    codeword_len = poporon_bch_get_codeword_length(bch);
    parity_bits = codeword_len - data_len;
    data_bytes = (data_len + 7) / 8;
    parity_bytes_len = (parity_bits + 7) / 8;

    if (size < data_bytes) {
        return false;
    }

    for (i = 0; i < data_bytes && i < 4; i++) {
        data_val |= ((uint32_t)data[i]) << (8 * (data_bytes - 1 - i));
    }

    if (data_len < 32) {
        data_val &= ((uint32_t)1 << data_len) - 1;
    }

    for (i = 0; i < parity_bytes_len && i < 4; i++) {
        parity_val |= ((uint32_t)parity[i]) << (8 * (parity_bytes_len - 1 - i));
    }

    if (parity_bits < 32) {
        parity_val &= ((uint32_t)1 << parity_bits) - 1;
    }

    received = (data_val << parity_bits) | parity_val;

    if (!poporon_bch_decode(bch, received, &corrected, &num_errors)) {
        pprn->ctx.bch.last_num_errors = -1;
        return false;
    }

    pprn->ctx.bch.last_num_errors = num_errors;

    corrected_data = poporon_bch_extract_data(bch, corrected);
    for (i = 0; i < data_bytes && i < 4; i++) {
        data[data_bytes - 1 - i] = (uint8_t)(corrected_data >> (8 * i));
    }

    if (corrected_num) {
        *corrected_num = (num_errors > 0) ? (size_t)num_errors : 0;
    }

    return true;
}

extern bool poporon_decode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, size_t *corrected_num)
{
    if (!pprn || !data || !parity || !size) {
        return false;
    }

    switch (pprn->fec_type) {
    case PPLN_FEC_RS:
        return rs_decode(pprn, data, size, parity, corrected_num);
    case PPLN_FEC_LDPC:
        return ldpc_decode(pprn, data, size, parity, corrected_num);
    case PPLN_FEC_BCH:
        return bch_decode(pprn, data, size, parity, corrected_num);
    default:
        return false;
    }
}
