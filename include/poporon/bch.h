/*
 * libpoporon - bch.h
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_BCH_H
#define POPORON_BCH_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _poporon_bch_t poporon_bch_t;

poporon_bch_t *poporon_bch_create(uint8_t symbol_size, uint16_t generator_polynomial, uint8_t t);
void poporon_bch_destroy(poporon_bch_t *bch);

uint16_t poporon_bch_get_codeword_length(const poporon_bch_t *bch);
uint16_t poporon_bch_get_data_length(const poporon_bch_t *bch);
uint8_t poporon_bch_get_correction_capability(const poporon_bch_t *bch);
bool poporon_bch_encode(poporon_bch_t *bch, uint32_t data, uint32_t *codeword);
bool poporon_bch_decode(poporon_bch_t *bch, uint32_t received, uint32_t *corrected, int32_t *num_errors);
uint32_t poporon_bch_extract_data(const poporon_bch_t *bch, uint32_t codeword);

#ifdef __cplusplus
}
#endif

#endif /* POPORON_BCH_H */
