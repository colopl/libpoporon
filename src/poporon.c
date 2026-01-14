/*
 * libpoporon - poporon.c
 * 
 * This file is part of libpoporon.
 * 
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <poporon.h>

#include "internal/common.h"

static inline void decoder_buffer_destroy(decoder_buffer_t *buffer)
{
    pfree(buffer->error_locator);
    pfree(buffer);
}

static inline decoder_buffer_t *decoder_buffer_create(uint16_t num_roots)
{
    decoder_buffer_t *buffer;
    uint16_t *raw_buffer;
    uint32_t i;

    buffer = (decoder_buffer_t *)pmalloc(sizeof(decoder_buffer_t));
    if (!buffer) {
        return NULL;
    }

    raw_buffer = (uint16_t *)pmalloc(8 * (num_roots + 1) * sizeof(uint16_t));
    if (!raw_buffer) {
        pfree(buffer);

        return NULL;
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

extern void poporon_destroy(poporon_t *pprn)
{
    if (!pprn) {
        return;
    }

    if (pprn->buffer) {
        decoder_buffer_destroy(pprn->buffer);
    }

    poporon_rs_destroy(pprn->rs);

    pfree(pprn);
}

extern poporon_t *poporon_create(uint8_t symbol_size, uint16_t generator_polynomial, uint16_t first_consecutive_root,
                                 uint16_t primitive_element, uint8_t num_roots)
{
    poporon_t *pprn;
    poporon_rs_t *rs;
    decoder_buffer_t *buffer;
    uint32_t iterations;
    uint16_t primitive_inverse;

    rs = poporon_rs_create(symbol_size, generator_polynomial, first_consecutive_root, primitive_element, num_roots);
    if (!rs) {
        return NULL;
    }

    buffer = decoder_buffer_create(num_roots);
    if (!buffer) {
        poporon_rs_destroy(rs);

        return NULL;
    }

    if (primitive_element == 0) {
        decoder_buffer_destroy(buffer);
        poporon_rs_destroy(rs);

        return NULL;
    }

    iterations = 0;
    for (primitive_inverse = 1; (primitive_inverse % primitive_element) != 0; primitive_inverse += rs->gf->field_size) {
        if (++iterations > rs->gf->field_size * 2) {
            decoder_buffer_destroy(buffer);
            poporon_rs_destroy(rs);

            return NULL;
        }
    }
    buffer->primitive_inverse = primitive_inverse / primitive_element;

    pprn = (poporon_t *)pmalloc(sizeof(poporon_t));
    if (!pprn) {
        decoder_buffer_destroy(buffer);
        poporon_rs_destroy(rs);

        return NULL;
    }

    pprn->rs = rs;
    pprn->buffer = buffer;

    return pprn;
}

extern uint32_t poporon_version_id()
{
    return (uint32_t)POPORON_VERSION_ID;
}

extern poporon_buildtime_t poporon_buildtime()
{
    return (poporon_buildtime_t)POPORON_BUILDTIME;
}
