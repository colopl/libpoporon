/*
 * libpoporon - test_basic.c
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

#include "../src/internal/common.h"
#include "unity.h"
#include "util.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_version_id(void)
{
    TEST_ASSERT_EQUAL_UINT32(POPORON_VERSION_ID, poporon_version_id());
}

void test_buildtime(void)
{
    TEST_ASSERT_GREATER_THAN(0, poporon_buildtime());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_version_id);
    RUN_TEST(test_buildtime);

    return UNITY_END();
}
