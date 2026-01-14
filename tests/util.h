/*
 * libpoporon - util.h
 * 
 * This file is part of libpoporon.
 * 
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef POPORON_TEST_TEST_UTIL_H
#define POPORON_TEST_TEST_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline void random_data(uint8_t *out, size_t size)
{
    srand((unsigned int)time(NULL));

    for (size_t i = 0; i < size; i++) {
        out[i] = (uint8_t)(rand() % 256);
    }
}

static inline bool break_data(uint8_t *data, size_t data_size, uint32_t count)
{
    uint32_t corrupted_count = 0;
    size_t pos;

    if (!data || data_size == 0 || count == 0) {
        return false;
    }

    if (count > data_size) {
        count = data_size;
    }

    bool *corrupted = (bool *)calloc(data_size, sizeof(bool));
    if (!corrupted) {
        return false;
    }

    while (corrupted_count < count) {
        pos = rand() % data_size;

        if (!corrupted[pos]) {
            data[pos] ^= 0xFF;
            corrupted[pos] = true;
            corrupted_count++;
        }
    }

    free(corrupted);

    return true;
}

static inline bool break_data_erasure(uint8_t *data, size_t data_size, uint32_t count, poporon_erasure_t *erasure)
{
    uint32_t corrupted_count = 0;
    size_t pos;

    if (!data || data_size == 0 || count == 0) {
        return false;
    }

    if (count > data_size) {
        count = data_size;
    }

    bool *corrupted = (bool *)calloc(data_size, sizeof(bool));
    if (!corrupted) {
        return false;
    }

    while (corrupted_count < count) {
        pos = rand() % data_size;

        if (!corrupted[pos]) {
            data[pos] ^= 0xFF;
            corrupted[pos] = true;
            poporon_erasure_add_position(erasure, pos);
            corrupted_count++;
        }
    }

    free(corrupted);

    return true;
}

static inline void print_hex(uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }

    printf("\n");
}

#endif /* POPORON_TEST_TEST_UTIL_H */
