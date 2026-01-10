/*
 * libpoporon - bch.c
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <poporon/bch.h>
#include <poporon/gf.h>

#include "internal/common.h"

#define BCH_MAX_POLY 64
#define BCH_MAX_T    16

struct _poporon_bch_t {
    poporon_gf_t *gf;
    uint8_t correction_capability;
    uint16_t codeword_length;
    uint16_t data_length;
    uint16_t parity_bits;
    uint32_t gen_poly;
    uint16_t gen_poly_deg;
};

static inline int32_t bch_compute_syndromes(poporon_bch_t *bch, uint32_t codeword, uint16_t *syndromes)
{
    poporon_gf_t *gf;
    uint16_t exp_val;
    int32_t i, j, has_nonzero, syndrome_count;

    gf = bch->gf;
    has_nonzero = 0;
    syndrome_count = 2 * bch->correction_capability;

    for (i = 0; i < syndrome_count; i++) {
        syndromes[i] = 0;

        for (j = 0; j < bch->codeword_length; j++) {
            if (codeword & (1U << j)) {
                exp_val = (uint16_t)(((i + 1) * j) % gf->field_size);
                syndromes[i] ^= gf->log2exp[exp_val];
            }
        }

        if (syndromes[i] != 0) {
            has_nonzero = 1;
        }
    }

    return has_nonzero;
}

static inline uint16_t bch_poly_eval(poporon_bch_t *bch, const uint16_t *poly, int32_t degree, uint16_t x)
{
    poporon_gf_t *gf;
    uint16_t sum, log_x, exp_val;
    int32_t i;

    gf = bch->gf;

    if (x == 0) {
        return poly[0];
    }

    sum = 0;
    log_x = gf->exp2log[x];

    for (i = 0; i <= degree; i++) {
        if (poly[i] != 0) {
            exp_val = (gf->exp2log[poly[i]] + (log_x * i) % gf->field_size) % gf->field_size;
            sum ^= gf->log2exp[exp_val];
        }
    }

    return sum;
}

static inline int32_t bch_berlekamp_massey(poporon_bch_t *bch, const uint16_t *syndromes, uint16_t *error_locator)
{
    poporon_gf_t *gf;
    uint16_t current[BCH_MAX_POLY], prev[BCH_MAX_POLY], temp[BCH_MAX_POLY], prev_discrepancy, discrepancy, log_mult,
        multiplier;
    int32_t error_count, shift, syndrome_count, iteration, i;

    gf = bch->gf;
    error_count = 0;
    shift = 1;
    prev_discrepancy = 1;
    syndrome_count = 2 * bch->correction_capability;

    pmemset(current, 0, sizeof(current));
    pmemset(prev, 0, sizeof(prev));
    pmemset(temp, 0, sizeof(temp));
    current[0] = 1;
    prev[0] = 1;

    for (iteration = 0; iteration < syndrome_count; iteration++) {
        discrepancy = syndromes[iteration];

        for (i = 1; i <= error_count; i++) {
            if (current[i] != 0 && syndromes[iteration - i] != 0) {
                uint16_t log_sum = (gf->exp2log[current[i]] + gf->exp2log[syndromes[iteration - i]]) % gf->field_size;
                discrepancy ^= gf->log2exp[log_sum];
            }
        }

        if (discrepancy == 0) {
            shift++;
        } else {
            log_mult = (gf->field_size - gf->exp2log[prev_discrepancy] + gf->exp2log[discrepancy]) % gf->field_size;
            multiplier = gf->log2exp[log_mult];

            if (2 * error_count <= iteration) {
                pmemcpy(temp, current, sizeof(temp));

                for (i = 0; i < BCH_MAX_POLY - shift; i++) {
                    if (prev[i] != 0) {
                        uint16_t log_product = (gf->exp2log[prev[i]] + gf->exp2log[multiplier]) % gf->field_size;
                        current[i + shift] ^= gf->log2exp[log_product];
                    }
                }

                pmemcpy(prev, temp, sizeof(prev));
                error_count = iteration + 1 - error_count;
                prev_discrepancy = discrepancy;
                shift = 1;
            } else {
                for (i = 0; i < BCH_MAX_POLY - shift; i++) {
                    if (prev[i] != 0) {
                        uint16_t log_product = (gf->exp2log[prev[i]] + gf->exp2log[multiplier]) % gf->field_size;
                        current[i + shift] ^= gf->log2exp[log_product];
                    }
                }
                shift++;
            }
        }
    }

    pmemcpy(error_locator, current, BCH_MAX_POLY * sizeof(uint16_t));

    return error_count;
}

static inline int32_t bch_chien_search(poporon_bch_t *bch, const uint16_t *error_locator, int32_t error_count,
                                       uint16_t *error_pos)
{
    poporon_gf_t *gf;
    uint16_t alpha_inv;
    int32_t found = 0, i;

    gf = bch->gf;

    for (i = 0; i < bch->codeword_length; i++) {
        alpha_inv = gf->log2exp[(gf->field_size - i) % gf->field_size];

        if (bch_poly_eval(bch, error_locator, error_count, alpha_inv) == 0) {
            error_pos[found++] = (uint16_t)i;

            if (found >= error_count) {
                break;
            }
        }
    }

    return found;
}

static inline uint32_t bch_get_minimal_polynomial(poporon_gf_t *gf, int32_t exp)
{
    uint32_t binary_poly;
    uint16_t poly[BCH_MAX_POLY], root, log_prod;
    int32_t poly_deg, conjugate, i, j;

    pmemset(poly, 0, sizeof(poly));
    poly[0] = 1;
    poly_deg = 0;

    conjugate = exp;

    do {
        root = gf->log2exp[conjugate];

        for (j = poly_deg; j >= 0; j--) {
            if (j + 1 < BCH_MAX_POLY) {
                poly[j + 1] ^= poly[j];
            }
            if (poly[j] != 0 && root != 0) {
                log_prod = (gf->exp2log[poly[j]] + gf->exp2log[root]) % gf->field_size;
                poly[j] = gf->log2exp[log_prod];
            } else {
                poly[j] = 0;
            }
        }
        poly_deg++;

        conjugate = (conjugate * 2) % gf->field_size;
    } while (conjugate != exp);

    binary_poly = 0;
    for (i = 0; i <= poly_deg; i++) {
        if (poly[i] == 1) {
            binary_poly |= (1U << i);
        }
    }

    return binary_poly;
}

static inline uint32_t bch_poly_multiply_binary(uint32_t a, int32_t deg_a, uint32_t b)
{
    int32_t i;
    uint32_t result = 0;

    for (i = 0; i <= deg_a; i++) {
        if (a & (1U << i)) {
            result ^= (b << i);
        }
    }

    return result;
}

static inline int32_t bch_poly_degree_binary(uint32_t poly)
{
    int32_t deg = 0, i;

    if (poly == 0) {
        return -1;
    }

    for (i = 31; i >= 0; i--) {
        if (poly & (1U << i)) {
            deg = i;
            break;
        }
    }

    return deg;
}

static inline bool bch_build_generator(poporon_bch_t *bch)
{
    poporon_gf_t *gf;
    uint32_t gen, min_poly;
    int32_t gen_deg, i, root_exp, conj, min_deg;
    bool *used;

    gf = bch->gf;
    used = (bool *)pcalloc(gf->field_size + 1, sizeof(bool));

    if (!used) {
        return false;
    }

    gen = 1;
    gen_deg = 0;

    for (i = 1; i <= 2 * bch->correction_capability; i++) {
        root_exp = i % gf->field_size;

        if (used[root_exp]) {
            continue;
        }

        conj = root_exp;
        do {
            used[conj] = true;
            conj = (conj * 2) % gf->field_size;
        } while (conj != root_exp);

        min_poly = bch_get_minimal_polynomial(gf, root_exp);
        min_deg = bch_poly_degree_binary(min_poly);

        gen = bch_poly_multiply_binary(gen, gen_deg, min_poly);
        gen_deg = bch_poly_degree_binary(gen);
    }

    pfree(used);

    bch->gen_poly = gen;
    bch->gen_poly_deg = (uint16_t)gen_deg;
    bch->parity_bits = (uint16_t)gen_deg;
    bch->data_length = bch->codeword_length - bch->parity_bits;

    return true;
}

extern poporon_bch_t *poporon_bch_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                         uint8_t correction_capability)
{
    poporon_bch_t *bch;

    if (symbol_size < 3 || symbol_size > 16) {
        return NULL;
    }

    if (correction_capability < 1 || correction_capability > BCH_MAX_T) {
        return NULL;
    }

    bch = (poporon_bch_t *)pcalloc(1, sizeof(poporon_bch_t));
    if (!bch) {
        return NULL;
    }

    bch->gf = poporon_gf_create(symbol_size, generator_polynomial);
    if (!bch->gf) {
        pfree(bch);
        return NULL;
    }

    bch->correction_capability = correction_capability;
    bch->codeword_length = (uint16_t)((1 << symbol_size) - 1);

    if (!bch_build_generator(bch)) {
        poporon_gf_destroy(bch->gf);
        pfree(bch);
        return NULL;
    }

    return bch;
}

extern void poporon_bch_destroy(poporon_bch_t *bch)
{
    if (!bch) {
        return;
    }

    if (bch->gf) {
        poporon_gf_destroy(bch->gf);
    }

    pfree(bch);
}

extern uint16_t poporon_bch_get_codeword_length(const poporon_bch_t *bch)
{
    return bch ? bch->codeword_length : 0;
}

extern uint16_t poporon_bch_get_data_length(const poporon_bch_t *bch)
{
    return bch ? bch->data_length : 0;
}

extern uint8_t poporon_bch_get_correction_capability(const poporon_bch_t *bch)
{
    return bch ? bch->correction_capability : 0;
}

extern bool poporon_bch_encode(poporon_bch_t *bch, uint32_t data, uint32_t *codeword)
{
    uint32_t shifted, remainder, gen;
    int32_t gen_deg, i;

    if (!bch || !codeword) {
        return false;
    }

    if (data >= (1U << bch->data_length)) {
        return false;
    }

    shifted = data << bch->parity_bits;

    remainder = shifted;
    gen = bch->gen_poly;
    gen_deg = bch->gen_poly_deg;

    for (i = bch->codeword_length - 1; i >= gen_deg; i--) {
        if (remainder & (1U << i)) {
            remainder ^= (gen << (i - gen_deg));
        }
    }

    *codeword = shifted ^ remainder;

    return true;
}

extern bool poporon_bch_decode(poporon_bch_t *bch, uint32_t received, uint32_t *corrected, int32_t *num_errors)
{
    uint32_t corrected_word;
    uint16_t syndromes[BCH_MAX_POLY], error_locator[BCH_MAX_POLY], error_positions[BCH_MAX_T];
    int32_t error_count, found, i;

    if (!bch || !corrected) {
        return false;
    }

    pmemset(syndromes, 0, sizeof(syndromes));
    pmemset(error_locator, 0, sizeof(error_locator));
    pmemset(error_positions, 0, sizeof(error_positions));

    received &= ((1U << bch->codeword_length) - 1);
    *corrected = received;

    if (num_errors) {
        *num_errors = 0;
    }

    if (!bch_compute_syndromes(bch, received, syndromes)) {
        return true;
    }

    error_count = bch_berlekamp_massey(bch, syndromes, error_locator);

    if (error_count > bch->correction_capability) {
        return false;
    }

    found = bch_chien_search(bch, error_locator, error_count, error_positions);

    if (found != error_count) {
        return false;
    }

    corrected_word = received;
    for (i = 0; i < found; i++) {
        corrected_word ^= (1U << error_positions[i]);
    }

    if (bch_compute_syndromes(bch, corrected_word, syndromes)) {
        return false;
    }

    *corrected = corrected_word;

    if (num_errors) {
        *num_errors = found;
    }

    return true;
}

extern uint32_t poporon_bch_extract_data(const poporon_bch_t *bch, uint32_t codeword)
{
    if (!bch) {
        return 0;
    }

    return (codeword >> bch->parity_bits) & ((1U << bch->data_length) - 1);
}
