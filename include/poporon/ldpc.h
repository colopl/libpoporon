/*
 * libpoporon - ldpc.h
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_LDPC_H
#define POPORON_LDPC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POPORON_LDPC_RATE_1_2 0
#define POPORON_LDPC_RATE_2_3 1
#define POPORON_LDPC_RATE_3_4 2
#define POPORON_LDPC_RATE_4_5 3
#define POPORON_LDPC_RATE_5_6 4

#define POPORON_LDPC_MATRIX_RANDOM 0
#define POPORON_LDPC_MATRIX_QC_PEG 1

typedef struct _poporon_ldpc_t poporon_ldpc_t;

typedef enum {
    PPRN_LDPC_RATE_1_2 = POPORON_LDPC_RATE_1_2, /* 50% redundancy */
    PPRN_LDPC_RATE_2_3 = POPORON_LDPC_RATE_2_3, /* 33% redundancy */
    PPRN_LDPC_RATE_3_4 = POPORON_LDPC_RATE_3_4, /* 25% redundancy */
    PPRN_LDPC_RATE_4_5 = POPORON_LDPC_RATE_4_5, /* 20% redundancy */
    PPRN_LDPC_RATE_5_6 = POPORON_LDPC_RATE_5_6, /* 17% redundancy */
} poporon_ldpc_rate_t;

typedef enum {
    PPRN_LDPC_RANDOM = POPORON_LDPC_MATRIX_RANDOM,
    PPRN_LDPC_QC_PEG = POPORON_LDPC_MATRIX_QC_PEG,
} poporon_ldpc_matrix_type_t;

typedef struct {
    poporon_ldpc_matrix_type_t matrix_type;
    uint32_t column_weight;
    bool use_interleaver;
    uint32_t interleave_depth;
    uint32_t lifting_factor;
} poporon_ldpc_config_t;

poporon_ldpc_t *poporon_ldpc_create(size_t block_size, poporon_ldpc_rate_t rate, const poporon_ldpc_config_t *config);
void poporon_ldpc_destroy(poporon_ldpc_t *ldpc);

bool poporon_ldpc_config_default(poporon_ldpc_config_t *config);
bool poporon_ldpc_config_burst_resistant(poporon_ldpc_config_t *config);

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

#ifdef __cplusplus
}
#endif

#endif /* POPORON_LDPC_H */
