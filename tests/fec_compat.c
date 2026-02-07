/*
 * libpoporon - fec_compat.c
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

#include <fec.h>

#include "unity.h"
#include "util.h"

#define SYMBOL_SIZE            8
#define GENERATER_POLYNOMIAL   0x11D
#define FIRST_CONSECUTIVE_ROOT 1
#define PRIMITIVE_ELEMENT      1
#define NUMBER_OF_ROOTS        127

#define FEC_N ((1 << SYMBOL_SIZE) - 1)
#define FEC_K (FEC_N - NUMBER_OF_ROOTS)

void setUp(void)
{
}

void tearDown(void)
{
}

void test_fec(void)
{
    void *rs;
    int i, error_locations[NUMBER_OF_ROOTS] = {0};
    unsigned char *data, *broken_data, *parity, *codeword;

    rs = init_rs_char(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS, 0);
    TEST_ASSERT_NOT_NULL(rs);

    data = (unsigned char *)malloc(FEC_K * sizeof(unsigned char));
    TEST_ASSERT_NOT_NULL(data);

    broken_data = (unsigned char *)malloc(FEC_K * sizeof(unsigned char));
    TEST_ASSERT_NOT_NULL(broken_data);

    parity = (unsigned char *)malloc(NUMBER_OF_ROOTS * sizeof(unsigned char));
    TEST_ASSERT_NOT_NULL(parity);

    codeword = (unsigned char *)malloc(FEC_N * sizeof(unsigned char));
    TEST_ASSERT_NOT_NULL(codeword);

    random_data(data, FEC_K);

    encode_rs_char(rs, data, parity);

    memcpy(codeword, data, FEC_K);
    memcpy(codeword + FEC_K, parity, NUMBER_OF_ROOTS);

    TEST_ASSERT_EQUAL_INT(0, decode_rs_char(rs, codeword, error_locations, 0));

    for (i = 1; i <= (NUMBER_OF_ROOTS / 2); i++) {
        memcpy(broken_data, data, FEC_K);

        break_data(broken_data, FEC_K, i);

        memcpy(codeword, broken_data, FEC_K);
        memcpy(codeword + FEC_K, parity, NUMBER_OF_ROOTS);

        TEST_ASSERT_EQUAL_INT(i, decode_rs_char(rs, codeword, error_locations, 0));
    }

    free(data);
    free(broken_data);
    free(parity);
    free(codeword);
    free_rs_char(rs);
}

void test_compat(void)
{
    poporon_t *pprn;
    uint8_t *data_pprn, *broken_data_pprn, *parity_pprn;
    size_t corrected_num;
    void *fec;
    int i, error_locations[NUMBER_OF_ROOTS] = {0};
    unsigned char *data_fec, *broken_data_fec, *parity_fec, *codeword;

    fec =
        init_rs_char(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS, 0);
    TEST_ASSERT_NOT_NULL(fec);
    data_fec = (unsigned char *)malloc(FEC_K * sizeof(unsigned char));
    TEST_ASSERT_NOT_NULL(data_fec);
    broken_data_fec = (unsigned char *)malloc(FEC_K * sizeof(unsigned char));
    TEST_ASSERT_NOT_NULL(broken_data_fec);
    parity_fec = (unsigned char *)malloc(NUMBER_OF_ROOTS * sizeof(unsigned char));
    TEST_ASSERT_NOT_NULL(parity_fec);
    codeword = (unsigned char *)malloc(FEC_N * sizeof(unsigned char));
    TEST_ASSERT_NOT_NULL(codeword);

    pprn =
        poporon_create(SYMBOL_SIZE, GENERATER_POLYNOMIAL, FIRST_CONSECUTIVE_ROOT, PRIMITIVE_ELEMENT, NUMBER_OF_ROOTS);
    TEST_ASSERT_NOT_NULL(pprn);
    data_pprn = (uint8_t *)malloc(FEC_K * sizeof(uint8_t));
    TEST_ASSERT_NOT_NULL(data_pprn);
    broken_data_pprn = (uint8_t *)malloc(FEC_K * sizeof(uint8_t));
    TEST_ASSERT_NOT_NULL(broken_data_pprn);
    parity_pprn = (uint8_t *)malloc(NUMBER_OF_ROOTS * sizeof(uint8_t));
    TEST_ASSERT_NOT_NULL(parity_pprn);

    random_data(data_fec, FEC_K);
    memcpy(data_pprn, data_fec, FEC_K);

    encode_rs_char(fec, data_fec, parity_fec);
    poporon_encode_u8(pprn, data_pprn, FEC_K, parity_pprn);
    TEST_ASSERT_EQUAL_MEMORY(parity_fec, parity_pprn, NUMBER_OF_ROOTS);

    memcpy(codeword, data_fec, FEC_K);
    memcpy(codeword + FEC_K, parity_fec, NUMBER_OF_ROOTS);
    TEST_ASSERT_EQUAL_INT(0, decode_rs_char(fec, codeword, error_locations, 0));

    TEST_ASSERT_TRUE(poporon_decode_u8(pprn, data_pprn, FEC_K, parity_pprn, &corrected_num));
    TEST_ASSERT_EQUAL_size_t(0, corrected_num);

    for (i = 1; i <= (NUMBER_OF_ROOTS / 2); i++) {
        memcpy(broken_data_fec, data_fec, FEC_K);

        break_data(broken_data_fec, FEC_K, i);
        memcpy(broken_data_pprn, broken_data_fec, FEC_K);

        memcpy(codeword, broken_data_fec, FEC_K);
        memcpy(codeword + FEC_K, parity_fec, NUMBER_OF_ROOTS);

        TEST_ASSERT_EQUAL_INT(i, decode_rs_char(fec, codeword, error_locations, 0));
        TEST_ASSERT_TRUE(poporon_decode_u8(pprn, broken_data_pprn, FEC_K, parity_pprn, &corrected_num));
        TEST_ASSERT_EQUAL_size_t(i, corrected_num);

        TEST_ASSERT_EQUAL_MEMORY(codeword, broken_data_pprn, FEC_K);
    }

    poporon_destroy(pprn);
    free(data_pprn);
    free(broken_data_pprn);
    free(parity_pprn);

    free_rs_char(fec);
    free(data_fec);
    free(broken_data_fec);
    free(parity_fec);
    free(codeword);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_fec);
    RUN_TEST(test_compat);

    return UNITY_END();
}
