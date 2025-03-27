/*
 * libpoporon - poporon_internal.h
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POPORON_INTERNAL_H
#define POPORON_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <poporon.h>

#include <poporon/gf.h>
#include <poporon/rs.h>

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
    uint16_t first_consective_root;
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

#define pmemcpy(dest, src, size)            memcpy(dest, src, size)
#define pmemmove(dest, src, size)           memmove(dest, src, size)
#define pmemcmp(s1, s2, size)               memcmp(s1, s2, size)
#define pmemset(dest, value, size)          memset(dest, value, size)
#define pmalloc(size)                       malloc(size)
#define pcalloc(count, size)                calloc(count, size)
#define prealloc(ptr, size)                 realloc(ptr, size)
#define pfree(ptr)                          free(ptr)

static inline decoder_buffer_t *decoder_buffer_create(uint16_t num_roots)
{
    decoder_buffer_t *buffer;
    uint16_t *raw_buffer;
    uint32_t i;
    
    buffer = (decoder_buffer_t *)pmalloc(sizeof(decoder_buffer_t));
    if (!buffer) {
        return NULL; /* LCOV_EXCL_LINE */
    }
    
    raw_buffer = (uint16_t *)pmalloc(8 * (num_roots + 1) * sizeof(uint16_t));
    if (!raw_buffer) {
        /* LCOV_EXCL_START */
        pfree(buffer);
        return NULL;
        /* LCOV_EXCL_STOP */
    }
    
    buffer->primitive_inverse = 0;
    buffer->error_locator = raw_buffer;
    buffer->syndrome = buffer->error_locator + (num_roots + 1);
    buffer->coefficients = buffer->syndrome + (num_roots + 1);
    buffer->polynomial = buffer->coefficients + (num_roots + 1);
    buffer->error_evaluator = buffer->polynomial + (num_roots + 1);
    buffer->error_roots = buffer->error_evaluator + (num_roots + 1);
    buffer->register_coefficients = buffer->error_roots + (num_roots + 1);
    buffer->error_locations = buffer->register_coefficients + (num_roots + 1);
    
    for (i = 0; i < 8 * (num_roots + 1); i++) {
        raw_buffer[i] = 0;
    }
    
    return buffer;
}

static inline void decoder_buffer_destroy(decoder_buffer_t *buffer)
{
    pfree(buffer->error_locator);
    pfree(buffer);
}

static inline uint8_t gf_mod(poporon_gf_t *gf, uint16_t value)
{    
    while (value >= gf->field_size) {
        value -= gf->field_size;
        value = (value >> gf->symbol_size) + (value & gf->field_size);
    }
    
    return value;
}

#endif  /* POPORON_INTERNAL_H */
