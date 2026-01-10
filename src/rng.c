/*
 * libpoporon - rng.c
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include <poporon/rng.h>

#include "internal/common.h"

#define SPLITMIX32_CONST_0 0x6C078965U
#define SPLITMIX32_CONST_1 0x9D2C5680U
#define SPLITMIX32_CONST_2 0xEFC60000U
#define SPLITMIX32_CONST_3 0x12345678U

struct _poporon_rng_t {
    poporon_rng_type_t type;
    uint32_t s[4]; /* State for Xoshiro128++ */
};

static inline uint32_t rotl(uint32_t x, int k)
{
    return (x << k) | (x >> (32 - k));
}

static inline uint32_t splitmix32(uint32_t z)
{
    z = (z ^ (z >> 16)) * 0x85EBCA6BU;
    z = (z ^ (z >> 13)) * 0xC2B2AE35U;
    return z ^ (z >> 16);
}

static inline void xoshiro128pp_init(poporon_rng_t *rng, const void *seed, size_t seed_size)
{
    uint32_t s, z;

    s = 0;
    if (seed && seed_size > 0) {
        memcpy(&s, seed, seed_size < sizeof(uint32_t) ? seed_size : sizeof(uint32_t));
    }

    z = s + SPLITMIX32_CONST_0;
    rng->s[0] = splitmix32(z);

    z = rng->s[0] + SPLITMIX32_CONST_1;
    rng->s[1] = splitmix32(z);

    z = rng->s[1] + SPLITMIX32_CONST_2;
    rng->s[2] = splitmix32(z);

    z = rng->s[2] + SPLITMIX32_CONST_3;
    rng->s[3] = splitmix32(z);
}

static inline uint32_t xoshiro128pp_next(poporon_rng_t *rng)
{
    uint32_t result, t;

    result = rotl(rng->s[0] + rng->s[3], 7) + rng->s[0];
    t = rng->s[1] << 9;

    rng->s[2] ^= rng->s[0];
    rng->s[3] ^= rng->s[1];
    rng->s[1] ^= rng->s[2];
    rng->s[0] ^= rng->s[3];
    rng->s[2] ^= t;
    rng->s[3] = rotl(rng->s[3], 11);

    return result;
}

extern poporon_rng_t *poporon_rng_create(poporon_rng_type_t type, void *seed, size_t seed_size)
{
    poporon_rng_t *rng;

    rng = (poporon_rng_t *)pcalloc(1, sizeof(poporon_rng_t));
    if (!rng) {
        return NULL;
    }

    rng->type = type;

    switch (type) {
    case XOSHIRO128PP:
    default:
        xoshiro128pp_init(rng, seed, seed_size);
        break;
    }

    return rng;
}

extern void poporon_rng_destroy(poporon_rng_t *rng)
{
    if (rng) {
        pfree(rng);
    }
}

extern bool poporon_rng_next(poporon_rng_t *rng, void *dest, size_t size)
{
    uint8_t *p;
    uint32_t val;
    size_t i, remaining;

    if (!rng || !dest || size == 0) {
        return false;
    }

    p = (uint8_t *)dest;
    i = 0;

    while (i + 4 <= size) {
        val = xoshiro128pp_next(rng);
        pmemcpy(p + i, &val, 4);
        i += 4;
    }

    remaining = size - i;
    if (remaining > 0) {
        val = xoshiro128pp_next(rng);
        pmemcpy(p + i, &val, remaining);
    }

    return true;
}
