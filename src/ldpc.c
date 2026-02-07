/*
 * libpoporon - ldpc.c
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "internal/ldpc.h"

#include <poporon/rng.h>

#define MIN_BLOCK_SIZE 32
#define MAX_BLOCK_SIZE 8192

#define DEFAULT_COL_WEIGHT         3
#define BURST_RESISTANT_COL_WEIGHT 6

#define MIN_COL_WEIGHT 3
#define MAX_COL_WEIGHT 8

#define DEFAULT_MAX_ITERATIONS 50

#define BURST_RESISTANT_MAX_ITERATIONS 100

#define LLR_SCALE_FACTOR 256

#define AUTO_INTERLEAVE_DEPTH_DIVISOR 4

#define AUTO_LIFTING_FACTOR_DIVISOR 8
#define MIN_LIFTING_FACTOR          4
#define MAX_LIFTING_FACTOR          256

#define MINSUM_ALPHA_NUMERATOR   15
#define MINSUM_ALPHA_DENOMINATOR 16

static inline void get_rate_params(poporon_ldpc_rate_t rate, uint32_t *info_num, uint32_t *parity_num)
{
    switch (rate) {
    case PPRN_LDPC_RATE_1_3:
        *info_num = 1;
        *parity_num = 2;
        break;
    case PPRN_LDPC_RATE_1_2:
        *info_num = 1;
        *parity_num = 1;
        break;
    case PPRN_LDPC_RATE_2_3:
        *info_num = 2;
        *parity_num = 1;
        break;
    case PPRN_LDPC_RATE_3_4:
        *info_num = 3;
        *parity_num = 1;
        break;
    case PPRN_LDPC_RATE_4_5:
        *info_num = 4;
        *parity_num = 1;
        break;
    case PPRN_LDPC_RATE_5_6:
        *info_num = 5;
        *parity_num = 1;
        break;
    default:
        *info_num = 1;
        *parity_num = 1;
    }
}

static inline uint8_t get_bit(const uint8_t *data, size_t bit_idx)
{
    return (data[bit_idx / 8] >> (7 - (bit_idx % 8))) & 1;
}

static inline void set_bit(uint8_t *data, size_t bit_idx, uint8_t value)
{
    size_t byte_idx = bit_idx / 8;
    uint8_t bit_mask = 1 << (7 - (bit_idx % 8));

    if (value) {
        data[byte_idx] |= bit_mask;
    } else {
        data[byte_idx] &= ~bit_mask;
    }
}

static inline void interleave_bits(const poporon_ldpc_t *ldpc, const uint8_t *input, uint8_t *output)
{
    uint8_t bit;
    size_t i;

    if (!ldpc->interleaver.forward) {
        pmemcpy(output, input, ldpc->codeword_bytes);
        return;
    }

    pmemset(output, 0, ldpc->codeword_bytes);
    for (i = 0; i < ldpc->codeword_bits; i++) {
        bit = get_bit(input, i);
        set_bit(output, ldpc->interleaver.forward[i], bit);
    }
}

static inline void deinterleave_bits(const poporon_ldpc_t *ldpc, const uint8_t *input, uint8_t *output)
{
    uint8_t bit;
    size_t i;

    if (!ldpc->interleaver.inverse) {
        pmemcpy(output, input, ldpc->codeword_bytes);
        return;
    }

    pmemset(output, 0, ldpc->codeword_bytes);
    for (i = 0; i < ldpc->codeword_bits; i++) {
        bit = get_bit(input, i);
        set_bit(output, ldpc->interleaver.inverse[i], bit);
    }
}

static inline void interleave_llr(const poporon_ldpc_t *ldpc, const int8_t *input, int8_t *output)
{
    size_t i;

    if (!ldpc->interleaver.forward) {
        pmemcpy(output, input, ldpc->codeword_bits);
        return;
    }

    for (i = 0; i < ldpc->codeword_bits; i++) {
        output[ldpc->interleaver.forward[i]] = input[i];
    }
}

static inline void deinterleave_llr(const poporon_ldpc_t *ldpc, const int8_t *input, int8_t *output)
{
    size_t i;

    if (!ldpc->interleaver.inverse) {
        pmemcpy(output, input, ldpc->codeword_bits);
        return;
    }

    for (i = 0; i < ldpc->codeword_bits; i++) {
        output[ldpc->interleaver.inverse[i]] = input[i];
    }
}

static inline bool build_interleaver(poporon_ldpc_t *ldpc)
{
    poporon_rng_t *rng;
    uint32_t depth, width, temp, seed, rval, *col_perm;
    size_t i, j, row, col, interleaved_pos;

    if (!ldpc->config.use_inner_interleave) {
        ldpc->interleaver.forward = NULL;
        ldpc->interleaver.inverse = NULL;
        ldpc->interleaver.size = 0;
        ldpc->interleaver.depth = 0;
        return true;
    }

    depth = ldpc->config.interleave_depth;
    if (depth == 0) {
        depth = (uint32_t)(ldpc->codeword_bits / AUTO_INTERLEAVE_DEPTH_DIVISOR);
        if (depth < 8) {
            depth = 8;
        }
        if (depth > 256) {
            depth = 256;
        }
    }

    width = (uint32_t)((ldpc->codeword_bits + depth - 1) / depth);

    ldpc->interleaver.size = ldpc->codeword_bits;
    ldpc->interleaver.depth = depth;

    ldpc->interleaver.forward = (uint32_t *)pmalloc(ldpc->codeword_bits * sizeof(uint32_t));
    ldpc->interleaver.inverse = (uint32_t *)pmalloc(ldpc->codeword_bits * sizeof(uint32_t));

    if (!ldpc->interleaver.forward || !ldpc->interleaver.inverse) {
        return false;
    }

    col_perm = (uint32_t *)pmalloc(width * sizeof(uint32_t));
    if (!col_perm) {
        return false;
    }

    for (i = 0; i < width; i++) {
        col_perm[i] = (uint32_t)i;
    }

    seed = (uint32_t)(ldpc->config.seed ^ (uint64_t)ldpc->codeword_bits);
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    if (!rng) {
        pfree(col_perm);
        return false;
    }

    for (i = width - 1; i > 0; i--) {
        poporon_rng_next(rng, &rval, sizeof(rval));
        j = rval % (i + 1);
        temp = col_perm[i];
        col_perm[i] = col_perm[j];
        col_perm[j] = temp;
    }
    poporon_rng_destroy(rng);

    for (i = 0; i < ldpc->codeword_bits; i++) {
        row = i / width;
        col = i % width;

        if (row < depth) {
            interleaved_pos = col_perm[col] * depth + row;
            if (interleaved_pos < ldpc->codeword_bits) {
                ldpc->interleaver.forward[i] = (uint32_t)interleaved_pos;
            } else {
                ldpc->interleaver.forward[i] = (uint32_t)i;
            }
        } else {
            ldpc->interleaver.forward[i] = (uint32_t)i;
        }
    }

    for (i = 0; i < ldpc->codeword_bits; i++) {
        ldpc->interleaver.inverse[ldpc->interleaver.forward[i]] = (uint32_t)i;
    }

    pfree(col_perm);
    return true;
}

static inline bool build_outer_interleaver(poporon_ldpc_t *ldpc)
{
    poporon_rng_t *rng;
    uint32_t seed, rval, temp;
    size_t i, j;

    if (!ldpc->config.use_outer_interleave) {
        ldpc->outer_interleaver.forward = NULL;
        ldpc->outer_interleaver.inverse = NULL;
        ldpc->outer_interleaver.size = 0;
        return true;
    }

    ldpc->outer_interleaver.size = ldpc->info_bytes;
    ldpc->outer_interleaver.forward = (uint32_t *)pmalloc(ldpc->info_bytes * sizeof(uint32_t));
    ldpc->outer_interleaver.inverse = (uint32_t *)pmalloc(ldpc->info_bytes * sizeof(uint32_t));

    if (!ldpc->outer_interleaver.forward || !ldpc->outer_interleaver.inverse) {
        return false;
    }

    for (i = 0; i < ldpc->info_bytes; i++) {
        ldpc->outer_interleaver.forward[i] = (uint32_t)i;
    }

    seed = (uint32_t)(ldpc->config.seed ^ (uint64_t)(ldpc->info_bits ^ 0xDEADBEEF));
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    if (!rng) {
        return false;
    }

    for (i = ldpc->info_bytes - 1; i > 0; i--) {
        poporon_rng_next(rng, &rval, sizeof(rval));
        j = rval % (i + 1);
        temp = ldpc->outer_interleaver.forward[i];
        ldpc->outer_interleaver.forward[i] = ldpc->outer_interleaver.forward[j];
        ldpc->outer_interleaver.forward[j] = temp;
    }
    poporon_rng_destroy(rng);

    for (i = 0; i < ldpc->info_bytes; i++) {
        ldpc->outer_interleaver.inverse[ldpc->outer_interleaver.forward[i]] = (uint32_t)i;
    }

    return true;
}

static inline bool build_parity_check_matrix_random(poporon_ldpc_t *ldpc, uint32_t col_weight)
{
    poporon_rng_t *rng;
    uint32_t col, seed, rval, *col_counts;
    size_t i, j, target_row, num_info_edges, num_parity_edges, total_edges, parity_col, idx;

    ldpc->parity_matrix.num_bits = (uint32_t)ldpc->codeword_bits;
    ldpc->parity_matrix.num_checks = (uint32_t)ldpc->parity_bits;

    num_info_edges = ldpc->info_bits * col_weight;
    num_parity_edges = ldpc->parity_bits * 2 - 1;
    total_edges = num_info_edges + num_parity_edges;

    ldpc->parity_matrix.num_edges = (uint32_t)total_edges;

    ldpc->parity_matrix.row_ptr = (uint32_t *)pcalloc(ldpc->parity_matrix.num_checks + 1, sizeof(uint32_t));
    ldpc->parity_matrix.col_idx = (uint32_t *)pmalloc(ldpc->parity_matrix.num_edges * sizeof(uint32_t));

    if (!ldpc->parity_matrix.row_ptr || !ldpc->parity_matrix.col_idx) {
        return false;
    }

    col_counts = (uint32_t *)pcalloc(ldpc->parity_matrix.num_checks, sizeof(uint32_t));
    if (!col_counts) {
        return false;
    }

    seed = (uint32_t)ldpc->config.seed;
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    if (!rng) {
        pfree(col_counts);
        return false;
    }

    for (i = 0; i < ldpc->info_bits; i++) {
        for (j = 0; j < col_weight; j++) {
            poporon_rng_next(rng, &rval, sizeof(rval));
            target_row = rval % ldpc->parity_bits;
            col_counts[target_row]++;
        }
    }
    poporon_rng_destroy(rng);

    for (i = 0; i < ldpc->parity_bits; i++) {
        if (i == 0) {
            col_counts[i] += 1;
        } else {
            col_counts[i] += 2;
        }
    }

    ldpc->parity_matrix.row_ptr[0] = 0;
    for (i = 0; i < ldpc->parity_matrix.num_checks; i++) {
        ldpc->parity_matrix.row_ptr[i + 1] = ldpc->parity_matrix.row_ptr[i] + col_counts[i];
    }

    pmemset(col_counts, 0, ldpc->parity_matrix.num_checks * sizeof(uint32_t));

    seed = (uint32_t)ldpc->config.seed;
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    if (!rng) {
        pfree(col_counts);
        return false;
    }

    for (i = 0; i < ldpc->info_bits; i++) {
        for (j = 0; j < col_weight; j++) {
            poporon_rng_next(rng, &rval, sizeof(rval));
            target_row = rval % ldpc->parity_bits;
            ldpc->parity_matrix.col_idx[ldpc->parity_matrix.row_ptr[target_row] + col_counts[target_row]] = (uint32_t)i;
            col_counts[target_row]++;
        }
    }

    for (i = 0; i < ldpc->parity_bits; i++) {
        parity_col = ldpc->info_bits + i;

        if (i > 0) {
            ldpc->parity_matrix.col_idx[ldpc->parity_matrix.row_ptr[i] + col_counts[i]] =
                (uint32_t)(ldpc->info_bits + i - 1);
            col_counts[i]++;
        }

        ldpc->parity_matrix.col_idx[ldpc->parity_matrix.row_ptr[i] + col_counts[i]] = (uint32_t)parity_col;
        col_counts[i]++;
    }

    poporon_rng_destroy(rng);
    pfree(col_counts);

    ldpc->parity_matrix_cols.col_ptr = (uint32_t *)pcalloc(ldpc->parity_matrix.num_bits + 1, sizeof(uint32_t));
    ldpc->parity_matrix_cols.row_idx = (uint32_t *)pmalloc(ldpc->parity_matrix.num_edges * sizeof(uint32_t));
    ldpc->parity_matrix_cols.edge_idx = (uint32_t *)pmalloc(ldpc->parity_matrix.num_edges * sizeof(uint32_t));

    if (!ldpc->parity_matrix_cols.col_ptr || !ldpc->parity_matrix_cols.row_idx || !ldpc->parity_matrix_cols.edge_idx) {
        return false;
    }

    col_counts = (uint32_t *)pcalloc(ldpc->parity_matrix.num_bits, sizeof(uint32_t));
    if (!col_counts) {
        return false;
    }

    for (i = 0; i < ldpc->parity_matrix.num_checks; i++) {
        for (j = ldpc->parity_matrix.row_ptr[i]; j < ldpc->parity_matrix.row_ptr[i + 1]; j++) {
            col_counts[ldpc->parity_matrix.col_idx[j]]++;
        }
    }

    ldpc->parity_matrix_cols.col_ptr[0] = 0;
    for (i = 0; i < ldpc->parity_matrix.num_bits; i++) {
        ldpc->parity_matrix_cols.col_ptr[i + 1] = ldpc->parity_matrix_cols.col_ptr[i] + col_counts[i];
        col_counts[i] = 0;
    }

    for (i = 0; i < ldpc->parity_matrix.num_checks; i++) {
        for (j = ldpc->parity_matrix.row_ptr[i]; j < ldpc->parity_matrix.row_ptr[i + 1]; j++) {
            col = ldpc->parity_matrix.col_idx[j];
            idx = ldpc->parity_matrix_cols.col_ptr[col] + col_counts[col];
            ldpc->parity_matrix_cols.row_idx[idx] = (uint32_t)i;
            ldpc->parity_matrix_cols.edge_idx[idx] = (uint32_t)j;
            col_counts[col]++;
        }
    }

    pfree(col_counts);

    return true;
}

static inline bool build_parity_check_matrix_qc(poporon_ldpc_t *ldpc, uint32_t col_weight)
{
    poporon_rng_t *rng;
    uint32_t seed, rval;
    uint32_t col, lifting_factor, base_rows, base_info_cols, *col_counts, shift, row_in_block, block_row, base_col,
        pos_in_block;
    size_t i, j, target_row, num_info_edges, num_parity_edges, total_edges, parity_col, idx;

    ldpc->parity_matrix.num_bits = (uint32_t)ldpc->codeword_bits;
    ldpc->parity_matrix.num_checks = (uint32_t)ldpc->parity_bits;

    lifting_factor = ldpc->config.lifting_factor;
    if (lifting_factor == 0) {
        lifting_factor = (uint32_t)(ldpc->parity_bits / AUTO_LIFTING_FACTOR_DIVISOR);
        if (lifting_factor < MIN_LIFTING_FACTOR) {
            lifting_factor = MIN_LIFTING_FACTOR;
        }
        if (lifting_factor > MAX_LIFTING_FACTOR) {
            lifting_factor = MAX_LIFTING_FACTOR;
        }
        while ((lifting_factor & (lifting_factor - 1)) != 0) {
            lifting_factor &= lifting_factor - 1;
        }
    }

    base_rows = (uint32_t)((ldpc->parity_bits + lifting_factor - 1) / lifting_factor);
    base_info_cols = (uint32_t)((ldpc->info_bits + lifting_factor - 1) / lifting_factor);
    (void)base_info_cols;

    num_info_edges = ldpc->info_bits * col_weight;
    num_parity_edges = ldpc->parity_bits * 2 - 1;
    total_edges = num_info_edges + num_parity_edges;

    ldpc->parity_matrix.num_edges = (uint32_t)total_edges;

    ldpc->parity_matrix.row_ptr = (uint32_t *)pcalloc(ldpc->parity_matrix.num_checks + 1, sizeof(uint32_t));
    ldpc->parity_matrix.col_idx = (uint32_t *)pmalloc(ldpc->parity_matrix.num_edges * sizeof(uint32_t));

    if (!ldpc->parity_matrix.row_ptr || !ldpc->parity_matrix.col_idx) {
        return false;
    }

    col_counts = (uint32_t *)pcalloc(ldpc->parity_matrix.num_checks, sizeof(uint32_t));
    if (!col_counts) {
        return false;
    }

    seed = (uint32_t)ldpc->config.seed;
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    if (!rng) {
        pfree(col_counts);
        return false;
    }
    for (i = 0; i < ldpc->info_bits; i++) {
        base_col = (uint32_t)(i / lifting_factor);
        pos_in_block = (uint32_t)(i % lifting_factor);
        (void)base_col;

        for (j = 0; j < col_weight; j++) {
            poporon_rng_next(rng, &rval, sizeof(rval));
            block_row = rval % base_rows;
            poporon_rng_next(rng, &rval, sizeof(rval));
            shift = rval % lifting_factor;

            row_in_block = (pos_in_block + shift) % lifting_factor;
            target_row = block_row * lifting_factor + row_in_block;

            if (target_row < ldpc->parity_bits) {
                col_counts[target_row]++;
            }
        }
    }

    for (i = 0; i < ldpc->parity_bits; i++) {
        if (i == 0) {
            col_counts[i] += 1;
        } else {
            col_counts[i] += 2;
        }
    }

    ldpc->parity_matrix.row_ptr[0] = 0;
    for (i = 0; i < ldpc->parity_matrix.num_checks; i++) {
        ldpc->parity_matrix.row_ptr[i + 1] = ldpc->parity_matrix.row_ptr[i] + col_counts[i];
    }

    pmemset(col_counts, 0, ldpc->parity_matrix.num_checks * sizeof(uint32_t));

    poporon_rng_destroy(rng);
    seed = (uint32_t)ldpc->config.seed;
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    if (!rng) {
        pfree(col_counts);
        return false;
    }
    for (i = 0; i < ldpc->info_bits; i++) {
        pos_in_block = (uint32_t)(i % lifting_factor);

        for (j = 0; j < col_weight; j++) {
            poporon_rng_next(rng, &rval, sizeof(rval));
            block_row = rval % base_rows;
            poporon_rng_next(rng, &rval, sizeof(rval));
            shift = rval % lifting_factor;

            row_in_block = (pos_in_block + shift) % lifting_factor;
            target_row = block_row * lifting_factor + row_in_block;

            if (target_row < ldpc->parity_bits) {
                ldpc->parity_matrix.col_idx[ldpc->parity_matrix.row_ptr[target_row] + col_counts[target_row]] =
                    (uint32_t)i;
                col_counts[target_row]++;
            }
        }
    }

    for (i = 0; i < ldpc->parity_bits; i++) {
        parity_col = ldpc->info_bits + i;

        if (i > 0) {
            ldpc->parity_matrix.col_idx[ldpc->parity_matrix.row_ptr[i] + col_counts[i]] =
                (uint32_t)(ldpc->info_bits + i - 1);
            col_counts[i]++;
        }

        ldpc->parity_matrix.col_idx[ldpc->parity_matrix.row_ptr[i] + col_counts[i]] = (uint32_t)parity_col;
        col_counts[i]++;
    }

    poporon_rng_destroy(rng);
    pfree(col_counts);

    ldpc->parity_matrix_cols.col_ptr = (uint32_t *)pcalloc(ldpc->parity_matrix.num_bits + 1, sizeof(uint32_t));
    ldpc->parity_matrix_cols.row_idx = (uint32_t *)pmalloc(ldpc->parity_matrix.num_edges * sizeof(uint32_t));
    ldpc->parity_matrix_cols.edge_idx = (uint32_t *)pmalloc(ldpc->parity_matrix.num_edges * sizeof(uint32_t));

    if (!ldpc->parity_matrix_cols.col_ptr || !ldpc->parity_matrix_cols.row_idx || !ldpc->parity_matrix_cols.edge_idx) {
        return false;
    }

    col_counts = (uint32_t *)pcalloc(ldpc->parity_matrix.num_bits, sizeof(uint32_t));
    if (!col_counts) {
        return false;
    }

    for (i = 0; i < ldpc->parity_matrix.num_checks; i++) {
        for (j = ldpc->parity_matrix.row_ptr[i]; j < ldpc->parity_matrix.row_ptr[i + 1]; j++) {
            col_counts[ldpc->parity_matrix.col_idx[j]]++;
        }
    }

    ldpc->parity_matrix_cols.col_ptr[0] = 0;
    for (i = 0; i < ldpc->parity_matrix.num_bits; i++) {
        ldpc->parity_matrix_cols.col_ptr[i + 1] = ldpc->parity_matrix_cols.col_ptr[i] + col_counts[i];
        col_counts[i] = 0;
    }

    for (i = 0; i < ldpc->parity_matrix.num_checks; i++) {
        for (j = ldpc->parity_matrix.row_ptr[i]; j < ldpc->parity_matrix.row_ptr[i + 1]; j++) {
            col = ldpc->parity_matrix.col_idx[j];
            idx = ldpc->parity_matrix_cols.col_ptr[col] + col_counts[col];
            ldpc->parity_matrix_cols.row_idx[idx] = (uint32_t)i;
            ldpc->parity_matrix_cols.edge_idx[idx] = (uint32_t)j;
            col_counts[col]++;
        }
    }

    pfree(col_counts);

    return true;
}

static inline bool build_parity_check_matrix(poporon_ldpc_t *ldpc)
{
    uint32_t col_weight = ldpc->config.column_weight;

    if (col_weight < MIN_COL_WEIGHT) {
        col_weight = MIN_COL_WEIGHT;
    } else if (col_weight > MAX_COL_WEIGHT) {
        col_weight = MAX_COL_WEIGHT;
    }

    switch (ldpc->config.matrix_type) {
    case PPRN_LDPC_QC_RANDOM:
        return build_parity_check_matrix_qc(ldpc, col_weight);
    case PPRN_LDPC_RANDOM:
    default:
        return build_parity_check_matrix_random(ldpc, col_weight);
    }
}

static inline bool allocate_messages(poporon_ldpc_t *ldpc)
{
    ldpc->msg.check_to_var = (int16_t *)pcalloc(ldpc->parity_matrix.num_edges, sizeof(int16_t));
    ldpc->msg.var_to_check = (int16_t *)pcalloc(ldpc->parity_matrix.num_edges, sizeof(int16_t));
    ldpc->msg.llr_total = (int16_t *)pcalloc(ldpc->parity_matrix.num_bits, sizeof(int16_t));
    ldpc->temp_codeword = (uint8_t *)pmalloc(ldpc->codeword_bytes);

    if (!ldpc->msg.check_to_var || !ldpc->msg.var_to_check || !ldpc->msg.llr_total || !ldpc->temp_codeword) {
        return false;
    }

    if (ldpc->config.use_inner_interleave) {
        ldpc->temp_interleaved = (uint8_t *)pmalloc(ldpc->codeword_bytes);
        if (!ldpc->temp_interleaved) {
            return false;
        }
    } else {
        ldpc->temp_interleaved = NULL;
    }

    if (ldpc->config.use_outer_interleave) {
        ldpc->temp_outer = (uint8_t *)pmalloc(ldpc->info_bytes);
        if (!ldpc->temp_outer) {
            return false;
        }
    } else {
        ldpc->temp_outer = NULL;
    }

    return true;
}

static inline bool check_syndrome(const poporon_ldpc_t *ldpc, const uint8_t *codeword)
{
    uint8_t syndrome_bit;
    size_t i, j;

    for (i = 0; i < ldpc->parity_matrix.num_checks; i++) {
        syndrome_bit = 0;

        for (j = ldpc->parity_matrix.row_ptr[i]; j < ldpc->parity_matrix.row_ptr[i + 1]; j++) {
            syndrome_bit ^= get_bit(codeword, ldpc->parity_matrix.col_idx[j]);
        }

        if (syndrome_bit != 0) {
            return false;
        }
    }

    return true;
}

static inline void initialize_messages_soft(poporon_ldpc_t *ldpc, const int8_t *llr)
{
    int16_t llr_val;
    size_t i, j;

    for (i = 0; i < ldpc->codeword_bits; i++) {
        llr_val = (int16_t)llr[i] * LLR_SCALE_FACTOR;
        ldpc->msg.llr_total[i] = ldpc_saturate(llr_val);
    }

    for (i = 0; i < ldpc->parity_matrix.num_bits; i++) {
        for (j = ldpc->parity_matrix_cols.col_ptr[i]; j < ldpc->parity_matrix_cols.col_ptr[i + 1]; j++) {
            ldpc->msg.var_to_check[ldpc->parity_matrix_cols.edge_idx[j]] = ldpc->msg.llr_total[i];
        }
    }

    pmemset(ldpc->msg.check_to_var, 0, ldpc->parity_matrix.num_edges * sizeof(int16_t));
}

static inline void initialize_messages_hard(poporon_ldpc_t *ldpc, const uint8_t *codeword)
{
    int16_t llr_val;
    size_t i, j;

    for (i = 0; i < ldpc->codeword_bits; i++) {
        llr_val = get_bit(codeword, i) ? -LLR_INFINITY : LLR_INFINITY;
        ldpc->msg.llr_total[i] = llr_val;
    }

    for (i = 0; i < ldpc->parity_matrix.num_bits; i++) {
        for (j = ldpc->parity_matrix_cols.col_ptr[i]; j < ldpc->parity_matrix_cols.col_ptr[i + 1]; j++) {
            ldpc->msg.var_to_check[ldpc->parity_matrix_cols.edge_idx[j]] = ldpc->msg.llr_total[i];
        }
    }

    pmemset(ldpc->msg.check_to_var, 0, ldpc->parity_matrix.num_edges * sizeof(int16_t));
}

static inline void check_node_update(poporon_ldpc_t *ldpc)
{
    uint32_t min1_idx;
    int16_t min1, min2, sign, msg, abs_msg;
    size_t i, j, k;

    for (i = 0; i < ldpc->parity_matrix.num_checks; i++) {
        sign = 1;
        min1 = LLR_MAX;
        min2 = LLR_MAX;
        min1_idx = 0;

        for (j = ldpc->parity_matrix.row_ptr[i]; j < ldpc->parity_matrix.row_ptr[i + 1]; j++) {
            msg = ldpc->msg.var_to_check[j];
            if (msg < 0) {
                sign = -sign;
                abs_msg = -msg;
            } else {
                abs_msg = msg;
            }

            if (abs_msg < min1) {
                min2 = min1;
                min1 = abs_msg;
                min1_idx = (uint32_t)j;
            } else if (abs_msg < min2) {
                min2 = abs_msg;
            }
        }

        for (j = ldpc->parity_matrix.row_ptr[i]; j < ldpc->parity_matrix.row_ptr[i + 1]; j++) {
            msg = ldpc->msg.var_to_check[j];

            if (j == min1_idx) {
                abs_msg = min2;
            } else {
                abs_msg = min1;
            }

            abs_msg = (int16_t)((int32_t)abs_msg * MINSUM_ALPHA_NUMERATOR / MINSUM_ALPHA_DENOMINATOR);

            k = (msg < 0) ? -sign : sign;
            ldpc->msg.check_to_var[j] = (int16_t)(k * abs_msg);
        }
    }
}

static inline void variable_node_update(poporon_ldpc_t *ldpc, const int8_t *channel_llr)
{
    int32_t sum;
    int16_t channel;
    size_t i, j;

    for (i = 0; i < ldpc->parity_matrix.num_bits; i++) {
        if (channel_llr) {
            channel = (int16_t)channel_llr[i] * LLR_SCALE_FACTOR;
        } else {
            channel = ldpc->msg.llr_total[i];
        }

        sum = channel;

        for (j = ldpc->parity_matrix_cols.col_ptr[i]; j < ldpc->parity_matrix_cols.col_ptr[i + 1]; j++) {
            sum += ldpc->msg.check_to_var[ldpc->parity_matrix_cols.edge_idx[j]];
        }

        ldpc->msg.llr_total[i] = ldpc_saturate(sum);

        for (j = ldpc->parity_matrix_cols.col_ptr[i]; j < ldpc->parity_matrix_cols.col_ptr[i + 1]; j++) {
            ldpc->msg.var_to_check[ldpc->parity_matrix_cols.edge_idx[j]] =
                ldpc_saturate(sum - ldpc->msg.check_to_var[ldpc->parity_matrix_cols.edge_idx[j]]);
        }
    }
}

static inline void make_hard_decision(poporon_ldpc_t *ldpc, uint8_t *codeword)
{
    size_t i;

    pmemset(codeword, 0, ldpc->codeword_bytes);
    for (i = 0; i < ldpc->codeword_bits; i++) {
        if (ldpc->msg.llr_total[i] < 0) {
            set_bit(codeword, i, 1);
        }
    }
}

extern bool poporon_ldpc_params_default(poporon_ldpc_params_t *config)
{
    if (!config) {
        return false;
    }

    config->matrix_type = PPRN_LDPC_RANDOM;
    config->column_weight = DEFAULT_COL_WEIGHT;
    config->use_inner_interleave = false;
    config->use_outer_interleave = false;
    config->interleave_depth = 0;
    config->lifting_factor = 0;
    config->seed = 0;

    return true;
}

extern bool poporon_ldpc_params_burst_resistant(poporon_ldpc_params_t *config)
{
    if (!config) {
        return false;
    }

    config->matrix_type = PPRN_LDPC_RANDOM;
    config->column_weight = 7;
    config->use_inner_interleave = true;
    config->use_outer_interleave = true;
    config->interleave_depth = 0;
    config->lifting_factor = 0;
    config->seed = 0;

    return true;
}

extern poporon_ldpc_t *poporon_ldpc_create(size_t block_size, poporon_ldpc_rate_t rate,
                                           const poporon_ldpc_params_t *config)
{
    poporon_ldpc_t *ldpc;
    poporon_ldpc_params_t default_config;
    uint32_t info_num, parity_num;

    if (block_size < MIN_BLOCK_SIZE || block_size > MAX_BLOCK_SIZE || (block_size % 4) != 0) {
        return NULL;
    }

    if (rate > PPRN_LDPC_RATE_5_6) {
        return NULL;
    }

    get_rate_params(rate, &info_num, &parity_num);

    ldpc = (poporon_ldpc_t *)pcalloc(1, sizeof(poporon_ldpc_t));
    if (!ldpc) {
        return NULL;
    }

    if (config) {
        ldpc->config = *config;
    } else {
        poporon_ldpc_params_default(&default_config);
        ldpc->config = default_config;
    }

    ldpc->rate = rate;
    ldpc->info_bits = block_size * 8;
    ldpc->parity_bits = (ldpc->info_bits * parity_num) / info_num;
    ldpc->codeword_bits = ldpc->info_bits + ldpc->parity_bits;
    ldpc->info_bytes = block_size;
    ldpc->parity_bytes = (ldpc->parity_bits + 7) / 8;
    ldpc->codeword_bytes = ldpc->info_bytes + ldpc->parity_bytes;

    if (!build_parity_check_matrix(ldpc)) {
        poporon_ldpc_destroy(ldpc);
        return NULL;
    }

    if (!build_interleaver(ldpc)) {
        poporon_ldpc_destroy(ldpc);
        return NULL;
    }

    if (!build_outer_interleaver(ldpc)) {
        poporon_ldpc_destroy(ldpc);
        return NULL;
    }

    if (!allocate_messages(ldpc)) {
        poporon_ldpc_destroy(ldpc);
        return NULL;
    }

    return ldpc;
}

extern void poporon_ldpc_destroy(poporon_ldpc_t *ldpc)
{
    if (!ldpc) {
        return;
    }

    pfree(ldpc->parity_matrix.row_ptr);
    pfree(ldpc->parity_matrix.col_idx);
    pfree(ldpc->parity_matrix_cols.col_ptr);
    pfree(ldpc->parity_matrix_cols.row_idx);
    pfree(ldpc->parity_matrix_cols.edge_idx);
    pfree(ldpc->interleaver.forward);
    pfree(ldpc->interleaver.inverse);
    pfree(ldpc->outer_interleaver.forward);
    pfree(ldpc->outer_interleaver.inverse);
    pfree(ldpc->msg.check_to_var);
    pfree(ldpc->msg.var_to_check);
    pfree(ldpc->msg.llr_total);
    pfree(ldpc->temp_codeword);
    pfree(ldpc->temp_interleaved);
    pfree(ldpc->temp_outer);
    pfree(ldpc);
}

extern size_t poporon_ldpc_info_size(const poporon_ldpc_t *ldpc)
{
    if (!ldpc) {
        return 0;
    }

    return ldpc->info_bytes;
}

extern size_t poporon_ldpc_codeword_size(const poporon_ldpc_t *ldpc)
{
    if (!ldpc) {
        return 0;
    }

    return ldpc->codeword_bytes;
}

extern size_t poporon_ldpc_parity_size(const poporon_ldpc_t *ldpc)
{
    if (!ldpc) {
        return 0;
    }

    return ldpc->parity_bytes;
}

extern bool poporon_ldpc_encode(poporon_ldpc_t *ldpc, const uint8_t *info, uint8_t *parity)
{
    uint32_t col;
    uint8_t xor_val, prev_parity, *codeword;
    size_t i, j;

    if (!ldpc || !info || !parity) {
        return false;
    }

    codeword = ldpc->temp_codeword;
    pmemset(codeword, 0, ldpc->codeword_bytes);
    pmemcpy(codeword, info, ldpc->info_bytes);

    prev_parity = 0;

    for (i = 0; i < ldpc->parity_bits; i++) {
        xor_val = 0;

        for (j = ldpc->parity_matrix.row_ptr[i]; j < ldpc->parity_matrix.row_ptr[i + 1]; j++) {
            col = ldpc->parity_matrix.col_idx[j];
            if (col < ldpc->info_bits) {
                xor_val ^= get_bit(codeword, col);
            }
        }

        xor_val ^= prev_parity;

        set_bit(codeword, ldpc->info_bits + i, xor_val);
        prev_parity = xor_val;
    }

    pmemcpy(parity, codeword + ldpc->info_bytes, ldpc->parity_bytes);

    return true;
}

extern bool poporon_ldpc_check(const poporon_ldpc_t *ldpc, const uint8_t *codeword)
{
    if (!ldpc || !codeword) {
        return false;
    }

    return check_syndrome(ldpc, codeword);
}

extern bool poporon_ldpc_decode_hard(poporon_ldpc_t *ldpc, uint8_t *codeword, uint32_t max_iterations,
                                     uint32_t *iterations_used)
{
    uint32_t iter;
    uint8_t *working_codeword;

    if (!ldpc || !codeword) {
        return false;
    }

    if (max_iterations == 0) {
        max_iterations = DEFAULT_MAX_ITERATIONS;
    }

    working_codeword = ldpc->temp_codeword;
    if (ldpc->config.use_inner_interleave && ldpc->interleaver.inverse) {
        deinterleave_bits(ldpc, codeword, working_codeword);
    } else {
        pmemcpy(working_codeword, codeword, ldpc->codeword_bytes);
    }

    if (check_syndrome(ldpc, working_codeword)) {
        pmemcpy(codeword, working_codeword, ldpc->codeword_bytes);
        if (iterations_used) {
            *iterations_used = 0;
        }
        return true;
    }

    initialize_messages_hard(ldpc, working_codeword);

    for (iter = 0; iter < max_iterations; iter++) {
        check_node_update(ldpc);
        variable_node_update(ldpc, NULL);

        make_hard_decision(ldpc, working_codeword);

        if (check_syndrome(ldpc, working_codeword)) {
            pmemcpy(codeword, working_codeword, ldpc->codeword_bytes);

            if (iterations_used) {
                *iterations_used = iter + 1;
            }
            return true;
        }
    }

    pmemcpy(codeword, working_codeword, ldpc->codeword_bytes);

    if (iterations_used) {
        *iterations_used = max_iterations;
    }

    return false;
}

extern bool poporon_ldpc_decode_soft(poporon_ldpc_t *ldpc, const int8_t *llr, uint8_t *codeword,
                                     uint32_t max_iterations, uint32_t *iterations_used)
{
    uint32_t iter;
    uint8_t *working_codeword;
    int8_t *working_llr;

    if (!ldpc || !llr || !codeword) {
        return false;
    }

    if (max_iterations == 0) {
        max_iterations = DEFAULT_MAX_ITERATIONS;
    }

    working_llr = NULL;
    if (ldpc->config.use_inner_interleave && ldpc->interleaver.inverse) {
        working_llr = (int8_t *)pmalloc(ldpc->codeword_bits);
        if (!working_llr) {
            return false;
        }
        deinterleave_llr(ldpc, llr, working_llr);
        initialize_messages_soft(ldpc, working_llr);
    } else {
        initialize_messages_soft(ldpc, llr);
    }

    working_codeword = ldpc->temp_codeword;

    for (iter = 0; iter < max_iterations; iter++) {
        check_node_update(ldpc);
        variable_node_update(ldpc, working_llr ? working_llr : llr);

        make_hard_decision(ldpc, working_codeword);

        if (check_syndrome(ldpc, working_codeword)) {
            pmemcpy(codeword, working_codeword, ldpc->codeword_bytes);

            if (working_llr) {
                pfree(working_llr);
            }
            if (iterations_used) {
                *iterations_used = iter + 1;
            }
            return true;
        }
    }

    pmemcpy(codeword, working_codeword, ldpc->codeword_bytes);

    if (working_llr) {
        pfree(working_llr);
    }
    if (iterations_used) {
        *iterations_used = max_iterations;
    }

    return false;
}

extern bool poporon_ldpc_has_interleaver(const poporon_ldpc_t *ldpc)
{
    if (!ldpc) {
        return false;
    }
    return ldpc->config.use_inner_interleave && ldpc->interleaver.forward != NULL;
}

extern bool poporon_ldpc_interleave(const poporon_ldpc_t *ldpc, const uint8_t *input, uint8_t *output)
{
    if (!ldpc || !input || !output) {
        return false;
    }

    if (!ldpc->interleaver.forward) {
        pmemcpy(output, input, ldpc->codeword_bytes);
        return true;
    }

    interleave_bits(ldpc, input, output);

    return true;
}

extern bool poporon_ldpc_deinterleave(const poporon_ldpc_t *ldpc, const uint8_t *input, uint8_t *output)
{
    if (!ldpc || !input || !output) {
        return false;
    }

    if (!ldpc->interleaver.inverse) {
        pmemcpy(output, input, ldpc->codeword_bytes);
        return true;
    }

    deinterleave_bits(ldpc, input, output);

    return true;
}
