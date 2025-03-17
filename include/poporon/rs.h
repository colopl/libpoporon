/*
 * libpoporon - poporon/rs.h
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POPORON_RS_H
#define POPORON_RS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _poporon_rs_t poporon_rs_t;

poporon_rs_t *poporon_rs_create(uint8_t symbol_size, uint16_t generator_polynomial, uint16_t first_consective_root, uint16_t primitive_element, uint8_t num_roots);
void poporon_rs_destroy(poporon_rs_t *rs);

#ifdef __cplusplus
}
#endif

#endif  /* POPORON_RS_H */
