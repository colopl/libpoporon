/*
 * libpoporon - rs.c
 * 
 * This file is part of libpoporon.
 * 
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "internal/common.h"

void poporon_rs_destroy(poporon_rs_t *rs)
{
    if (!rs) {
        return;
    }

    if (rs->gf) {
        poporon_gf_destroy(rs->gf);
    }

    if (rs->generator_polynomial) {
        pfree(rs->generator_polynomial);
    }

    pfree(rs);
}

poporon_rs_t *poporon_rs_create(uint8_t symbol_size, uint16_t generator_polynomial, uint16_t first_consecutive_root,
                                uint16_t primitive_element, uint8_t num_roots)
{
    poporon_rs_t *rs;
    poporon_gf_t *gf;
    uint16_t i, j, generator_root;

    gf = poporon_gf_create(symbol_size, generator_polynomial);
    if (!gf) {
        return NULL;
    }

    rs = (poporon_rs_t *)pmalloc(sizeof(poporon_rs_t));
    if (!rs) {
        poporon_gf_destroy(gf);

        return NULL;
    }

    rs->gf = gf;
    rs->first_consecutive_root = first_consecutive_root;
    rs->primitive_element = primitive_element;
    rs->num_roots = num_roots;
    rs->generator_polynomial = (uint16_t *)pmalloc((num_roots + 1) * sizeof(uint16_t));
    if (!rs->generator_polynomial) {
        poporon_rs_destroy(rs);

        return NULL;
    }

    rs->generator_polynomial[0] = 1;
    for (i = 0, generator_root = first_consecutive_root * primitive_element; i < num_roots;
         i++, generator_root += primitive_element) {
        rs->generator_polynomial[i + 1] = 1;

        for (j = i; j > 0; j--) {
            if (rs->generator_polynomial[j] != 0) {
                rs->generator_polynomial[j] =
                    rs->generator_polynomial[j - 1] ^
                    gf->log2exp[gf_mod(gf, gf->exp2log[rs->generator_polynomial[j]] + generator_root)];
            } else {
                rs->generator_polynomial[j] = rs->generator_polynomial[j - 1];
            }
        }

        rs->generator_polynomial[0] =
            gf->log2exp[gf_mod(gf, gf->exp2log[rs->generator_polynomial[0]] + generator_root)];
    }

    for (i = 0; i <= num_roots; i++) {
        rs->generator_polynomial[i] = gf->exp2log[rs->generator_polynomial[i]];
    }

    return rs;
}
