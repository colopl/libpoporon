/*
 * libpoporon - test_invalid.c
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

void setUp(void)
{
}

void tearDown(void)
{
}

void test_poporon(void)
{
    TEST_ASSERT_NULL(poporon_create(NULL));
    poporon_destroy(NULL);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_poporon);

    return UNITY_END();
}
