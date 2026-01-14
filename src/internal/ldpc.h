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

#include <poporon/ldpc.h>

#include "common.h"

#define LLR_MAX      ((int16_t)32000)
#define LLR_MIN      ((int16_t)-32000)
#define LLR_INFINITY ((int16_t)30000)

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

struct _poporon_ldpc_t {
    poporon_ldpc_rate_t rate;
    poporon_ldpc_config_t config;
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

    uint8_t *temp_codeword;
    uint8_t *temp_interleaved;
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
