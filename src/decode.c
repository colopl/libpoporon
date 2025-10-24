/*
 * libpoporon - decode.c
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "poporon_internal.h"

static inline bool error_correction_u8(
    poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, uint16_t *syndrome_ptr,
    uint32_t erasure_count, uint32_t *erasure_positions, uint16_t *corrections,
    int16_t padding_length,
    size_t *errors_corrected
) {
    uint32_t iteration_count, polynomial_degree;
    uint16_t error_locator_degree, error_evaluator_degree, polynomial_evaluation;
    uint16_t temp_value, numerator_value, second_numerator, denominator_value, discrepancy;
    uint16_t error_count;
    int16_t i, j, k;
    uint8_t poly_term;
    bool success;
    
    success = true;

    pmemset(&pprn->buffer->error_locator[1], 0, pprn->rs->num_roots * sizeof(pprn->buffer->error_locator[0]));
    pprn->buffer->error_locator[0] = 1;

    if (erasure_count > 0) {
        pprn->buffer->error_locator[1] = pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, pprn->rs->primitive_element * (pprn->rs->gf->field_size - 1 - (erasure_positions[0] + padding_length)))];
        for (i = 1; i < erasure_count; i++) {
            poly_term = gf_mod(pprn->rs->gf, pprn->rs->primitive_element * (pprn->rs->gf->field_size - 1 - (erasure_positions[i] + padding_length)));
            for (j = i + 1; j > 0; j--) {
                temp_value = pprn->rs->gf->exp2log[pprn->buffer->error_locator[j - 1]];
                if (temp_value != pprn->rs->gf->field_size) {
                    pprn->buffer->error_locator[j] ^= pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, poly_term + temp_value)];
                }
            }
        }
    }

    for (i = 0; i < pprn->rs->num_roots + 1; i++) {
        pprn->buffer->coefficients[i] = pprn->rs->gf->exp2log[pprn->buffer->error_locator[i]];
    }

    /* Berlekamp-Massey */
    iteration_count = erasure_count;
    polynomial_degree = erasure_count;
    while (++iteration_count <= pprn->rs->num_roots) {
        discrepancy = 0;
        for (i = 0; i < iteration_count; i++) {
            if ((pprn->buffer->error_locator[i] != 0) && (syndrome_ptr[iteration_count - i - 1] != pprn->rs->gf->field_size)) {
                discrepancy ^= pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, pprn->rs->gf->exp2log[pprn->buffer->error_locator[i]] + syndrome_ptr[iteration_count - i - 1])];
            }
        }
        discrepancy = pprn->rs->gf->exp2log[discrepancy];
        
        if (discrepancy == pprn->rs->gf->field_size) {
            pmemmove(&pprn->buffer->coefficients[1], pprn->buffer->coefficients, pprn->rs->num_roots * sizeof(pprn->buffer->coefficients[0]));
            pprn->buffer->coefficients[0] = pprn->rs->gf->field_size;
        } else {
            pprn->buffer->polynomial[0] = pprn->buffer->error_locator[0];

            for (i = 0; i < pprn->rs->num_roots; i++) {
                if (pprn->buffer->coefficients[i] != pprn->rs->gf->field_size) {
                    pprn->buffer->polynomial[i + 1] = pprn->buffer->error_locator[i + 1] ^ 
                        pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, discrepancy + pprn->buffer->coefficients[i])];
                } else {
                    pprn->buffer->polynomial[i + 1] = pprn->buffer->error_locator[i + 1];
                }
            }

            if (2 * polynomial_degree <= iteration_count + erasure_count - 1) {
                polynomial_degree = iteration_count + erasure_count - polynomial_degree;
                for (i = 0; i <= pprn->rs->num_roots; i++) {
                    pprn->buffer->coefficients[i] = (pprn->buffer->error_locator[i] == 0) ? 
                        pprn->rs->gf->field_size : 
                        gf_mod(pprn->rs->gf, pprn->rs->gf->exp2log[pprn->buffer->error_locator[i]] - discrepancy + pprn->rs->gf->field_size);
                }
            } else {
                pmemmove(&pprn->buffer->coefficients[1], pprn->buffer->coefficients, pprn->rs->num_roots * sizeof(pprn->buffer->coefficients[0]));
                pprn->buffer->coefficients[0] = pprn->rs->gf->field_size;
            }

            pmemcpy(pprn->buffer->error_locator, pprn->buffer->polynomial, (pprn->rs->num_roots + 1) * sizeof(pprn->buffer->polynomial[0]));
        }
    }

    error_locator_degree = 0;
    for (i = 0; i < pprn->rs->num_roots + 1; i++) {
        pprn->buffer->error_locator[i] = pprn->rs->gf->exp2log[pprn->buffer->error_locator[i]];

        if (pprn->buffer->error_locator[i] != pprn->rs->gf->field_size) {
            error_locator_degree = i;
        }
    }

    if (error_locator_degree == 0) {
        return false;
    }

    /* Chien search */
    pmemcpy(&pprn->buffer->register_coefficients[1], &pprn->buffer->error_locator[1], pprn->rs->num_roots * sizeof(pprn->buffer->register_coefficients[0]));
    error_count = 0;
    for (i = 1, k = pprn->buffer->primitive_inverse - 1; i <= pprn->rs->gf->field_size; i++, k = gf_mod(pprn->rs->gf, k + pprn->buffer->primitive_inverse)) {
        polynomial_evaluation = 1;

        for (j = error_locator_degree; j > 0; j--) {
            if (pprn->buffer->register_coefficients[j] != pprn->rs->gf->field_size) {
                pprn->buffer->register_coefficients[j] = gf_mod(pprn->rs->gf, pprn->buffer->register_coefficients[j] + j);
                polynomial_evaluation ^= pprn->rs->gf->log2exp[pprn->buffer->register_coefficients[j]];
            }
        }

        if (polynomial_evaluation != 0)
            continue;

        if (k < padding_length) {
            return false;
        }

        pprn->buffer->error_roots[error_count] = i;
        pprn->buffer->error_locations[error_count] = k;
        if (++error_count == error_locator_degree) {
            break;
        }
    }
    
    if (error_locator_degree != error_count) {
        return false;
    }
    
    /* Forney */
    error_evaluator_degree = error_locator_degree - 1;
    for (i = 0; i <= error_evaluator_degree; i++) {
        temp_value = 0;

        for (j = i; j >= 0; j--) {
            if ((syndrome_ptr[i - j] != pprn->rs->gf->field_size) && (pprn->buffer->error_locator[j] != pprn->rs->gf->field_size)) {
                temp_value ^= pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, syndrome_ptr[i - j] + pprn->buffer->error_locator[j])];
            }
        }

        pprn->buffer->error_evaluator[i] = pprn->rs->gf->exp2log[temp_value];
    }
    *errors_corrected = 0;
    for (j = error_count - 1; j >= 0; j--) {
        numerator_value = 0;

        for (i = error_evaluator_degree; i >= 0; i--) {
            if (pprn->buffer->error_evaluator[i] != pprn->rs->gf->field_size) {
                numerator_value ^= pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, pprn->buffer->error_evaluator[i] + i * pprn->buffer->error_roots[j])];
            }
        }

        if (numerator_value == 0) {
            pprn->buffer->coefficients[j] = 0;
            continue;
        }

        second_numerator = pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, pprn->buffer->error_roots[j] * (pprn->rs->first_consective_root - 1) + pprn->rs->gf->field_size)];
        denominator_value = 0;

        for (i = (error_locator_degree < (pprn->rs->num_roots - 1) ? error_locator_degree : (pprn->rs->num_roots - 1)) & ~1; i >= 0; i -= 2) {
            if (pprn->buffer->error_locator[i + 1] != pprn->rs->gf->field_size) {
                denominator_value ^= pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, pprn->buffer->error_locator[i + 1] + i * pprn->buffer->error_roots[j])];
            }
        }

        pprn->buffer->coefficients[j] = pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, pprn->rs->gf->exp2log[numerator_value] + pprn->rs->gf->exp2log[second_numerator] + pprn->rs->gf->field_size - pprn->rs->gf->exp2log[denominator_value])];
        (*errors_corrected)++;
    }

    /* Validate */
    for (i = 0; i < pprn->rs->num_roots; i++) {
        temp_value = 0;

        for (j = 0; j < error_count; j++) {
            if (pprn->buffer->coefficients[j] == 0) {
                continue;
            }

            k = (pprn->rs->first_consective_root + i) * pprn->rs->primitive_element * (pprn->rs->gf->field_size - pprn->buffer->error_locations[j] - 1);
            temp_value ^= pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, pprn->rs->gf->exp2log[pprn->buffer->coefficients[j]] + k)];
        }

        if (temp_value != pprn->rs->gf->log2exp[syndrome_ptr[i]]) {
            return false;
        }
    }

    /* Correction */
    if (corrections && erasure_positions) {
        for (i = 0; i < error_count; i++) {
            data[erasure_positions[i]] ^= pprn->buffer->coefficients[i];
        }
    } else if (data && parity) {
        for (i = 0; i < error_count; i++) {
            if (pprn->buffer->error_locations[i] < (pprn->rs->gf->field_size - pprn->rs->num_roots)) {
                data[pprn->buffer->error_locations[i] - padding_length] ^= pprn->buffer->coefficients[i];
            }
            else {
                parity[pprn->buffer->error_locations[i] - padding_length - size] ^= pprn->buffer->coefficients[i];
            }
        }
    }

    return true;
}

static inline bool calculate_syndrome_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, uint16_t *syndrome)
{
    int16_t i, j;
    uint16_t syndrome_error_flag;
    
    syndrome_error_flag = 0;

    /* Calculate syndrome from data */
    for (i = 0; i < pprn->rs->num_roots; i++) {
        syndrome[i] = data[0] & ((uint16_t)pprn->rs->gf->field_size);
    }

    for (j = 1; j < size; j++) {
        for (i = 0; i < pprn->rs->num_roots; i++) {
            if (syndrome[i] == 0) {
                syndrome[i] = data[j] & ((uint16_t)pprn->rs->gf->field_size);
            } else {
                syndrome[i] = (data[j] & ((uint16_t)pprn->rs->gf->field_size)) ^
                    pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, pprn->rs->gf->exp2log[syndrome[i]] + (pprn->rs->first_consective_root + i) * pprn->rs->primitive_element)];
            }
        }
    }

    /* Calculate syndrome from parity */
    for (j = 0; j < pprn->rs->num_roots; j++) {
        for (i = 0; i < pprn->rs->num_roots; i++) {
            if (syndrome[i] == 0) {
                syndrome[i] = parity[j] & ((uint16_t)pprn->rs->gf->field_size);
            } else {
                syndrome[i] = (parity[j] & ((uint16_t)pprn->rs->gf->field_size)) ^
                    pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, pprn->rs->gf->exp2log[syndrome[i]] + (pprn->rs->first_consective_root + i) * pprn->rs->primitive_element)];
            }
        }
    }
    
    for (i = 0; i < pprn->rs->num_roots; i++) {
        syndrome_error_flag |= syndrome[i];
        syndrome[i] = pprn->rs->gf->exp2log[syndrome[i]];
    }

    return syndrome_error_flag != 0;
}

static inline int16_t calculate_padding_length(poporon_t *pprn, size_t size)
{
    int16_t padding_length;
    
    padding_length = pprn->rs->gf->field_size - pprn->rs->num_roots - size;
    
    if (padding_length < 0 || padding_length >= pprn->rs->gf->field_size - pprn->rs->num_roots) {
        return -1;
    }
    
    return padding_length;
}

extern bool poporon_decode_u8_with_syndrome(poporon_t *pprn, uint8_t *data, uint8_t *parity, size_t size, uint16_t *syndrome, size_t *corrected_num)
{
    size_t errors_corrected;
    int16_t padding_length;
    uint16_t i;
    bool success, has_errors;

    errors_corrected = 0;
    success = false;
    has_errors = false;

    if (!pprn || !data || !parity || !size || !syndrome) {
        goto finish;
    }

    padding_length = calculate_padding_length(pprn, size);
    if (padding_length < 0) {
        goto finish;
    }

    for (i = 0; i < pprn->rs->num_roots; i++) {
        if (syndrome[i] != pprn->rs->gf->field_size) {
            has_errors = true;
            break;
        }
    }

    if (has_errors && !error_correction_u8(pprn, data, size, parity, syndrome, 0, NULL, NULL, padding_length, &errors_corrected)) {
        goto finish;
    }

    success = true;

finish:
    if (corrected_num) {
        *corrected_num = errors_corrected;
    }

    return success;
}

extern bool poporon_decode_u8_with_erasure(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, poporon_erasure_t *eras, size_t *corrected_num)
{
    size_t errors_corrected;
    int16_t padding_length;
    bool success;

    errors_corrected = 0;
    success = false;

    if (!pprn || !data || !parity || !eras || !size) {
        goto finish;
    }

    padding_length = calculate_padding_length(pprn, size);
    if (padding_length < 0) {
        goto finish;
    }

    success = !calculate_syndrome_u8(pprn, data, size, parity, pprn->buffer->syndrome) 
            || error_correction_u8(pprn, data, size, parity, pprn->buffer->syndrome, eras->erasure_count, eras->erasure_positions, eras->corrections, padding_length, &errors_corrected);

finish:
    if (corrected_num) {
        *corrected_num = errors_corrected;
    }

    return success;
}

extern bool poporon_decode_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, size_t *corrected_num)
{
    size_t errors_corrected;
    int16_t padding_length;
    bool success;
    
    errors_corrected = 0;
    success = false;

    if (!pprn || !data || !parity || !size) {
        goto finish;
    }

    padding_length = calculate_padding_length(pprn, size);
    if (padding_length < 0) {
        goto finish;
    }

    success = !calculate_syndrome_u8(pprn, data, size, parity, pprn->buffer->syndrome)
             || error_correction_u8(pprn, data, size, parity, pprn->buffer->syndrome, 0, NULL, NULL, padding_length, &errors_corrected);

finish:
    if (corrected_num) {
        *corrected_num = errors_corrected;
    }

    return success;
}
