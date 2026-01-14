/*
 * libpoporon - test_bch.c
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
#include <poporon/bch.h>

#include "unity.h"
#include "util.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_bch_create_destroy(void)
{
    poporon_bch_t *bch;

    bch = poporon_bch_create(4, 0x13, 3);
    TEST_ASSERT_NOT_NULL(bch);
    TEST_ASSERT_EQUAL_UINT16(15, poporon_bch_get_codeword_length(bch));
    TEST_ASSERT_EQUAL_UINT8(3, poporon_bch_get_correction_capability(bch));
    poporon_bch_destroy(bch);

    bch = poporon_bch_create(5, 0x25, 3);
    TEST_ASSERT_NOT_NULL(bch);
    TEST_ASSERT_EQUAL_UINT16(31, poporon_bch_get_codeword_length(bch));
    poporon_bch_destroy(bch);

    bch = poporon_bch_create(6, 0x43, 2);
    TEST_ASSERT_NOT_NULL(bch);
    TEST_ASSERT_EQUAL_UINT16(63, poporon_bch_get_codeword_length(bch));
    poporon_bch_destroy(bch);

    bch = poporon_bch_create(2, 0x07, 1);
    TEST_ASSERT_NULL(bch);

    bch = poporon_bch_create(17, 0x0001, 1);
    TEST_ASSERT_NULL(bch);

    bch = poporon_bch_create(4, 0x13, 0);
    TEST_ASSERT_NULL(bch);

    poporon_bch_destroy(NULL);
}

void test_bch_encode_decode_no_errors(void)
{
    poporon_bch_t *bch;
    uint32_t codeword, corrected, data, extracted;
    int32_t num_errors;
    bool result;

    bch = poporon_bch_create(4, 0x13, 3);
    TEST_ASSERT_NOT_NULL(bch);

    result = poporon_bch_encode(bch, 0, &codeword);
    TEST_ASSERT_TRUE(result);

    result = poporon_bch_decode(bch, codeword, &corrected, &num_errors);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(codeword, corrected);
    TEST_ASSERT_EQUAL_INT32(0, num_errors);

    for (data = 0; data < 32; data++) {
        result = poporon_bch_encode(bch, data, &codeword);
        TEST_ASSERT_TRUE(result);

        result = poporon_bch_decode(bch, codeword, &corrected, &num_errors);
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL_UINT32(codeword, corrected);
        TEST_ASSERT_EQUAL_INT32(0, num_errors);

        extracted = poporon_bch_extract_data(bch, corrected);
        TEST_ASSERT_EQUAL_UINT32(data, extracted);
    }

    poporon_bch_destroy(bch);
}

void test_bch_single_bit_error_correction(void)
{
    poporon_bch_t *bch;
    uint32_t codeword, corrupted, corrected, original_data, extracted;
    uint16_t n;
    int32_t num_errors, bit;
    bool result;

    bch = poporon_bch_create(4, 0x13, 3);
    TEST_ASSERT_NOT_NULL(bch);

    original_data = 21;
    result = poporon_bch_encode(bch, original_data, &codeword);
    TEST_ASSERT_TRUE(result);

    n = poporon_bch_get_codeword_length(bch);
    for (bit = 0; bit < n; bit++) {
        corrupted = codeword ^ (1U << bit);

        result = poporon_bch_decode(bch, corrupted, &corrected, &num_errors);
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL_UINT32(codeword, corrected);
        TEST_ASSERT_EQUAL_INT32(1, num_errors);

        extracted = poporon_bch_extract_data(bch, corrected);
        TEST_ASSERT_EQUAL_UINT32(original_data, extracted);
    }

    poporon_bch_destroy(bch);
}

void test_bch_double_bit_error_correction(void)
{
    poporon_bch_t *bch;
    uint32_t codeword, corrupted, corrected, original_data, extracted;
    uint16_t n;
    int32_t num_errors, bit1, bit2;
    bool result;

    bch = poporon_bch_create(4, 0x13, 3);
    TEST_ASSERT_NOT_NULL(bch);

    original_data = 7;
    result = poporon_bch_encode(bch, original_data, &codeword);
    TEST_ASSERT_TRUE(result);

    n = poporon_bch_get_codeword_length(bch);

    for (bit1 = 0; bit1 < n - 1; bit1++) {
        for (bit2 = bit1 + 1; bit2 < n; bit2++) {
            corrupted = codeword ^ (1U << bit1) ^ (1U << bit2);

            result = poporon_bch_decode(bch, corrupted, &corrected, &num_errors);
            TEST_ASSERT_TRUE(result);
            TEST_ASSERT_EQUAL_UINT32(codeword, corrected);
            TEST_ASSERT_EQUAL_INT32(2, num_errors);

            extracted = poporon_bch_extract_data(bch, corrected);
            TEST_ASSERT_EQUAL_UINT32(original_data, extracted);
        }
    }

    poporon_bch_destroy(bch);
}

void test_bch_triple_bit_error_correction(void)
{
    poporon_bch_t *bch;
    uint32_t codeword, corrupted, corrected, original_data = 15, extracted;
    int32_t num_errors, test_patterns[][3] = {{0, 1, 2}, {0, 7, 14}, {3, 8, 12}, {1, 5, 10}, {4, 9, 13}}, i;
    bool result;

    bch = poporon_bch_create(4, 0x13, 3);
    TEST_ASSERT_NOT_NULL(bch);

    original_data = 15;
    result = poporon_bch_encode(bch, original_data, &codeword);
    TEST_ASSERT_TRUE(result);

    for (i = 0; i < 5; i++) {
        corrupted = codeword ^ (1U << test_patterns[i][0]) ^ (1U << test_patterns[i][1]) ^ (1U << test_patterns[i][2]);

        result = poporon_bch_decode(bch, corrupted, &corrected, &num_errors);
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL_UINT32(codeword, corrected);
        TEST_ASSERT_EQUAL_INT32(3, num_errors);

        extracted = poporon_bch_extract_data(bch, corrected);
        TEST_ASSERT_EQUAL_UINT32(original_data, extracted);
    }

    poporon_bch_destroy(bch);
}

void test_bch_too_many_errors(void)
{
    poporon_bch_t *bch;
    uint32_t codeword, corrupted, corrected, original_data = 3;
    int32_t num_errors;
    bool result;

    bch = poporon_bch_create(4, 0x13, 3);
    TEST_ASSERT_NOT_NULL(bch);

    result = poporon_bch_encode(bch, original_data, &codeword);
    TEST_ASSERT_TRUE(result);

    corrupted = codeword ^ (1U << 0) ^ (1U << 3) ^ (1U << 7) ^ (1U << 11);

    result = poporon_bch_decode(bch, corrupted, &corrected, &num_errors);

    poporon_bch_destroy(bch);
}

void test_bch_encode_invalid_data(void)
{
    poporon_bch_t *bch;
    uint32_t codeword;
    uint16_t k;
    bool result;

    bch = poporon_bch_create(4, 0x13, 3);
    TEST_ASSERT_NOT_NULL(bch);

    k = poporon_bch_get_data_length(bch);

    result = poporon_bch_encode(bch, (1U << k), &codeword);
    TEST_ASSERT_FALSE(result);

    result = poporon_bch_encode(NULL, 0, &codeword);
    TEST_ASSERT_FALSE(result);

    result = poporon_bch_encode(bch, 0, NULL);
    TEST_ASSERT_FALSE(result);

    poporon_bch_destroy(bch);
}

void test_bch_decode_invalid_params(void)
{
    poporon_bch_t *bch;
    uint32_t corrected;
    bool result;

    bch = poporon_bch_create(4, 0x13, 3);
    TEST_ASSERT_NOT_NULL(bch);

    result = poporon_bch_decode(NULL, 0, &corrected, NULL);
    TEST_ASSERT_FALSE(result);

    result = poporon_bch_decode(bch, 0, NULL, NULL);
    TEST_ASSERT_FALSE(result);

    poporon_bch_destroy(bch);
}

void test_bch_all_data_values(void)
{
    poporon_bch_t *bch;
    uint32_t codeword, corrected, data, extracted;
    int32_t num_errors;
    bool result;

    bch = poporon_bch_create(4, 0x13, 3);
    TEST_ASSERT_NOT_NULL(bch);

    for (data = 0; data < 32; data++) {
        result = poporon_bch_encode(bch, data, &codeword);
        TEST_ASSERT_TRUE(result);

        result = poporon_bch_decode(bch, codeword, &corrected, &num_errors);
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL_INT32(0, num_errors);
        TEST_ASSERT_EQUAL_UINT32(codeword, corrected);
    }

    poporon_bch_destroy(bch);
}

void test_bch_different_field_sizes(void)
{
    poporon_bch_t *bch;
    uint32_t codeword, corrupted, corrected, max_data;
    uint16_t k;
    int32_t num_errors;
    bool result;

    bch = poporon_bch_create(5, 0x25, 2);
    TEST_ASSERT_NOT_NULL(bch);
    TEST_ASSERT_EQUAL_UINT16(31, poporon_bch_get_codeword_length(bch));

    k = poporon_bch_get_data_length(bch);
    max_data = (1U << k) - 1;

    result = poporon_bch_encode(bch, max_data, &codeword);
    TEST_ASSERT_TRUE(result);

    corrupted = codeword ^ (1U << 15);
    result = poporon_bch_decode(bch, corrupted, &corrected, &num_errors);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(codeword, corrected);
    TEST_ASSERT_EQUAL_INT32(1, num_errors);

    corrupted = codeword ^ (1U << 5) ^ (1U << 20);
    result = poporon_bch_decode(bch, corrupted, &corrected, &num_errors);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(codeword, corrected);
    TEST_ASSERT_EQUAL_INT32(2, num_errors);

    poporon_bch_destroy(bch);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_bch_create_destroy);
    RUN_TEST(test_bch_encode_decode_no_errors);
    RUN_TEST(test_bch_single_bit_error_correction);
    RUN_TEST(test_bch_double_bit_error_correction);
    RUN_TEST(test_bch_triple_bit_error_correction);
    RUN_TEST(test_bch_too_many_errors);
    RUN_TEST(test_bch_encode_invalid_data);
    RUN_TEST(test_bch_decode_invalid_params);
    RUN_TEST(test_bch_all_data_values);
    RUN_TEST(test_bch_different_field_sizes);

    return UNITY_END();
}
