/*
 * libpoporon - rng.h
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_RNG_H
#define POPORON_RNG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define POPORON_RNG_TYPE_XOSHIRO128PP 0

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XOSHIRO128PP = POPORON_RNG_TYPE_XOSHIRO128PP
} poporon_rng_type_t;

typedef struct _poporon_rng_t poporon_rng_t;

poporon_rng_t *poporon_rng_create(poporon_rng_type_t type, void *seed, size_t seed_size);
void poporon_rng_destroy(poporon_rng_t *rng);

bool poporon_rng_next(poporon_rng_t *rng, void *dest, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* POPORON_RNG_H */
