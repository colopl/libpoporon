/*
 * libpoporon - poporon/erasure.h
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POPORON_ERASURE_H
#define POPORON_ERASURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _poporon_erasure_t poporon_erasure_t;

poporon_erasure_t *poporon_erasure_create(uint16_t num_roots, uint32_t initial_capacity);
poporon_erasure_t *poporon_erasure_create_from_positions(uint16_t num_roots, const uint32_t *erasure_positions, uint32_t erasure_count);
bool poporon_erasure_add_position(poporon_erasure_t *erasure, uint32_t position);
void poporon_erasure_reset(poporon_erasure_t *erasure);
void poporon_erasure_destroy(poporon_erasure_t *eras);

#ifdef __cplusplus
}
#endif

#endif  /* POPORON_ERASURE_H */
