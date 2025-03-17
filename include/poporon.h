/*
 * libpoporon - poporon.h
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POPORON_H
#define POPORON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "poporon/erasure.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _poporon_t poporon_t;

poporon_t *poporon_create(uint8_t symbol_size, uint16_t generator_polynomial, uint16_t first_consective_root, uint16_t primitive_element, uint8_t num_roots);
void poporon_destroy(poporon_t *poporon);

bool poporon_encode_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity);

bool poporon_decode_u8_with_syndrome(poporon_t *pprn, uint8_t *data, uint8_t *parity, size_t size, uint16_t *syndrome, size_t *corrected_num);
bool poporon_decode_u8_with_erasure(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, poporon_erasure_t *eras, size_t *corrected_num);
bool poporon_decode_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity, size_t *corrected_num);

#ifdef __cplusplus
}
#endif

#endif  /* POPORON_H */
