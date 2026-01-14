/*
 * libpoporon - test_codec.c
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

#define SYMBOL_SIZE            8
#define GENERATER_POLYNOMIAL   0x11D
#define FIRST_CONSECUTIVE_ROOT 1
#define PRIMITIVE_ELEMENT      1
#define NUMBER_OF_ROOTS        32

#define DATA_SIZE 64

void setUp(void)
{
}

void tearDown(void)
{
}

void test_encode_u8(void)
{
    poporon_t *pprn;
    uint8_t *data, *parity, i;
    bool all_zeros;

    pprn =
        poporon_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(pprn);

    data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 0, DATA_SIZE);
    random_data(data, DATA_SIZE);

    parity = (uint8_t *)malloc(NUMBER_OF_ROOTS * sizeof(uint8_t));
    TEST_ASSERT_NOT_NULL(parity);
    memset(parity, 0, NUMBER_OF_ROOTS * sizeof(uint8_t));

    TEST_ASSERT_TRUE(poporon_encode_u8(pprn, data, DATA_SIZE, parity));

    all_zeros = true;
    for (i = 0; i < NUMBER_OF_ROOTS; i++) {
        if (parity[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(all_zeros);

    TEST_ASSERT_FALSE(poporon_encode_u8(NULL, data, DATA_SIZE, parity));

    TEST_ASSERT_FALSE(poporon_encode_u8(pprn, NULL, DATA_SIZE, parity));

    TEST_ASSERT_FALSE(poporon_encode_u8(pprn, data, DATA_SIZE, NULL));

    poporon_destroy(pprn);
    free(data);
    free(parity);
}

void test_decode_u8_with_syndrome(void)
{
    poporon_t *pprn;
    uint8_t *data, *copy_data, *parity;
    uint16_t syndrome[NUMBER_OF_ROOTS];
    size_t corrected_num;
    uint32_t i;

    pprn =
        poporon_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(pprn);

    data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 0, DATA_SIZE);
    random_data(data, DATA_SIZE);

    copy_data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(copy_data);

    parity = (uint8_t *)malloc(NUMBER_OF_ROOTS * sizeof(uint8_t));
    TEST_ASSERT_NOT_NULL(parity);
    memset(parity, 0, NUMBER_OF_ROOTS * sizeof(uint8_t));

    TEST_ASSERT_TRUE(poporon_encode_u8(pprn, data, DATA_SIZE, parity));

    TEST_ASSERT_FALSE(poporon_decode_u8_with_syndrome(NULL, data, parity, DATA_SIZE, syndrome, &corrected_num));
    TEST_ASSERT_FALSE(poporon_decode_u8_with_syndrome(pprn, NULL, parity, DATA_SIZE, syndrome, &corrected_num));
    TEST_ASSERT_FALSE(poporon_decode_u8_with_syndrome(pprn, data, NULL, DATA_SIZE, syndrome, &corrected_num));
    TEST_ASSERT_FALSE(poporon_decode_u8_with_syndrome(pprn, data, parity, 0, syndrome, &corrected_num));
    TEST_ASSERT_FALSE(poporon_decode_u8_with_syndrome(pprn, data, parity, DATA_SIZE, NULL, &corrected_num));

    memset(syndrome, 0xFF, NUMBER_OF_ROOTS * sizeof(uint16_t));
    for (i = 0; i < NUMBER_OF_ROOTS; i++) {
        syndrome[i] = 0xFF;
    }

    memcpy(copy_data, data, DATA_SIZE);
    corrected_num = 0;
    TEST_ASSERT_TRUE(poporon_decode_u8_with_syndrome(pprn, copy_data, parity, DATA_SIZE, syndrome, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(0, corrected_num);
    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);

    poporon_destroy(pprn);
    free(data);
    free(copy_data);
    free(parity);
}

void test_decode_u8_with_erasure(void)
{
    poporon_t *pprn;
    poporon_erasure_t *erasure;
    uint8_t *data, *copy_data, *parity;
    size_t corrected_num;
    uint32_t i;

    pprn =
        poporon_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(pprn);

    data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 0, DATA_SIZE);
    random_data(data, DATA_SIZE);

    copy_data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(copy_data);
    memcpy(copy_data, data, DATA_SIZE);

    parity = (uint8_t *)malloc(NUMBER_OF_ROOTS * sizeof(uint8_t));
    TEST_ASSERT_NOT_NULL(parity);
    memset(parity, 0, NUMBER_OF_ROOTS * sizeof(uint8_t));

    TEST_ASSERT_TRUE(poporon_encode_u8(pprn, data, DATA_SIZE, parity));

    erasure = poporon_erasure_create(NUMBER_OF_ROOTS, NUMBER_OF_ROOTS / 2);
    TEST_ASSERT_NOT_NULL(erasure);

    memcpy(copy_data, data, DATA_SIZE);

    break_data_erasure(copy_data, DATA_SIZE, NUMBER_OF_ROOTS / 2, erasure);

    corrected_num = 0;
    TEST_ASSERT_TRUE(poporon_decode_u8_with_erasure(pprn, copy_data, DATA_SIZE, parity, erasure, &corrected_num));

    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);

    TEST_ASSERT_FALSE(poporon_decode_u8_with_erasure(NULL, copy_data, DATA_SIZE, parity, erasure, &corrected_num));

    TEST_ASSERT_FALSE(poporon_decode_u8_with_erasure(pprn, NULL, DATA_SIZE, parity, erasure, &corrected_num));

    TEST_ASSERT_FALSE(poporon_decode_u8_with_erasure(pprn, copy_data, DATA_SIZE, NULL, erasure, &corrected_num));

    TEST_ASSERT_FALSE(poporon_decode_u8_with_erasure(pprn, copy_data, 0, parity, erasure, &corrected_num));

    TEST_ASSERT_FALSE(poporon_decode_u8_with_erasure(pprn, copy_data, DATA_SIZE, parity, NULL, &corrected_num));

    poporon_erasure_destroy(erasure);
    poporon_destroy(pprn);
    free(data);
    free(copy_data);
    free(parity);
}

void test_decode_u8(void)
{
    poporon_t *pprn;
    uint8_t *data, *copy_data, *parity;
    size_t corrected_num;
    uint32_t i;

    pprn =
        poporon_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(pprn);

    data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 0, DATA_SIZE);
    random_data(data, DATA_SIZE);

    copy_data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(copy_data);
    memcpy(copy_data, data, DATA_SIZE);

    parity = (uint8_t *)malloc(NUMBER_OF_ROOTS * sizeof(uint8_t));
    TEST_ASSERT_NOT_NULL(parity);
    memset(parity, 0, NUMBER_OF_ROOTS * sizeof(uint8_t));

    TEST_ASSERT_TRUE(poporon_encode_u8(pprn, data, DATA_SIZE, parity));

    corrected_num = 0;
    TEST_ASSERT_TRUE(poporon_decode_u8(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(0, corrected_num);

    corrected_num = 0;
    memcpy(copy_data, data, DATA_SIZE);
    break_data(copy_data, DATA_SIZE, 1);
    TEST_ASSERT_TRUE(poporon_decode_u8(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(1, corrected_num);
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

    TEST_ASSERT_FALSE(poporon_decode_u8(NULL, copy_data, DATA_SIZE, parity, &corrected_num));

    TEST_ASSERT_FALSE(poporon_decode_u8(pprn, NULL, DATA_SIZE, parity, &corrected_num));

    TEST_ASSERT_FALSE(poporon_decode_u8(pprn, copy_data, DATA_SIZE, NULL, &corrected_num));

    TEST_ASSERT_FALSE(poporon_decode_u8(pprn, copy_data, 0, parity, &corrected_num));

    memcpy(copy_data, data, DATA_SIZE);
    break_data(copy_data, DATA_SIZE, 1);
    TEST_ASSERT_TRUE(poporon_decode_u8(pprn, copy_data, DATA_SIZE, parity, NULL));

    poporon_destroy(pprn);
    free(data);
    free(copy_data);
    free(parity);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_encode_u8);
    RUN_TEST(test_decode_u8_with_syndrome);
    RUN_TEST(test_decode_u8_with_erasure);
    RUN_TEST(test_decode_u8);

    return UNITY_END();
}
