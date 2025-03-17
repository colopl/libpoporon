/*
 * libpoporon - encode.c
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "common.h"

extern bool poporon_encode_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity)
{
    uint16_t i, j, fb;

    if (!pprn || !data || !parity) {
        return false;
    }

    memset(parity, 0, pprn->rs->num_roots * sizeof(uint8_t));

    for (i = 0; i < size; i++) {
        fb = pprn->rs->gf->exp2log[(((uint16_t)data[i]) & ((uint16_t)pprn->rs->gf->field_size)) ^ parity[0]];
        
        if (fb != pprn->rs->gf->field_size) {
            for (j = 1; j < pprn->rs->num_roots; j++) {
                parity[j] ^= pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, fb + pprn->rs->generator_polynomial[pprn->rs->num_roots - j])];
            }
        }
        
        memmove(&parity[0], &parity[1], sizeof(uint8_t) * (pprn->rs->num_roots - 1));
        
        if (fb != pprn->rs->gf->field_size) {
            parity[pprn->rs->num_roots - 1] = pprn->rs->gf->log2exp[gf_mod(pprn->rs->gf, fb + pprn->rs->generator_polynomial[0])];
        } else {
            parity[pprn->rs->num_roots - 1] = 0;
        }
    }

    return true;
}
