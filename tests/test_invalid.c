/*
 * libpoporon test - test_invalid.c
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
#include <poporon/gf.h>

#include "util.h"

#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_poporon(void)
{
    TEST_ASSERT_NULL(poporon_create(0, 0, 0, 0, 0));
    poporon_destroy(NULL);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_poporon);
    
    return UNITY_END();
}
