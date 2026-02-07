/*
 * libpoporon - poporon.h
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_H
#define POPORON_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "poporon/erasure.h"
#include "poporon/gf.h"
#include "poporon/rng.h"

#define POPORON_FEC_RS      1
#define POPORON_FEC_LDPC    2
#define POPORON_FEC_BCH     3
#define POPORON_FEC_UNKNOWN 255

#define POPORON_LDPC_RATE_1_3 0
#define POPORON_LDPC_RATE_1_2 1
#define POPORON_LDPC_RATE_2_3 2
#define POPORON_LDPC_RATE_3_4 3
#define POPORON_LDPC_RATE_4_5 4
#define POPORON_LDPC_RATE_5_6 5

#define POPORON_LDPC_MATRIX_RANDOM    1
#define POPORON_LDPC_MATRIX_QC_RANDOM 2

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t poporon_buildtime_t;

typedef struct _poporon_t poporon_t;
typedef struct _poporon_config_t poporon_config_t;

typedef enum {
    PPLN_FEC_RS = POPORON_FEC_RS,
    PPLN_FEC_LDPC = POPORON_FEC_LDPC,
    PPLN_FEC_BCH = POPORON_FEC_BCH,
    PPLN_FEC_UNKNOWN = POPORON_FEC_UNKNOWN,
} poporon_fec_type_t;

typedef enum {
    PPRN_LDPC_RATE_1_3 = POPORON_LDPC_RATE_1_3, /* 200% redundancy */
    PPRN_LDPC_RATE_1_2 = POPORON_LDPC_RATE_1_2, /* 100% redundancy */
    PPRN_LDPC_RATE_2_3 = POPORON_LDPC_RATE_2_3, /* 50% redundancy */
    PPRN_LDPC_RATE_3_4 = POPORON_LDPC_RATE_3_4, /* 33% redundancy */
    PPRN_LDPC_RATE_4_5 = POPORON_LDPC_RATE_4_5, /* 25% redundancy */
    PPRN_LDPC_RATE_5_6 = POPORON_LDPC_RATE_5_6, /* 20% redundancy */
} poporon_ldpc_rate_t;

typedef enum {
    PPRN_LDPC_RANDOM = POPORON_LDPC_MATRIX_RANDOM,
    PPRN_LDPC_QC_RANDOM = POPORON_LDPC_MATRIX_QC_RANDOM,
} poporon_ldpc_matrix_type_t;

poporon_config_t *poporon_rs_config_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                           uint16_t first_consecutive_root, uint16_t primitive_element,
                                           uint8_t num_roots, poporon_erasure_t *erasure, uint16_t *syndrome);

poporon_config_t *poporon_ldpc_config_create(size_t block_size, poporon_ldpc_rate_t rate,
                                             poporon_ldpc_matrix_type_t matrix_type, uint32_t column_weight,
                                             bool use_soft_decode, bool use_outer_interleave, bool use_inner_interleave,
                                             uint32_t interleave_depth, uint32_t lifting_factor,
                                             uint32_t max_iterations, const int8_t *soft_llr, size_t soft_llr_size,
                                             uint64_t seed);

poporon_config_t *poporon_bch_config_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                            uint8_t correction_capability);

poporon_config_t *poporon_config_rs_default(void);
poporon_config_t *poporon_config_ldpc_default(size_t block_size, poporon_ldpc_rate_t rate);
poporon_config_t *poporon_config_ldpc_burst_resistant(size_t block_size, poporon_ldpc_rate_t rate);
poporon_config_t *poporon_config_bch_default(void);
void poporon_config_destroy(poporon_config_t *config);

poporon_t *poporon_create(const poporon_config_t *config);
void poporon_destroy(poporon_t *pprn);

bool poporon_encode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity);
bool poporon_decode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, size_t *corrected_num);

poporon_fec_type_t poporon_get_fec_type(const poporon_t *pprn);
uint32_t poporon_get_iterations_used(const poporon_t *pprn);
size_t poporon_get_parity_size(const poporon_t *pprn);
size_t poporon_get_info_size(const poporon_t *pprn);

uint32_t poporon_version_id(void);
poporon_buildtime_t poporon_buildtime(void);

#ifdef __cplusplus
}
#endif

#endif /* POPORON_H */
