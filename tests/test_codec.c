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

#define NUMBER_OF_ROOTS 32
#define DATA_SIZE       64

void setUp(void)
{
}

void tearDown(void)
{
}

static poporon_t *create_rs_instance(void)
{
    poporon_config_t *config;
    poporon_t *pprn;
    config = poporon_config_rs_default();
    pprn = poporon_create(config);
    poporon_config_destroy(config);
    return pprn;
}

void test_encode(void)
{
    poporon_t *pprn;
    uint8_t *data, *parity, i;
    bool all_zeros;

    pprn = create_rs_instance();
    TEST_ASSERT_NOT_NULL(pprn);

    data = (uint8_t *)malloc(DATA_SIZE);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 0, DATA_SIZE);
    random_data(data, DATA_SIZE);

    parity = (uint8_t *)malloc(NUMBER_OF_ROOTS * sizeof(uint8_t));
    TEST_ASSERT_NOT_NULL(parity);
    memset(parity, 0, NUMBER_OF_ROOTS * sizeof(uint8_t));

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, DATA_SIZE, parity));

    all_zeros = true;
    for (i = 0; i < NUMBER_OF_ROOTS; i++) {
        if (parity[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(all_zeros);

    TEST_ASSERT_FALSE(poporon_encode(NULL, data, DATA_SIZE, parity));
    TEST_ASSERT_FALSE(poporon_encode(pprn, NULL, DATA_SIZE, parity));
    TEST_ASSERT_FALSE(poporon_encode(pprn, data, DATA_SIZE, NULL));

    poporon_destroy(pprn);
    free(data);
    free(parity);
}

void test_decode_with_syndrome(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint32_t i;
    uint16_t syndrome[NUMBER_OF_ROOTS];
    uint8_t *data, *copy_data, *parity;
    size_t corrected_num;

    for (i = 0; i < NUMBER_OF_ROOTS; i++) {
        syndrome[i] = 0xFF;
    }

    config = poporon_rs_config_create(8, 0x11D, 1, 1, NUMBER_OF_ROOTS, NULL, syndrome);
    TEST_ASSERT_NOT_NULL(config);
    pprn = poporon_create(config);
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

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, DATA_SIZE, parity));

    memcpy(copy_data, data, DATA_SIZE);
    corrected_num = 0;
    TEST_ASSERT_TRUE(poporon_decode(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(0, corrected_num);
    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);

    poporon_destroy(pprn);
    poporon_config_destroy(config);
    free(data);
    free(copy_data);
    free(parity);
}

void test_decode_with_erasure(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    poporon_erasure_t *erasure;
    uint8_t *data, *copy_data, *parity;
    size_t corrected_num;

    erasure = poporon_erasure_create(NUMBER_OF_ROOTS, NUMBER_OF_ROOTS / 2);
    TEST_ASSERT_NOT_NULL(erasure);

    config = poporon_rs_config_create(8, 0x11D, 1, 1, NUMBER_OF_ROOTS, erasure, NULL);
    TEST_ASSERT_NOT_NULL(config);
    pprn = poporon_create(config);
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

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, DATA_SIZE, parity));

    memcpy(copy_data, data, DATA_SIZE);
    break_data_erasure(copy_data, DATA_SIZE, NUMBER_OF_ROOTS / 2, erasure);

    corrected_num = 0;
    TEST_ASSERT_TRUE(poporon_decode(pprn, copy_data, DATA_SIZE, parity, &corrected_num));

    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);

    poporon_erasure_destroy(erasure);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
    free(data);
    free(copy_data);
    free(parity);
}

void test_decode(void)
{
    poporon_t *pprn;
    uint32_t i;
    uint8_t *data, *copy_data, *parity;
    size_t corrected_num;

    pprn = create_rs_instance();
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

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, DATA_SIZE, parity));

    corrected_num = 0;
    TEST_ASSERT_TRUE(poporon_decode(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(0, corrected_num);

    corrected_num = 0;
    memcpy(copy_data, data, DATA_SIZE);
    break_data(copy_data, DATA_SIZE, 1);
    TEST_ASSERT_TRUE(poporon_decode(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(1, corrected_num);
    TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);

    for (i = 1; i <= NUMBER_OF_ROOTS / 2; i++) {
        corrected_num = 0;
        memcpy(copy_data, data, DATA_SIZE);
        break_data(copy_data, DATA_SIZE, i);
        TEST_ASSERT_TRUE(poporon_decode(pprn, copy_data, DATA_SIZE, parity, &corrected_num));
        TEST_ASSERT_EQUAL_size_t(i, corrected_num);
        TEST_ASSERT_EQUAL_MEMORY(data, copy_data, DATA_SIZE);
    }

    corrected_num = 0;
    memcpy(copy_data, data, DATA_SIZE);
    break_data(copy_data, DATA_SIZE, NUMBER_OF_ROOTS / 2 + 1);
    TEST_ASSERT_FALSE(poporon_decode(pprn, copy_data, DATA_SIZE, parity, &corrected_num));

    TEST_ASSERT_FALSE(poporon_decode(NULL, copy_data, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_FALSE(poporon_decode(pprn, NULL, DATA_SIZE, parity, &corrected_num));
    TEST_ASSERT_FALSE(poporon_decode(pprn, copy_data, DATA_SIZE, NULL, &corrected_num));
    TEST_ASSERT_FALSE(poporon_decode(pprn, copy_data, 0, parity, &corrected_num));

    memcpy(copy_data, data, DATA_SIZE);
    break_data(copy_data, DATA_SIZE, 1);
    TEST_ASSERT_TRUE(poporon_decode(pprn, copy_data, DATA_SIZE, parity, NULL));

    poporon_destroy(pprn);
    free(data);
    free(copy_data);
    free(parity);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_encode);
    RUN_TEST(test_decode_with_syndrome);
    RUN_TEST(test_decode_with_erasure);
    RUN_TEST(test_decode);

    return UNITY_END();
}
