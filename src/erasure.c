/*
 * libpoporon - erasure.c
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "common.h"

extern void poporon_erasure_destroy(poporon_erasure_t *erasure)
{
    if (erasure) {
        if (erasure->erasure_positions) {
            free(erasure->erasure_positions);
        }
        if (erasure->corrections) {
            free(erasure->corrections);
        }

        free(erasure);
    }
}

extern poporon_erasure_t *poporon_erasure_create(uint16_t num_roots, uint32_t initial_capacity)
{
    poporon_erasure_t *erasure = NULL;
    uint32_t capacity;

    capacity = (initial_capacity > 0) ? initial_capacity : (uint32_t)num_roots;

    erasure = (poporon_erasure_t *)malloc(sizeof(poporon_erasure_t));
    if (!erasure) {
        return NULL;
    }
    erasure->erasure_positions = NULL;
    erasure->corrections = NULL;

    erasure->erasure_positions = (uint32_t *)malloc(capacity * sizeof(uint32_t));
    if (!erasure->erasure_positions) {
        poporon_erasure_destroy(erasure);
        return NULL;
    }

    erasure->corrections = (uint16_t *)malloc(capacity * sizeof(uint16_t));
    if (!erasure->corrections) {
        poporon_erasure_destroy(erasure);
        return NULL;
    }

    erasure->capacity = capacity;
    erasure->erasure_count = 0;

    return erasure;
}

extern poporon_erasure_t *poporon_erasure_create_from_positions(uint16_t num_roots, const uint32_t *erasure_positions, uint32_t erasure_count)
{
    poporon_erasure_t *erasure = NULL;
    uint32_t capacity;

    if (!erasure_positions || erasure_count == 0) {
        return NULL;
    }

    capacity = (erasure_count > num_roots) ? erasure_count : num_roots;

    erasure = poporon_erasure_create(num_roots, capacity);
    if (!erasure) {
        return NULL;
    }

    memcpy(erasure->erasure_positions, erasure_positions, erasure_count * sizeof(uint32_t));
    erasure->erasure_count = erasure_count;

    return erasure;
}

extern bool poporon_erasure_add_position(poporon_erasure_t *erasure, uint32_t position)
{
    uint32_t new_capacity, *new_positions;
    uint16_t *new_corrections;

    if (!erasure) {
        return false;
    }

    if (erasure->erasure_count >= erasure->capacity) {
        new_capacity = erasure->capacity * 2;
        if (new_capacity < erasure->capacity + 32) {
            new_capacity = erasure->capacity + 32;
        }
        
        new_positions = (uint32_t *)realloc(erasure->erasure_positions, new_capacity * sizeof(uint32_t));
        new_corrections = (uint16_t *)realloc(erasure->corrections, new_capacity * sizeof(uint16_t));

        if (!new_positions || !new_corrections) {
            if (new_positions) {
                free(new_positions);
            }
            if (new_corrections) {
                free(new_corrections);
            }

            return false;
        }

        erasure->erasure_positions = new_positions;
        erasure->corrections = new_corrections;
        erasure->capacity = new_capacity;
    }

    erasure->erasure_positions[erasure->erasure_count++] = position;

    return true;
}

extern void poporon_erasure_reset(poporon_erasure_t *erasure)
{
    if (erasure) {
        erasure->erasure_count = 0;
    }
}
