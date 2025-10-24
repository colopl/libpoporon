/*
 * libpoporon test - test_gf.c
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

#include <poporon.h>

#include "../src/poporon_internal.h"

#include "util.h"

#include "unity.h"

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
