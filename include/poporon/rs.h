/*
 * libpoporon - rs.h
 * 
 * This file is part of libpoporon.
 * 
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_RS_H
#define POPORON_RS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _poporon_rs_t poporon_rs_t;

poporon_rs_t *poporon_rs_create(uint8_t symbol_size, uint16_t generator_polynomial, uint16_t first_consecutive_root,
                                uint16_t primitive_element, uint8_t num_roots);
void poporon_rs_destroy(poporon_rs_t *rs);

#ifdef __cplusplus
}
#endif

#endif /* POPORON_RS_H */
