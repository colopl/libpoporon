/*
 * libpoporon - test_rs.c
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
#include <poporon/rs.h>

#include "unity.h"
#include "util.h"

#define SYMBOL_SIZE            8
#define GENERATER_POLYNOMIAL   0x11D
#define FIRST_CONSECUTIVE_ROOT 1
#define PRIMITIVE_ELEMENT      1
#define NUMBER_OF_ROOTS        16

void setUp(void)
{
}

void tearDown(void)
{
}

void test_rs_create_destroy(void)
{
    poporon_rs_t *rs;

    rs = poporon_rs_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT,
                           NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(rs);
    poporon_rs_destroy(rs);

    /* GF(2^4) */
    rs = poporon_rs_create(4, 0x13, 1, 2, 8);
    TEST_ASSERT_NOT_NULL(rs);
    poporon_rs_destroy(rs);

    rs = poporon_rs_create(0, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS);
    TEST_ASSERT_NULL(rs);

    poporon_rs_destroy(NULL);
}

void test_rs_configurations(void)
{
    poporon_rs_t *rs;

    rs = poporon_rs_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, 4);
    TEST_ASSERT_NOT_NULL(rs);
    poporon_rs_destroy(rs);

    rs = poporon_rs_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, 8);
    TEST_ASSERT_NOT_NULL(rs);
    poporon_rs_destroy(rs);

    rs = poporon_rs_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, 32);
    TEST_ASSERT_NOT_NULL(rs);
    poporon_rs_destroy(rs);

    rs = poporon_rs_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, 0, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(rs);
    poporon_rs_destroy(rs);

    rs = poporon_rs_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, 2, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(rs);
    poporon_rs_destroy(rs);

    rs = poporon_rs_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, 2, NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(rs);
    poporon_rs_destroy(rs);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_rs_create_destroy);
    RUN_TEST(test_rs_configurations);

    return UNITY_END();
}
