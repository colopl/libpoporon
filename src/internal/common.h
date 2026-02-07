/*
 * libpoporon - common.h
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_INTERNAL_COMMON_H
#define POPORON_INTERNAL_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <poporon.h>
#include <poporon/gf.h>

#define pmemcpy(dest, src, size)   memcpy(dest, src, size)
#define pmemmove(dest, src, size)  memmove(dest, src, size)
#define pmemcmp(s1, s2, size)      memcmp(s1, s2, size)
#define pmemset(dest, value, size) memset(dest, value, size)
#define pmalloc(size)              malloc(size)
#define pcalloc(count, size)       calloc(count, size)
#define prealloc(ptr, size)        realloc(ptr, size)
#define pfree(ptr)                 free(ptr)

#define POPORON_VERSION_ID 20000000

#ifndef POPORON_BUILDTIME
#define POPORON_BUILDTIME 0
#endif

typedef struct _poporon_rs_t poporon_rs_t;
typedef struct _poporon_bch_t poporon_bch_t;
typedef struct _poporon_ldpc_t poporon_ldpc_t;

struct _poporon_erasure_t {
    uint32_t capacity;
    uint32_t erasure_count;
    uint32_t *erasure_positions;
    uint16_t *corrections;
};

struct _poporon_gf_t {
    uint8_t symbol_size;
    uint8_t field_size;
    uint16_t *log2exp;
    uint16_t *exp2log;
    uint16_t generator_polynomial;
};

struct _poporon_rs_t {
    poporon_gf_t *gf;
    uint16_t first_consecutive_root;
    uint16_t primitive_element;
    uint16_t num_roots;
    uint16_t *generator_polynomial;
};

typedef struct {
    uint16_t *error_locator;
    uint16_t *syndrome;
    uint16_t *coefficients;
    uint16_t *polynomial;
    uint16_t *error_evaluator;
    uint16_t *error_roots;
    uint16_t *register_coefficients;
    uint16_t *error_locations;
    uint16_t primitive_inverse;
} decoder_buffer_t;

struct _poporon_t {
    poporon_fec_type_t fec_type;

    union {
        struct {
            poporon_rs_t *rs;
            decoder_buffer_t *buffer;
            poporon_erasure_t *erasure;
            uint16_t *ext_syndrome;
            size_t last_corrected;
        } rs;

        struct {
            poporon_ldpc_t *ldpc;
            const int8_t *soft_llr;
            size_t soft_llr_size;
            uint32_t max_iterations;
            uint32_t last_iterations;
            bool use_soft_decode;
        } ldpc;

        struct {
            poporon_bch_t *bch;
            int32_t last_num_errors;
        } bch;
    } ctx;
};

static inline uint8_t gf_mod(poporon_gf_t *gf, uint16_t value)
{
    while (value >= gf->field_size) {
        value -= gf->field_size;
        value = (value >> gf->symbol_size) + (value & gf->field_size);
    }

    return value;
}

poporon_rs_t *poporon_rs_create(uint8_t symbol_size, uint16_t generator_polynomial, uint16_t first_consecutive_root,
                                uint16_t primitive_element, uint8_t num_roots);
void poporon_rs_destroy(poporon_rs_t *rs);

poporon_bch_t *poporon_bch_create(uint8_t symbol_size, uint16_t generator_polynomial, uint8_t t);
void poporon_bch_destroy(poporon_bch_t *bch);

uint16_t poporon_bch_get_codeword_length(const poporon_bch_t *bch);
uint16_t poporon_bch_get_data_length(const poporon_bch_t *bch);
uint8_t poporon_bch_get_correction_capability(const poporon_bch_t *bch);
bool poporon_bch_encode(poporon_bch_t *bch, uint32_t data, uint32_t *codeword);
bool poporon_bch_decode(poporon_bch_t *bch, uint32_t received, uint32_t *corrected, int32_t *num_errors);
uint32_t poporon_bch_extract_data(const poporon_bch_t *bch, uint32_t codeword);

#endif /* POPORON_INTERNAL_COMMON_H */
