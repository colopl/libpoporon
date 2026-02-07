/*
 * libpoporon - test_erasure.c
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <poporon.h>

#include "unity.h"
#include "util.h"

#define NUMBER_OF_ROOTS  16
#define INITIAL_CAPACITY 8

void setUp(void)
{
}

void tearDown(void)
{
}

void test_erasure_create_destroy(void)
{
    poporon_erasure_t *erasure;

    erasure = poporon_erasure_create(NUMBER_OF_ROOTS, INITIAL_CAPACITY);
    TEST_ASSERT_NOT_NULL(erasure);
    poporon_erasure_destroy(erasure);

    erasure = poporon_erasure_create(NUMBER_OF_ROOTS, 0);
    TEST_ASSERT_NOT_NULL(erasure);
    poporon_erasure_destroy(erasure);

    poporon_erasure_destroy(NULL);
}

void test_erasure_create_from_positions(void)
{
    poporon_erasure_t *erasure;
    uint32_t positions[] = {1, 3, 5, 7, 9};
    uint32_t erasure_count = sizeof(positions) / sizeof(positions[0]);

    erasure = poporon_erasure_create_from_positions(NUMBER_OF_ROOTS, positions, erasure_count);
    TEST_ASSERT_NOT_NULL(erasure);
    poporon_erasure_destroy(erasure);

    erasure = poporon_erasure_create_from_positions(NUMBER_OF_ROOTS, NULL, erasure_count);
    TEST_ASSERT_NULL(erasure);

    erasure = poporon_erasure_create_from_positions(NUMBER_OF_ROOTS, positions, 0);
    TEST_ASSERT_NULL(erasure);
}

void test_erasure_add_position(void)
{
    poporon_erasure_t *erasure;
    uint32_t i;

    erasure = poporon_erasure_create(NUMBER_OF_ROOTS, INITIAL_CAPACITY);
    TEST_ASSERT_NOT_NULL(erasure);

    for (i = 0; i < INITIAL_CAPACITY; i++) {
        TEST_ASSERT_TRUE(poporon_erasure_add_position(erasure, i));
    }

    for (i = INITIAL_CAPACITY; i < INITIAL_CAPACITY + 10; i++) {
        TEST_ASSERT_TRUE(poporon_erasure_add_position(erasure, i));
    }

    TEST_ASSERT_FALSE(poporon_erasure_add_position(NULL, 0));

    poporon_erasure_destroy(erasure);
}

void test_erasure_reset(void)
{
    poporon_erasure_t *erasure;
    uint32_t i;

    erasure = poporon_erasure_create(NUMBER_OF_ROOTS, INITIAL_CAPACITY);
    TEST_ASSERT_NOT_NULL(erasure);

    for (i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(poporon_erasure_add_position(erasure, i));
    }

    poporon_erasure_reset(erasure);

    for (i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(poporon_erasure_add_position(erasure, i));
    }

    poporon_erasure_reset(NULL);

    poporon_erasure_destroy(erasure);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_erasure_create_destroy);
    RUN_TEST(test_erasure_create_from_positions);
    RUN_TEST(test_erasure_add_position);
    RUN_TEST(test_erasure_reset);

    return UNITY_END();
}
