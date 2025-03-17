/*
 * libpoporon test - test_u8.c
 * 
 * Copyright (c) 2025 Go Kudo
 *
 * This library is licensed under the BSD 3-Clause License.
 * For license details, please refer to the LICENSE file.
 *
 * SPDX-FileCopyrightText: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "poporon.h"

#include "test_util.h"

#include "unity.h"

#define SYMBOL_SIZE             8
#define GENERATER_POLYNOMIAL    0x11D
#define FIRST_CONSECUTIVE_ROOT  1
#define PRIMITIVE_ELEMENT       1
#define NUMBER_OF_ROOTS         128

#define DATA_SIZE               127

void setUp(void)
{
}

void tearDown(void)
{
}

void test_codec_u8(void)
{
    poporon_t *pprn;
    uint32_t i;
    uint8_t *data, *copy_data, *parity;
    size_t corrected_num;

    pprn = poporon_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(pprn);

    data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 0, DATA_SIZE);
    random_data(data, DATA_SIZE);

    parity = (uint8_t *)malloc(NUMBER_OF_ROOTS * sizeof(uint8_t));
    TEST_ASSERT_NOT_NULL(parity);
    memset(parity, 0, NUMBER_OF_ROOTS * sizeof(uint8_t));

    copy_data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(copy_data);
    memcpy(copy_data, data, DATA_SIZE);
    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);

    TEST_ASSERT_TRUE(poporon_encode_u8(pprn, data, DATA_SIZE, parity));
    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);

    corrected_num = 0;
    break_data(copy_data, DATA_SIZE, 1);
    TEST_ASSERT_TRUE(poporon_decode_u8(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(1, corrected_num);
    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);

    corrected_num = 0;
    break_data(copy_data, DATA_SIZE, 3);
    TEST_ASSERT_TRUE(poporon_decode_u8(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(3, corrected_num);
    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);
    
    corrected_num = 0;
    break_data(copy_data, DATA_SIZE, 8);
    TEST_ASSERT_TRUE(poporon_decode_u8(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(8, corrected_num);
    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);

    for (i = 1; i <= NUMBER_OF_ROOTS / 2; i++) {
        corrected_num = 0;
        memcpy(copy_data, data, DATA_SIZE);
        break_data(copy_data, DATA_SIZE, i);
        TEST_ASSERT_TRUE(poporon_decode_u8(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
        TEST_ASSERT_EQUAL_size_t(i, corrected_num);
        TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);
    }

    corrected_num = 0;
    memcpy(copy_data, data, DATA_SIZE);
    break_data(copy_data, DATA_SIZE, NUMBER_OF_ROOTS / 2 + 1);
    TEST_ASSERT_FALSE(poporon_decode_u8(pprn, copy_data, DATA_SIZE, parity, &corrected_num));

    poporon_destroy(pprn);
    free(data);
    free(parity);
    free(copy_data);
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_codec_u8);

    return UNITY_END();
}
