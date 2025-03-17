/*
 * libpoporon test - test_util.h
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POPORON_TEST_TEST_UTIL_H
#define POPORON_TEST_TEST_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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

static inline void print_hex(uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }

    printf("\n");
}

#endif  /* POPORON_TEST_TEST_UTIL_H */
