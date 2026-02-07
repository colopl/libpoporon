/*
 * libpoporon - config.h
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_INTERNAL_CONFIG_H
#define POPORON_INTERNAL_CONFIG_H

#include <poporon.h>

struct _poporon_config_t {
    poporon_fec_type_t fec_type;
    union {
        struct {
            uint8_t symbol_size;
            uint16_t generator_polynomial;
            uint16_t first_consecutive_root;
            uint16_t primitive_element;
            uint8_t num_roots;
            poporon_erasure_t *erasure;
            uint16_t *syndrome;
        } rs;
        struct {
            size_t block_size;
            poporon_ldpc_rate_t rate;
            poporon_ldpc_matrix_type_t matrix_type;
            uint32_t column_weight;
            bool use_soft_decode;
            bool use_outer_interleave;
            bool use_inner_interleave;
            uint32_t interleave_depth;
            uint32_t lifting_factor;
            uint32_t max_iterations;
            const int8_t *soft_llr;
            size_t soft_llr_size;
            uint64_t seed;
        } ldpc;
        struct {
            uint8_t symbol_size;
            uint16_t generator_polynomial;
            uint8_t correction_capability;
        } bch;
    } params;
};

#endif /* POPORON_INTERNAL_CONFIG_H */
