/*
 * libpoporon - test_rng.c
 * 
 * This file is part of libpoporon.
 * 
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <poporon/rng.h>

#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_rng_create_destroy(void)
{
    poporon_rng_t *rng;
    uint32_t seed = 12345;

    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    TEST_ASSERT_NOT_NULL(rng);

    poporon_rng_destroy(rng);
}

void test_rng_create_null_seed(void)
{
    poporon_rng_t *rng;

    rng = poporon_rng_create(XOSHIRO128PP, NULL, 0);
    TEST_ASSERT_NOT_NULL(rng);

    poporon_rng_destroy(rng);
}

void test_rng_next_basic(void)
{
    poporon_rng_t *rng;
    uint32_t seed, val1, val2;
    bool result;

    seed = 42;
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    TEST_ASSERT_NOT_NULL(rng);

    result = poporon_rng_next(rng, &val1, sizeof(val1));
    TEST_ASSERT_TRUE(result);

    result = poporon_rng_next(rng, &val2, sizeof(val2));
    TEST_ASSERT_TRUE(result);

    TEST_ASSERT_NOT_EQUAL(val1, val2);

    poporon_rng_destroy(rng);
}

void test_rng_deterministic(void)
{
    poporon_rng_t *rng1, *rng2;
    uint32_t seed, val1, val2;

    seed = 123456789;

    rng1 = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    rng2 = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    TEST_ASSERT_NOT_NULL(rng1);
    TEST_ASSERT_NOT_NULL(rng2);

    poporon_rng_next(rng1, &val1, sizeof(val1));
    poporon_rng_next(rng2, &val2, sizeof(val2));
    TEST_ASSERT_EQUAL_UINT32(val1, val2);

    poporon_rng_next(rng1, &val1, sizeof(val1));
    poporon_rng_next(rng2, &val2, sizeof(val2));
    TEST_ASSERT_EQUAL_UINT32(val1, val2);

    poporon_rng_next(rng1, &val1, sizeof(val1));
    poporon_rng_next(rng2, &val2, sizeof(val2));
    TEST_ASSERT_EQUAL_UINT32(val1, val2);

    poporon_rng_destroy(rng1);
    poporon_rng_destroy(rng2);
}

void test_rng_different_seeds(void)
{
    poporon_rng_t *rng1, *rng2;
    uint32_t seed1, seed2, val1, val2;

    seed1 = 111;
    seed2 = 222;

    rng1 = poporon_rng_create(XOSHIRO128PP, &seed1, sizeof(seed1));
    rng2 = poporon_rng_create(XOSHIRO128PP, &seed2, sizeof(seed2));
    TEST_ASSERT_NOT_NULL(rng1);
    TEST_ASSERT_NOT_NULL(rng2);

    poporon_rng_next(rng1, &val1, sizeof(val1));
    poporon_rng_next(rng2, &val2, sizeof(val2));
    TEST_ASSERT_NOT_EQUAL(val1, val2);

    poporon_rng_destroy(rng1);
    poporon_rng_destroy(rng2);
}

void test_rng_various_sizes(void)
{
    poporon_rng_t *rng;
    uint32_t seed;
    uint8_t buf1[1], buf3[3], buf8[8], buf16[16];
    bool result;

    seed = 0xDEADBEEF;
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    TEST_ASSERT_NOT_NULL(rng);

    result = poporon_rng_next(rng, buf1, sizeof(buf1));
    TEST_ASSERT_TRUE(result);

    result = poporon_rng_next(rng, buf3, sizeof(buf3));
    TEST_ASSERT_TRUE(result);

    result = poporon_rng_next(rng, buf8, sizeof(buf8));
    TEST_ASSERT_TRUE(result);

    result = poporon_rng_next(rng, buf16, sizeof(buf16));
    TEST_ASSERT_TRUE(result);

    poporon_rng_destroy(rng);
}

void test_rng_null_params(void)
{
    poporon_rng_t *rng;
    uint32_t seed, val;
    bool result;

    seed = 0;
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    TEST_ASSERT_NOT_NULL(rng);

    result = poporon_rng_next(NULL, &val, sizeof(val));
    TEST_ASSERT_FALSE(result);

    result = poporon_rng_next(rng, NULL, sizeof(val));
    TEST_ASSERT_FALSE(result);

    result = poporon_rng_next(rng, &val, 0);
    TEST_ASSERT_FALSE(result);

    poporon_rng_destroy(rng);
}

void test_rng_destroy_null(void)
{
    poporon_rng_destroy(NULL);
}

void test_rng_sequence_length(void)
{
    poporon_rng_t *rng;
    uint32_t seed, val, i;
    bool result;

    seed = 0xCAFEBABE;
    rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));
    TEST_ASSERT_NOT_NULL(rng);

    for (i = 0; i < 10000; i++) {
        result = poporon_rng_next(rng, &val, sizeof(val));
        TEST_ASSERT_TRUE(result);
    }

    poporon_rng_destroy(rng);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_rng_create_destroy);
    RUN_TEST(test_rng_create_null_seed);
    RUN_TEST(test_rng_next_basic);
    RUN_TEST(test_rng_deterministic);
    RUN_TEST(test_rng_different_seeds);
    RUN_TEST(test_rng_various_sizes);
    RUN_TEST(test_rng_null_params);
    RUN_TEST(test_rng_destroy_null);
    RUN_TEST(test_rng_sequence_length);

    return UNITY_END();
}
