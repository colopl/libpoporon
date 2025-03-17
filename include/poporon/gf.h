/*
 * libpoporon - poporon/gf.h
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POPORON_GF_H
#define POPORON_GF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _poporon_gf_t poporon_gf_t;

poporon_gf_t *poporon_gf_create(uint8_t symbol_size, uint16_t generator_polynomial);
void poporon_gf_destroy(poporon_gf_t *gf);

uint8_t poporon_gf_mod(poporon_gf_t *gf, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif  /* POPORON_GF_H */
