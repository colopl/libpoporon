/*
 * libpoporon - poporon.c
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "poporon.h"

#include "common.h"

extern void poporon_destroy(poporon_t *pprn)
{
    if (!pprn) {
        return;
    }

    decoder_buffer_destroy(pprn->buffer);
    poporon_rs_destroy(pprn->rs);

    free(pprn);
}

extern poporon_t *poporon_create(uint8_t symbol_size, uint16_t generator_polynomial, uint16_t first_consective_root, uint16_t primitive_element, uint8_t num_roots)
{
    poporon_t *pprn;
    poporon_rs_t *rs;
    decoder_buffer_t *buffer;
    uint16_t primitive_inverse;
    
    rs = poporon_rs_create(symbol_size, generator_polynomial, first_consective_root, primitive_element, num_roots);
    if (!rs) {
        return NULL;
    }
 
    buffer = decoder_buffer_create(num_roots);
    if (!buffer) {
        poporon_rs_destroy(rs);
        return NULL;
    }
    
    for (primitive_inverse = 1; (primitive_inverse % primitive_element) != 0; primitive_inverse += rs->gf->field_size);
    buffer->primitive_inverse = primitive_inverse / primitive_element;
    
    pprn = (poporon_t *)malloc(sizeof(poporon_t));
    if (!pprn) {
        decoder_buffer_destroy(buffer);
        poporon_rs_destroy(rs);
        return NULL;
    }
    
    pprn->rs = rs;
    pprn->buffer = buffer;
    
    return pprn;
}
