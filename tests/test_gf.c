/*
 * libpoporon - test_gf.c
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
#include <poporon/gf.h>

#include "unity.h"
#include "util.h"

#define SYMBOL_SIZE          8
#define GENERATER_POLYNOMIAL 0x11D

void setUp(void)
{
}

void tearDown(void)
{
}

void test_gf_create_destroy(void)
{
    poporon_gf_t *gf;

    gf = poporon_gf_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL);
    TEST_ASSERT_NOT_NULL(gf);
    poporon_gf_destroy(gf);

    /* GF(2^4) with primitive polynomial x^4 + x + 1 */
    gf = poporon_gf_create(4, 0x13);
    TEST_ASSERT_NOT_NULL(gf);
    poporon_gf_destroy(gf);

    gf = poporon_gf_create(0, GENERATER_POLYNOMIAL);
    TEST_ASSERT_NULL(gf);

    gf = poporon_gf_create(17, GENERATER_POLYNOMIAL); /* Should fail, > 16 */
    TEST_ASSERT_NULL(gf);

    poporon_gf_destroy(NULL);
}

void test_gf_mod(void)
{
    poporon_gf_t *gf;
    uint8_t result;

    gf = poporon_gf_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL);
    TEST_ASSERT_NOT_NULL(gf);

    result = poporon_gf_mod(gf, 0);
    TEST_ASSERT_EQUAL_UINT8(0, result);

    result = poporon_gf_mod(gf, 1);
    TEST_ASSERT_EQUAL_UINT8(1, result);

    result = poporon_gf_mod(gf, 256);
    TEST_ASSERT_EQUAL_UINT8(1, result);

    result = poporon_gf_mod(gf, 257);
    TEST_ASSERT_EQUAL_UINT8(2, result);

    for (uint16_t i = 0; i < 255; i++) {
        result = poporon_gf_mod(gf, i);
        TEST_ASSERT_EQUAL_UINT8(i, result);
    }

    poporon_gf_destroy(gf);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_gf_create_destroy);
    RUN_TEST(test_gf_mod);

    return UNITY_END();
}
