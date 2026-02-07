/*
 * libpoporon - test_unified.c
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

void setUp(void)
{
}

void tearDown(void)
{
}

void test_rs_encode_decode(void)
{
    poporon_config_t *config;
    poporon_t *pprn;
    uint8_t data[64], original[64], parity[32];
    size_t corrected = 0;

    config = poporon_config_rs_default();
    TEST_ASSERT_NOT_NULL(config);
    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);
    TEST_ASSERT_EQUAL(PPLN_FEC_RS, poporon_get_fec_type(pprn));
    TEST_ASSERT_EQUAL(32, poporon_get_parity_size(pprn));

    random_data(data, 64);
    memcpy(original, data, 64);
    memset(parity, 0, 32);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, 64, parity));
    TEST_ASSERT_TRUE(poporon_decode(pprn, data, 64, parity, &corrected));
    TEST_ASSERT_EQUAL(0, corrected);
    TEST_ASSERT_EQUAL_MEMORY(original, data, 64);

    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_rs_error_correction(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint8_t data[64], original[64], parity[32];
    size_t corrected = 0;

    config = poporon_config_rs_default();
    TEST_ASSERT_NOT_NULL(config);
    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);

    random_data(data, 64);
    memcpy(original, data, 64);
    memset(parity, 0, 32);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, 64, parity));

    break_data(data, 64, 10);

    TEST_ASSERT_TRUE(poporon_decode(pprn, data, 64, parity, &corrected));
    TEST_ASSERT_GREATER_THAN(0, corrected);
    TEST_ASSERT_EQUAL_MEMORY(original, data, 64);

    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_rs_with_erasure(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    poporon_erasure_t *eras;
    uint8_t data[64], original[64], parity[32];
    size_t corrected = 0;

    eras = poporon_erasure_create(32, 20);
    TEST_ASSERT_NOT_NULL(eras);

    config = poporon_rs_config_create(8, 0x11D, 1, 1, 32, eras, NULL);
    TEST_ASSERT_NOT_NULL(config);
    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);

    random_data(data, 64);
    memcpy(original, data, 64);
    memset(parity, 0, 32);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, 64, parity));

    break_data_erasure(data, 64, 20, eras);

    TEST_ASSERT_TRUE(poporon_decode(pprn, data, 64, parity, &corrected));
    TEST_ASSERT_EQUAL_MEMORY(original, data, 64);

    poporon_erasure_destroy(eras);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_ldpc_rate_1_3_encode_decode(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint8_t *data, *original, *parity;
    size_t block_size = 64;
    size_t parity_size, corrected = 0;

    config = poporon_ldpc_config_create(block_size, PPRN_LDPC_RATE_1_3, PPRN_LDPC_RANDOM, 3, false, false, false, 0, 0,
                                        0, NULL, 0, 0);
    TEST_ASSERT_NOT_NULL(config);

    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);
    TEST_ASSERT_EQUAL(PPLN_FEC_LDPC, poporon_get_fec_type(pprn));

    parity_size = poporon_get_parity_size(pprn);
    TEST_ASSERT_GREATER_THAN(0, parity_size);

    data = (uint8_t *)malloc(block_size);
    original = (uint8_t *)malloc(block_size);
    parity = (uint8_t *)malloc(parity_size);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_NOT_NULL(original);
    TEST_ASSERT_NOT_NULL(parity);

    random_data(data, block_size);
    memcpy(original, data, block_size);
    memset(parity, 0, parity_size);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, block_size, parity));
    TEST_ASSERT_TRUE(poporon_decode(pprn, data, block_size, parity, &corrected));
    TEST_ASSERT_EQUAL_MEMORY(original, data, block_size);

    free(data);
    free(original);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_ldpc_encode_decode_hard(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint8_t *data, *original, *parity;
    size_t block_size = 64;
    size_t parity_size, corrected = 0;

    config = poporon_ldpc_config_create(block_size, PPRN_LDPC_RATE_1_2, PPRN_LDPC_RANDOM, 3, false, false, false, 0, 0,
                                        0, NULL, 0, 0);
    TEST_ASSERT_NOT_NULL(config);

    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);
    TEST_ASSERT_EQUAL(PPLN_FEC_LDPC, poporon_get_fec_type(pprn));

    parity_size = poporon_get_parity_size(pprn);
    TEST_ASSERT_GREATER_THAN(0, parity_size);

    data = (uint8_t *)malloc(block_size);
    original = (uint8_t *)malloc(block_size);
    parity = (uint8_t *)malloc(parity_size);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_NOT_NULL(original);
    TEST_ASSERT_NOT_NULL(parity);

    random_data(data, block_size);
    memcpy(original, data, block_size);
    memset(parity, 0, parity_size);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, block_size, parity));
    TEST_ASSERT_TRUE(poporon_decode(pprn, data, block_size, parity, &corrected));
    TEST_ASSERT_EQUAL_MEMORY(original, data, block_size);

    free(data);
    free(original);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_ldpc_with_inner_interleave(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint8_t *data, *original, *parity;
    size_t block_size = 64, parity_size, corrected = 0;

    config = poporon_ldpc_config_create(block_size, PPRN_LDPC_RATE_1_2, PPRN_LDPC_RANDOM, 3, false, false, true, 0, 0,
                                        0, NULL, 0, 0);
    TEST_ASSERT_NOT_NULL(config);

    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);

    parity_size = poporon_get_parity_size(pprn);
    data = (uint8_t *)malloc(block_size);
    original = (uint8_t *)malloc(block_size);
    parity = (uint8_t *)malloc(parity_size);

    random_data(data, block_size);
    memcpy(original, data, block_size);
    memset(parity, 0, parity_size);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, block_size, parity));
    TEST_ASSERT_TRUE(poporon_decode(pprn, data, block_size, parity, &corrected));
    TEST_ASSERT_EQUAL_MEMORY(original, data, block_size);

    free(data);
    free(original);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_ldpc_with_outer_interleave(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint8_t *data, *original, *parity;
    size_t block_size = 64, parity_size, corrected = 0;

    config = poporon_ldpc_config_create(block_size, PPRN_LDPC_RATE_1_2, PPRN_LDPC_RANDOM, 3, false, true, false, 0, 0,
                                        0, NULL, 0, 0);
    TEST_ASSERT_NOT_NULL(config);

    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);

    parity_size = poporon_get_parity_size(pprn);
    data = (uint8_t *)malloc(block_size);
    original = (uint8_t *)malloc(block_size);
    parity = (uint8_t *)malloc(parity_size);

    random_data(data, block_size);
    memcpy(original, data, block_size);
    memset(parity, 0, parity_size);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, block_size, parity));
    TEST_ASSERT_TRUE(poporon_decode(pprn, data, block_size, parity, &corrected));
    TEST_ASSERT_EQUAL_MEMORY(original, data, block_size);

    free(data);
    free(original);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_ldpc_with_both_interleaves(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint8_t *data, *original, *parity;
    size_t block_size = 64, parity_size, corrected = 0;

    config = poporon_ldpc_config_create(block_size, PPRN_LDPC_RATE_1_2, PPRN_LDPC_RANDOM, 3, false, true, true, 0, 0, 0,
                                        NULL, 0, 0);
    TEST_ASSERT_NOT_NULL(config);

    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);

    parity_size = poporon_get_parity_size(pprn);
    data = (uint8_t *)malloc(block_size);
    original = (uint8_t *)malloc(block_size);
    parity = (uint8_t *)malloc(parity_size);

    random_data(data, block_size);
    memcpy(original, data, block_size);
    memset(parity, 0, parity_size);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, block_size, parity));
    TEST_ASSERT_TRUE(poporon_decode(pprn, data, block_size, parity, &corrected));
    TEST_ASSERT_EQUAL_MEMORY(original, data, block_size);

    free(data);
    free(original);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_ldpc_burst_resistant(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint8_t *data, *original, *parity;
    size_t block_size = 128, parity_size, corrected = 0;

    config = poporon_config_ldpc_burst_resistant(block_size, PPRN_LDPC_RATE_1_2);
    TEST_ASSERT_NOT_NULL(config);

    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);

    parity_size = poporon_get_parity_size(pprn);
    data = (uint8_t *)malloc(block_size);
    original = (uint8_t *)malloc(block_size);
    parity = (uint8_t *)malloc(parity_size);

    random_data(data, block_size);
    memcpy(original, data, block_size);
    memset(parity, 0, parity_size);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, block_size, parity));

    TEST_ASSERT_TRUE(poporon_decode(pprn, data, block_size, parity, &corrected));
    TEST_ASSERT_EQUAL_MEMORY(original, data, block_size);

    free(data);
    free(original);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_ldpc_iterations_getter(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint32_t iters;
    uint8_t *data, *parity;
    size_t block_size = 64, parity_size, corrected = 0;

    config = poporon_ldpc_config_create(block_size, PPRN_LDPC_RATE_1_2, PPRN_LDPC_RANDOM, 3, false, false, false, 0, 0,
                                        0, NULL, 0, 0);
    TEST_ASSERT_NOT_NULL(config);

    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);

    parity_size = poporon_get_parity_size(pprn);
    data = (uint8_t *)malloc(block_size);
    parity = (uint8_t *)malloc(parity_size);

    random_data(data, block_size);
    memset(parity, 0, parity_size);

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, block_size, parity));
    TEST_ASSERT_TRUE(poporon_decode(pprn, data, block_size, parity, &corrected));

    iters = poporon_get_iterations_used(pprn);
    TEST_ASSERT_EQUAL(0, iters);

    free(data);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_bch_encode_decode_no_error(void)
{
    poporon_t *pprn;
    poporon_config_t *config;
    uint8_t data[2], parity[2];
    size_t corrected = 0;

    config = poporon_config_bch_default();
    TEST_ASSERT_NOT_NULL(config);
    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);
    TEST_ASSERT_EQUAL(PPLN_FEC_BCH, poporon_get_fec_type(pprn));

    memset(data, 0, sizeof(data));
    memset(parity, 0, sizeof(parity));
    data[0] = 0x15;

    TEST_ASSERT_TRUE(poporon_encode(pprn, data, 1, parity));
    TEST_ASSERT_TRUE(poporon_decode(pprn, data, 1, parity, &corrected));
    TEST_ASSERT_EQUAL(0, corrected);

    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_create_null_config(void)
{
    TEST_ASSERT_NULL(poporon_create(NULL));
}

void test_encode_null(void)
{
    poporon_t *pprn;
    poporon_config_t *config;

    config = poporon_rs_config_create(8, 0x11D, 1, 1, 10, NULL, NULL);
    TEST_ASSERT_NOT_NULL(config);
    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);

    TEST_ASSERT_FALSE(poporon_encode(pprn, NULL, 10, NULL));
    TEST_ASSERT_FALSE(poporon_encode(NULL, NULL, 10, NULL));

    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

void test_decode_null(void)
{
    poporon_t *pprn;
    poporon_config_t *config;

    config = poporon_rs_config_create(8, 0x11D, 1, 1, 10, NULL, NULL);
    TEST_ASSERT_NOT_NULL(config);
    pprn = poporon_create(config);
    TEST_ASSERT_NOT_NULL(pprn);

    TEST_ASSERT_FALSE(poporon_decode(pprn, NULL, 10, NULL, NULL));
    TEST_ASSERT_FALSE(poporon_decode(NULL, NULL, 10, NULL, NULL));

    poporon_destroy(pprn);
    poporon_config_destroy(config);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_rs_encode_decode);
    RUN_TEST(test_rs_error_correction);
    RUN_TEST(test_rs_with_erasure);

    RUN_TEST(test_ldpc_rate_1_3_encode_decode);
    RUN_TEST(test_ldpc_encode_decode_hard);
    RUN_TEST(test_ldpc_with_inner_interleave);
    RUN_TEST(test_ldpc_with_outer_interleave);
    RUN_TEST(test_ldpc_with_both_interleaves);
    RUN_TEST(test_ldpc_burst_resistant);
    RUN_TEST(test_ldpc_iterations_getter);

    RUN_TEST(test_bch_encode_decode_no_error);

    RUN_TEST(test_create_null_config);
    RUN_TEST(test_encode_null);
    RUN_TEST(test_decode_null);

    return UNITY_END();
}
