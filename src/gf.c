/*
 * libpoporon - gf.c
 * 
 * This file is part of libpoporon.
 * 
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "internal/common.h"

extern void poporon_gf_destroy(poporon_gf_t *gf)
{
    if (!gf) {
        return;
    }

    if (gf->exp2log) {
        pfree(gf->exp2log);
    }

    if (gf->log2exp) {
        pfree(gf->log2exp);
    }

    pfree(gf);
}

extern poporon_gf_t *poporon_gf_create(uint8_t symbol_size, uint16_t generator_polynomial)
{
    poporon_gf_t *gf;
    uint16_t field_element;
    uint8_t i;

    if (symbol_size < 1 || symbol_size > 16) {
        return NULL;
    }

    gf = (poporon_gf_t *)pmalloc(sizeof(poporon_gf_t));
    if (!gf) {
        return NULL;
    }

    gf->symbol_size = symbol_size;
    gf->field_size = (1 << symbol_size) - 1;
    gf->generator_polynomial = generator_polynomial;

    gf->log2exp = (uint16_t *)pmalloc((gf->field_size + 1) * sizeof(uint16_t));
    if (!gf->log2exp) {
        poporon_gf_destroy(gf);

        return NULL;
    }

    gf->exp2log = (uint16_t *)pmalloc((gf->field_size + 1) * sizeof(uint16_t));
    if (!gf->exp2log) {
        poporon_gf_destroy(gf);

        return NULL;
    }

    gf->exp2log[0] = gf->field_size; /* log(0) = -inf represented as field_size */
    gf->log2exp[gf->field_size] = 0; /* exp(-inf) = 0 */

    field_element = 1;
    for (i = 0; i < gf->field_size; i++) {
        gf->exp2log[field_element] = i;
        gf->log2exp[i] = field_element;

        field_element <<= 1;

        if (field_element & (1 << symbol_size)) {
            field_element ^= generator_polynomial;
        }

        field_element &= gf->field_size;
    }

    if (field_element != gf->log2exp[0]) {
        poporon_gf_destroy(gf);

        return NULL;
    }

    return gf;
}

extern uint8_t poporon_gf_mod(poporon_gf_t *gf, uint16_t value)
{
    return gf_mod(gf, value);
}
