/*
 * libpoporon - ldpc.h
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_INTERNAL_LDPC_H
#define POPORON_INTERNAL_LDPC_H

#include "common.h"

#define LLR_MAX      ((int16_t)32000)
#define LLR_MIN      ((int16_t)-32000)
#define LLR_INFINITY ((int16_t)30000)

typedef struct {
    poporon_ldpc_matrix_type_t matrix_type;
    uint32_t column_weight;
    bool use_inner_interleave;
    bool use_outer_interleave;
    uint32_t interleave_depth;
    uint32_t lifting_factor;
    uint64_t seed;
} poporon_ldpc_params_t;

poporon_ldpc_t *poporon_ldpc_create(size_t block_size, poporon_ldpc_rate_t rate, const poporon_ldpc_params_t *config);
void poporon_ldpc_destroy(poporon_ldpc_t *ldpc);

bool poporon_ldpc_params_default(poporon_ldpc_params_t *config);
bool poporon_ldpc_params_burst_resistant(poporon_ldpc_params_t *config);

size_t poporon_ldpc_info_size(const poporon_ldpc_t *ldpc);
size_t poporon_ldpc_codeword_size(const poporon_ldpc_t *ldpc);
size_t poporon_ldpc_parity_size(const poporon_ldpc_t *ldpc);

bool poporon_ldpc_encode(poporon_ldpc_t *ldpc, const uint8_t *info, uint8_t *parity);
bool poporon_ldpc_decode_hard(poporon_ldpc_t *ldpc, uint8_t *codeword, uint32_t max_iterations,
                              uint32_t *iterations_used);
bool poporon_ldpc_decode_soft(poporon_ldpc_t *ldpc, const int8_t *llr, uint8_t *codeword, uint32_t max_iterations,
                              uint32_t *iterations_used);

bool poporon_ldpc_check(const poporon_ldpc_t *ldpc, const uint8_t *codeword);
bool poporon_ldpc_has_interleaver(const poporon_ldpc_t *ldpc);

bool poporon_ldpc_interleave(const poporon_ldpc_t *ldpc, const uint8_t *input, uint8_t *output);
bool poporon_ldpc_deinterleave(const poporon_ldpc_t *ldpc, const uint8_t *input, uint8_t *output);

typedef struct {
    uint32_t *row_ptr;
    uint32_t *col_idx;
    uint32_t num_checks;
    uint32_t num_bits;
    uint32_t num_edges;
} sparse_matrix_t;

typedef struct {
    uint32_t *col_ptr;
    uint32_t *row_idx;
    uint32_t *edge_idx;
} column_view_t;

typedef struct {
    int16_t *check_to_var;
    int16_t *var_to_check;
    int16_t *llr_total;
} messages_t;

typedef struct {
    uint32_t *forward;
    uint32_t *inverse;
    size_t size;
    uint32_t depth;
} interleaver_t;

typedef struct {
    uint32_t *forward;
    uint32_t *inverse;
    size_t size;
} outer_interleaver_t;

struct _poporon_ldpc_t {
    poporon_ldpc_rate_t rate;
    poporon_ldpc_params_t config;
    size_t info_bits;
    size_t parity_bits;
    size_t codeword_bits;
    size_t info_bytes;
    size_t parity_bytes;
    size_t codeword_bytes;

    sparse_matrix_t parity_matrix;
    column_view_t parity_matrix_cols;
    messages_t msg;
    interleaver_t interleaver;
    outer_interleaver_t outer_interleaver;

    uint8_t *temp_codeword;
    uint8_t *temp_interleaved;
    uint8_t *temp_outer;
};

static inline int16_t ldpc_saturate(int32_t val)
{
    if (val > LLR_MAX) {
        return LLR_MAX;
    } else if (val < LLR_MIN) {
        return LLR_MIN;
    }

    return (int16_t)val;
}

static inline int16_t ldpc_abs(int16_t val)
{
    return (val < 0) ? -val : val;
}

static inline int16_t ldpc_sign(int16_t val)
{
    return (val < 0) ? -1 : 1;
}

static inline int16_t ldpc_min(int16_t a, int16_t b)
{
    return (a < b) ? a : b;
}

#endif /* POPORON_INTERNAL_LDPC_H */
