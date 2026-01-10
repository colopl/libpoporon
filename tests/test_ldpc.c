/*
 * libpoporon - test_ldpc.c
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <poporon/ldpc.h>

#include <unity.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_ldpc_create_destroy(void)
{
    poporon_ldpc_t *ldpc;

    ldpc = poporon_ldpc_create(64, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);
    TEST_ASSERT_GREATER_THAN(0, poporon_ldpc_info_size(ldpc));
    TEST_ASSERT_GREATER_THAN(0, poporon_ldpc_codeword_size(ldpc));
    TEST_ASSERT_GREATER_THAN(0, poporon_ldpc_parity_size(ldpc));
    poporon_ldpc_destroy(ldpc);

    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_2_3, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);
    poporon_ldpc_destroy(ldpc);

    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_3_4, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);
    poporon_ldpc_destroy(ldpc);

    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_5_6, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);
    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_create_invalid(void)
{
    poporon_ldpc_t *ldpc;

    ldpc = poporon_ldpc_create(8, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NULL(ldpc);

    ldpc = poporon_ldpc_create(65, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NULL(ldpc);

    ldpc = poporon_ldpc_create(64, (poporon_ldpc_rate_t)100, NULL);
    TEST_ASSERT_NULL(ldpc);
}

static void test_ldpc_encode_basic(void)
{
    poporon_ldpc_t *ldpc;
    uint8_t *info, *parity, *codeword;
    size_t i, info_size, parity_size, codeword_size;

    ldpc = poporon_ldpc_create(64, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);

    info_size = poporon_ldpc_info_size(ldpc);
    parity_size = poporon_ldpc_parity_size(ldpc);
    codeword_size = poporon_ldpc_codeword_size(ldpc);

    info = (uint8_t *)malloc(info_size);
    parity = (uint8_t *)malloc(parity_size);
    codeword = (uint8_t *)malloc(codeword_size);

    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_NOT_NULL(parity);
    TEST_ASSERT_NOT_NULL(codeword);

    for (i = 0; i < info_size; i++) {
        info[i] = (uint8_t)(i * 17 + 23);
    }

    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);

    TEST_ASSERT_TRUE(poporon_ldpc_check(ldpc, codeword));

    codeword[0] ^= 0x01;
    TEST_ASSERT_FALSE(poporon_ldpc_check(ldpc, codeword));

    free(info);
    free(parity);
    free(codeword);
    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_decode_hard_no_errors(void)
{
    poporon_ldpc_t *ldpc;
    uint32_t iterations;
    uint8_t *info, *parity, *codeword;
    size_t i, info_size, parity_size, codeword_size;

    ldpc = poporon_ldpc_create(64, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);

    info_size = poporon_ldpc_info_size(ldpc);
    parity_size = poporon_ldpc_parity_size(ldpc);
    codeword_size = poporon_ldpc_codeword_size(ldpc);

    info = (uint8_t *)malloc(info_size);
    parity = (uint8_t *)malloc(parity_size);
    codeword = (uint8_t *)malloc(codeword_size);

    for (i = 0; i < info_size; i++) {
        info[i] = (uint8_t)(i * 17 + 23);
    }

    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);

    TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, codeword, 50, &iterations));
    TEST_ASSERT_EQUAL(0, iterations);
    TEST_ASSERT_EQUAL_MEMORY(info, codeword, info_size);

    free(info);
    free(parity);
    free(codeword);
    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_decode_hard_with_errors(void)
{
    poporon_ldpc_t *ldpc;
    uint32_t iterations;
    uint8_t *info, *parity, *codeword, *original;
    size_t i, info_size, parity_size, codeword_size;

    ldpc = poporon_ldpc_create(64, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);

    info_size = poporon_ldpc_info_size(ldpc);
    parity_size = poporon_ldpc_parity_size(ldpc);
    codeword_size = poporon_ldpc_codeword_size(ldpc);

    info = (uint8_t *)malloc(info_size);
    parity = (uint8_t *)malloc(parity_size);
    codeword = (uint8_t *)malloc(codeword_size);
    original = (uint8_t *)malloc(codeword_size);

    for (i = 0; i < info_size; i++) {
        info[i] = (uint8_t)(i * 17 + 23);
    }

    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);
    memcpy(original, codeword, codeword_size);

    codeword[0] ^= 0x01;
    codeword[10] ^= 0x80;
    codeword[20] ^= 0x40;

    TEST_ASSERT_FALSE(poporon_ldpc_check(ldpc, codeword));

    TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, codeword, 50, &iterations));
    TEST_ASSERT_GREATER_THAN(0, iterations);
    TEST_ASSERT_EQUAL_MEMORY(original, codeword, codeword_size);

    free(info);
    free(parity);
    free(codeword);
    free(original);
    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_decode_soft_basic(void)
{
    poporon_ldpc_t *ldpc;
    uint32_t iterations;
    uint8_t *info, *parity, *codeword, *decoded;
    int8_t *llr, bit;
    size_t i, byte_idx, bit_idx, info_size, parity_size, codeword_size, codeword_bits;

    ldpc = poporon_ldpc_create(64, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);

    info_size = poporon_ldpc_info_size(ldpc);
    parity_size = poporon_ldpc_parity_size(ldpc);
    codeword_size = poporon_ldpc_codeword_size(ldpc);
    codeword_bits = codeword_size * 8;

    info = (uint8_t *)malloc(info_size);
    parity = (uint8_t *)malloc(parity_size);
    codeword = (uint8_t *)malloc(codeword_size);
    decoded = (uint8_t *)malloc(codeword_size);
    llr = (int8_t *)malloc(codeword_bits);

    for (i = 0; i < info_size; i++) {
        info[i] = (uint8_t)(i * 17 + 23);
    }

    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);

    for (i = 0; i < codeword_bits; i++) {
        byte_idx = i / 8;
        bit_idx = 7 - (i % 8);
        bit = (codeword[byte_idx] >> bit_idx) & 1;
        llr[i] = bit ? -64 : 64;
    }

    TEST_ASSERT_TRUE(poporon_ldpc_decode_soft(ldpc, llr, decoded, 50, &iterations));
    TEST_ASSERT_EQUAL_MEMORY(codeword, decoded, codeword_size);

    free(info);
    free(parity);
    free(codeword);
    free(decoded);
    free(llr);
    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_decode_soft_with_noise(void)
{
    poporon_ldpc_t *ldpc;
    uint32_t iterations;
    uint8_t *info, *parity, *codeword, *decoded;
    int8_t *llr, bit, base_llr;
    size_t i, info_size, parity_size, codeword_size, codeword_bits, byte_idx, bit_idx;

    ldpc = poporon_ldpc_create(64, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);

    info_size = poporon_ldpc_info_size(ldpc);
    parity_size = poporon_ldpc_parity_size(ldpc);
    codeword_size = poporon_ldpc_codeword_size(ldpc);
    codeword_bits = codeword_size * 8;

    info = (uint8_t *)malloc(info_size);
    parity = (uint8_t *)malloc(parity_size);
    codeword = (uint8_t *)malloc(codeword_size);
    decoded = (uint8_t *)malloc(codeword_size);
    llr = (int8_t *)malloc(codeword_bits);

    for (i = 0; i < info_size; i++) {
        info[i] = (uint8_t)(i * 17 + 23);
    }

    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);

    for (i = 0; i < codeword_bits; i++) {
        byte_idx = i / 8;
        bit_idx = 7 - (i % 8);
        bit = (codeword[byte_idx] >> bit_idx) & 1;

        base_llr = (int8_t)(32 + (i % 32));
        llr[i] = bit ? -base_llr : base_llr;
    }

    llr[5] = -llr[5];
    llr[50] = -llr[50];
    llr[100] = -llr[100];

    TEST_ASSERT_TRUE(poporon_ldpc_decode_soft(ldpc, llr, decoded, 50, &iterations));
    TEST_ASSERT_EQUAL_MEMORY(codeword, decoded, codeword_size);

    free(info);
    free(parity);
    free(codeword);
    free(decoded);
    free(llr);
    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_various_rates(void)
{
    poporon_ldpc_rate_t rates[] = {PPRN_LDPC_RATE_1_2, PPRN_LDPC_RATE_2_3, PPRN_LDPC_RATE_3_4, PPRN_LDPC_RATE_5_6};
    poporon_ldpc_t *ldpc;
    uint32_t iterations;
    uint8_t *info, *parity, *codeword;
    size_t i, r, num_rates = sizeof(rates) / sizeof(rates[0]), info_size, parity_size, codeword_size;

    for (r = 0; r < num_rates; r++) {
        ldpc = poporon_ldpc_create(128, rates[r], NULL);
        TEST_ASSERT_NOT_NULL(ldpc);

        info_size = poporon_ldpc_info_size(ldpc);
        parity_size = poporon_ldpc_parity_size(ldpc);
        codeword_size = poporon_ldpc_codeword_size(ldpc);

        info = (uint8_t *)malloc(info_size);
        parity = (uint8_t *)malloc(parity_size);
        codeword = (uint8_t *)malloc(codeword_size);

        for (i = 0; i < info_size; i++) {
            info[i] = (uint8_t)(i ^ r);
        }

        TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

        memcpy(codeword, info, info_size);
        memcpy(codeword + info_size, parity, parity_size);

        TEST_ASSERT_TRUE(poporon_ldpc_check(ldpc, codeword));
        TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, codeword, 50, &iterations));
        TEST_ASSERT_EQUAL_MEMORY(info, codeword, info_size);

        free(info);
        free(parity);
        free(codeword);
        poporon_ldpc_destroy(ldpc);
    }
}

static void test_ldpc_decode_hard_block256_byte_errors(void)
{
    poporon_ldpc_t *ldpc;
    uint32_t iterations;
    uint8_t *info, *parity, *codeword, *original;
    size_t info_size, parity_size, codeword_size, i;

    ldpc = poporon_ldpc_create(256, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);

    info_size = poporon_ldpc_info_size(ldpc);
    parity_size = poporon_ldpc_parity_size(ldpc);
    codeword_size = poporon_ldpc_codeword_size(ldpc);

    TEST_ASSERT_EQUAL(codeword_size, info_size + parity_size);

    info = (uint8_t *)malloc(info_size);
    parity = (uint8_t *)malloc(parity_size);
    codeword = (uint8_t *)malloc(codeword_size);
    original = (uint8_t *)malloc(codeword_size);

    for (i = 0; i < info_size; i++) {
        info[i] = (uint8_t)(i * 17 + 23);
    }

    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);
    memcpy(original, codeword, codeword_size);

    codeword[5] ^= 0xAB;
    codeword[50] ^= 0xCD;
    codeword[100] ^= 0xEF;

    TEST_ASSERT_FALSE(poporon_ldpc_check(ldpc, codeword));

    TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, codeword, 100, &iterations));
    TEST_ASSERT_GREATER_THAN(0, iterations);
    TEST_ASSERT_EQUAL_MEMORY(original, codeword, codeword_size);

    free(info);
    free(parity);
    free(codeword);
    free(original);
    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_null_params(void)
{
    poporon_ldpc_t *ldpc;
    uint8_t info[64], parity[64], codeword[128];

    ldpc = poporon_ldpc_create(64, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);

    TEST_ASSERT_FALSE(poporon_ldpc_encode(NULL, info, parity));
    TEST_ASSERT_FALSE(poporon_ldpc_encode(ldpc, NULL, parity));
    TEST_ASSERT_FALSE(poporon_ldpc_encode(ldpc, info, NULL));

    TEST_ASSERT_FALSE(poporon_ldpc_decode_hard(NULL, codeword, 50, NULL));
    TEST_ASSERT_FALSE(poporon_ldpc_decode_hard(ldpc, NULL, 50, NULL));

    TEST_ASSERT_FALSE(poporon_ldpc_check(NULL, codeword));
    TEST_ASSERT_FALSE(poporon_ldpc_check(ldpc, NULL));

    TEST_ASSERT_EQUAL(0, poporon_ldpc_info_size(NULL));
    TEST_ASSERT_EQUAL(0, poporon_ldpc_codeword_size(NULL));
    TEST_ASSERT_EQUAL(0, poporon_ldpc_parity_size(NULL));

    poporon_ldpc_destroy(ldpc);
    poporon_ldpc_destroy(NULL);
}

static void test_ldpc_burst_resistant_config(void)
{
    poporon_ldpc_t *ldpc;
    poporon_ldpc_config_t config;
    uint32_t iterations;
    uint8_t info[128], parity[256], codeword[256];
    size_t i;

    TEST_ASSERT_TRUE(poporon_ldpc_config_burst_resistant(&config));
    TEST_ASSERT_EQUAL(PPRN_LDPC_RANDOM, config.matrix_type);
    TEST_ASSERT_TRUE(config.use_interleaver);
    TEST_ASSERT_EQUAL(7, config.column_weight);

    config.use_interleaver = false;

    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_1_2, &config);
    TEST_ASSERT_NOT_NULL(ldpc);

    for (i = 0; i < sizeof(info); i++) {
        info[i] = (uint8_t)(i * 17 + 5);
    }
    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, sizeof(info));
    memcpy(codeword + sizeof(info), parity, poporon_ldpc_parity_size(ldpc));

    TEST_ASSERT_TRUE(poporon_ldpc_check(ldpc, codeword));

    for (i = 40; i < 45; i++) {
        codeword[i] ^= 0xFF;
    }

    TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, codeword, 100, &iterations));
    TEST_ASSERT_TRUE(poporon_ldpc_check(ldpc, codeword));

    TEST_ASSERT_EQUAL_MEMORY(info, codeword, sizeof(info));

    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_burst_error_resistance(void)
{
    poporon_ldpc_t *ldpc_default, *ldpc_burst_resistant;
    poporon_ldpc_config_t default_config, burst_resistant_config;
    uint32_t iter_default, iter_burst_resistant;
    uint8_t info[256], parity_default[256], parity_burst_resistant[256], codeword_default[512],
        codeword_burst_resistant[512];
    int32_t default_success = 0, burst_resistant_success = 0, trials = 5;
    size_t i, burst_start, burst_len;

    TEST_ASSERT_TRUE(poporon_ldpc_config_default(&default_config));
    TEST_ASSERT_TRUE(poporon_ldpc_config_burst_resistant(&burst_resistant_config));

    burst_resistant_config.use_interleaver = false;

    ldpc_default = poporon_ldpc_create(256, PPRN_LDPC_RATE_1_2, &default_config);
    ldpc_burst_resistant = poporon_ldpc_create(256, PPRN_LDPC_RATE_1_2, &burst_resistant_config);
    TEST_ASSERT_NOT_NULL(ldpc_default);
    TEST_ASSERT_NOT_NULL(ldpc_burst_resistant);

    for (i = 0; i < sizeof(info); i++) {
        info[i] = (uint8_t)((i * 31) ^ 0xA5);
    }

    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc_default, info, parity_default));
    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc_burst_resistant, info, parity_burst_resistant));

    for (int32_t trial = 0; trial < trials; trial++) {
        burst_start = 32 + trial * 48;
        burst_len = 4;

        memcpy(codeword_default, info, sizeof(info));
        memcpy(codeword_default + sizeof(info), parity_default, poporon_ldpc_parity_size(ldpc_default));
        memcpy(codeword_burst_resistant, info, sizeof(info));
        memcpy(codeword_burst_resistant + sizeof(info), parity_burst_resistant,
               poporon_ldpc_parity_size(ldpc_burst_resistant));

        for (i = burst_start; i < burst_start + burst_len && i < sizeof(info); i++) {
            codeword_default[i] ^= 0xFF;
            codeword_burst_resistant[i] ^= 0xFF;
        }

        if (poporon_ldpc_decode_hard(ldpc_default, codeword_default, 100, &iter_default)) {
            if (memcmp(codeword_default, info, sizeof(info)) == 0) {
                default_success++;
            }
        }

        if (poporon_ldpc_decode_hard(ldpc_burst_resistant, codeword_burst_resistant, 100, &iter_burst_resistant)) {
            if (memcmp(codeword_burst_resistant, info, sizeof(info)) == 0) {
                burst_resistant_success++;
            }
        }
    }

    TEST_ASSERT_GREATER_THAN(0, default_success);
    TEST_ASSERT_GREATER_THAN(0, burst_resistant_success);

    poporon_ldpc_destroy(ldpc_default);
    poporon_ldpc_destroy(ldpc_burst_resistant);
}

static void test_ldpc_interleave_api(void)
{
    poporon_ldpc_t *ldpc;
    poporon_ldpc_config_t config;
    uint8_t info[128], parity[128], codeword[256], interleaved[256], deinterleaved[256];
    size_t i, codeword_size;

    TEST_ASSERT_TRUE(poporon_ldpc_config_burst_resistant(&config));
    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_1_2, &config);
    TEST_ASSERT_NOT_NULL(ldpc);

    TEST_ASSERT_TRUE(poporon_ldpc_has_interleaver(ldpc));

    codeword_size = poporon_ldpc_codeword_size(ldpc);

    for (i = 0; i < sizeof(info); i++) {
        info[i] = (uint8_t)(i * 17 + 5);
    }
    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, sizeof(info));
    memcpy(codeword + sizeof(info), parity, poporon_ldpc_parity_size(ldpc));

    TEST_ASSERT_TRUE(poporon_ldpc_interleave(ldpc, codeword, interleaved));

    TEST_ASSERT_FALSE(memcmp(codeword, interleaved, codeword_size) == 0);

    TEST_ASSERT_TRUE(poporon_ldpc_deinterleave(ldpc, interleaved, deinterleaved));

    TEST_ASSERT_EQUAL_MEMORY(codeword, deinterleaved, codeword_size);

    poporon_ldpc_destroy(ldpc);

    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_1_2, NULL);
    TEST_ASSERT_NOT_NULL(ldpc);
    TEST_ASSERT_FALSE(poporon_ldpc_has_interleaver(ldpc));

    TEST_ASSERT_TRUE(poporon_ldpc_interleave(ldpc, codeword, interleaved));
    TEST_ASSERT_EQUAL_MEMORY(codeword, interleaved, codeword_size);

    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_interleave_burst_correction(void)
{
    poporon_ldpc_t *ldpc;
    poporon_ldpc_config_t config;
    uint32_t iterations;
    uint8_t info[128], parity[128], codeword[256], interleaved[256], received[256];
    size_t i;

    TEST_ASSERT_TRUE(poporon_ldpc_config_burst_resistant(&config));
    TEST_ASSERT_TRUE(config.use_interleaver);

    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_1_2, &config);
    TEST_ASSERT_NOT_NULL(ldpc);
    TEST_ASSERT_TRUE(poporon_ldpc_has_interleaver(ldpc));

    for (i = 0; i < sizeof(info); i++) {
        info[i] = (uint8_t)(i * 17 + 5);
    }
    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, sizeof(info));
    memcpy(codeword + sizeof(info), parity, poporon_ldpc_parity_size(ldpc));

    TEST_ASSERT_TRUE(poporon_ldpc_check(ldpc, codeword));

    TEST_ASSERT_TRUE(poporon_ldpc_interleave(ldpc, codeword, interleaved));

    memcpy(received, interleaved, sizeof(received));
    for (i = 40; i < 44; i++) {
        received[i] ^= 0xFF;
    }

    TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, received, 100, &iterations));
    TEST_ASSERT_TRUE(poporon_ldpc_check(ldpc, received));

    TEST_ASSERT_EQUAL_MEMORY(info, received, sizeof(info));

    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_qc_peg_basic(void)
{
    poporon_ldpc_t *ldpc;
    poporon_ldpc_config_t config;
    uint32_t iterations;
    uint8_t info[128], parity[128], codeword[256];
    size_t i;

    TEST_ASSERT_TRUE(poporon_ldpc_config_default(&config));
    config.matrix_type = PPRN_LDPC_QC_PEG;

    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_1_2, &config);
    TEST_ASSERT_NOT_NULL(ldpc);

    for (i = 0; i < sizeof(info); i++) {
        info[i] = (uint8_t)(i * 17 + 5);
    }
    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, sizeof(info));
    memcpy(codeword + sizeof(info), parity, poporon_ldpc_parity_size(ldpc));

    TEST_ASSERT_TRUE(poporon_ldpc_check(ldpc, codeword));

    TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, codeword, 50, &iterations));
    TEST_ASSERT_EQUAL(0, iterations);
    TEST_ASSERT_EQUAL_MEMORY(info, codeword, sizeof(info));

    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_qc_peg_with_errors(void)
{
    poporon_ldpc_t *ldpc;
    poporon_ldpc_config_t config;
    uint32_t iterations;
    uint8_t info[128], parity[128], codeword[256], original[256];
    size_t i;

    TEST_ASSERT_TRUE(poporon_ldpc_config_default(&config));
    config.matrix_type = PPRN_LDPC_QC_PEG;

    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_1_2, &config);
    TEST_ASSERT_NOT_NULL(ldpc);

    for (i = 0; i < sizeof(info); i++) {
        info[i] = (uint8_t)(i * 31 + 7);
    }
    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, sizeof(info));
    memcpy(codeword + sizeof(info), parity, poporon_ldpc_parity_size(ldpc));
    memcpy(original, codeword, sizeof(codeword));

    codeword[0] ^= 0x01;
    codeword[20] ^= 0x80;
    codeword[50] ^= 0x40;

    TEST_ASSERT_FALSE(poporon_ldpc_check(ldpc, codeword));

    TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, codeword, 100, &iterations));
    TEST_ASSERT_GREATER_THAN(0, iterations);
    TEST_ASSERT_EQUAL_MEMORY(original, codeword, sizeof(codeword));

    poporon_ldpc_destroy(ldpc);
}

static void test_ldpc_qc_peg_various_rates(void)
{
    poporon_ldpc_rate_t rates[] = {PPRN_LDPC_RATE_1_2, PPRN_LDPC_RATE_2_3, PPRN_LDPC_RATE_3_4, PPRN_LDPC_RATE_5_6};
    poporon_ldpc_t *ldpc;
    poporon_ldpc_config_t config;
    uint8_t *info, *parity, *codeword;
    uint32_t iterations;
    size_t i, r, num_rates = sizeof(rates) / sizeof(rates[0]), info_size, parity_size, codeword_size;

    TEST_ASSERT_TRUE(poporon_ldpc_config_default(&config));
    config.matrix_type = PPRN_LDPC_QC_PEG;

    for (r = 0; r < num_rates; r++) {
        ldpc = poporon_ldpc_create(128, rates[r], &config);
        TEST_ASSERT_NOT_NULL(ldpc);

        info_size = poporon_ldpc_info_size(ldpc);
        parity_size = poporon_ldpc_parity_size(ldpc);
        codeword_size = poporon_ldpc_codeword_size(ldpc);

        info = (uint8_t *)malloc(info_size);
        parity = (uint8_t *)malloc(parity_size);
        codeword = (uint8_t *)malloc(codeword_size);

        for (i = 0; i < info_size; i++) {
            info[i] = (uint8_t)(i ^ r);
        }

        TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

        memcpy(codeword, info, info_size);
        memcpy(codeword + info_size, parity, parity_size);

        TEST_ASSERT_TRUE(poporon_ldpc_check(ldpc, codeword));
        TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, codeword, 50, &iterations));
        TEST_ASSERT_EQUAL_MEMORY(info, codeword, info_size);

        free(info);
        free(parity);
        free(codeword);
        poporon_ldpc_destroy(ldpc);
    }
}

static void test_ldpc_qc_peg_with_interleaver(void)
{
    poporon_ldpc_t *ldpc;
    poporon_ldpc_config_t config;
    uint32_t iterations;
    uint8_t info[128], parity[128], codeword[256], interleaved[256], received[256];
    size_t i;

    TEST_ASSERT_TRUE(poporon_ldpc_config_burst_resistant(&config));
    config.matrix_type = PPRN_LDPC_QC_PEG;

    ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_1_2, &config);
    TEST_ASSERT_NOT_NULL(ldpc);
    TEST_ASSERT_TRUE(poporon_ldpc_has_interleaver(ldpc));

    for (i = 0; i < sizeof(info); i++) {
        info[i] = (uint8_t)(i * 17 + 5);
    }
    TEST_ASSERT_TRUE(poporon_ldpc_encode(ldpc, info, parity));

    memcpy(codeword, info, sizeof(info));
    memcpy(codeword + sizeof(info), parity, poporon_ldpc_parity_size(ldpc));

    TEST_ASSERT_TRUE(poporon_ldpc_interleave(ldpc, codeword, interleaved));

    memcpy(received, interleaved, sizeof(received));
    for (i = 40; i < 44; i++) {
        received[i] ^= 0xFF;
    }

    TEST_ASSERT_TRUE(poporon_ldpc_decode_hard(ldpc, received, 100, &iterations));
    TEST_ASSERT_TRUE(poporon_ldpc_check(ldpc, received));
    TEST_ASSERT_EQUAL_MEMORY(info, received, sizeof(info));

    poporon_ldpc_destroy(ldpc);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ldpc_create_destroy);
    RUN_TEST(test_ldpc_create_invalid);
    RUN_TEST(test_ldpc_encode_basic);
    RUN_TEST(test_ldpc_decode_hard_no_errors);
    RUN_TEST(test_ldpc_decode_hard_with_errors);
    RUN_TEST(test_ldpc_decode_hard_block256_byte_errors);
    RUN_TEST(test_ldpc_decode_soft_basic);
    RUN_TEST(test_ldpc_decode_soft_with_noise);
    RUN_TEST(test_ldpc_various_rates);
    RUN_TEST(test_ldpc_null_params);
    RUN_TEST(test_ldpc_burst_resistant_config);
    RUN_TEST(test_ldpc_burst_error_resistance);
    RUN_TEST(test_ldpc_interleave_api);
    RUN_TEST(test_ldpc_interleave_burst_correction);
    RUN_TEST(test_ldpc_qc_peg_basic);
    RUN_TEST(test_ldpc_qc_peg_with_errors);
    RUN_TEST(test_ldpc_qc_peg_various_rates);
    RUN_TEST(test_ldpc_qc_peg_with_interleaver);

    return UNITY_END();
}
