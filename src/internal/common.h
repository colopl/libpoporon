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
#include <poporon/rs.h>

#define POPORON_VERSION_ID 10000000

#ifndef POPORON_BUILDTIME
#define POPORON_BUILDTIME 0
#endif

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
    poporon_rs_t *rs;
    decoder_buffer_t *buffer;
};

#define pmemcpy(dest, src, size)   memcpy(dest, src, size)
#define pmemmove(dest, src, size)  memmove(dest, src, size)
#define pmemcmp(s1, s2, size)      memcmp(s1, s2, size)
#define pmemset(dest, value, size) memset(dest, value, size)
#define pmalloc(size)              malloc(size)
#define pcalloc(count, size)       calloc(count, size)
#define prealloc(ptr, size)        realloc(ptr, size)
#define pfree(ptr)                 free(ptr)

static inline uint8_t gf_mod(poporon_gf_t *gf, uint16_t value)
{
    while (value >= gf->field_size) {
        value -= gf->field_size;
        value = (value >> gf->symbol_size) + (value & gf->field_size);
    }

    return value;
}

#endif /* POPORON_INTERNAL_COMMON_H */
