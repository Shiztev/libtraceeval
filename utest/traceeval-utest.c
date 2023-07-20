// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023 Google Inc, Stevie Alvarez <stevie.6strings@gmail.com>
 */

#include <string.h>
#include <stdio.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <histograms.h>

#define TRACEEVAL_SUITE		"traceeval library"


/**
 * Test traceeval_init() and traceeval_release() with NULL values.
 */
void test_eval_null()
{
	// set up
	char *name = "test none";
	enum traceeval_data_type type = TRACEEVAL_TYPE_NONE;
	const struct traceeval_type test_data[] =  {
		{
			.type = type,
			.name = name
		}
	};

	// test init
	struct traceeval *result_null = traceeval_init(NULL, NULL);
	struct traceeval *result_key = traceeval_init(test_data, NULL);
	struct traceeval *result_val = traceeval_init(NULL, test_data);
	
	// analyze alloc
	CU_ASSERT(!result_null);
	CU_ASSERT(!result_key);
	CU_ASSERT(!result_val);

	// test release
	int release_result = traceeval_release(NULL);
	
	// analyze release
	CU_ASSERT(release_result == -1);
}

/**
 * Tests the traceeval_init() and traceeval_release() functions.
 */
void test_eval(void)
{
	test_eval_null();
}


/**
 * Register tests with CUnit.
 */
void test_traceeval_lib(void)
{
	CU_pSuite suite = NULL;
	
	// set up suite
	suite = CU_add_suite(TRACEEVAL_SUITE, NULL, NULL);
	if (suite == NULL) {
		fprintf(stderr, "Suite %s cannot be created\n", TRACEEVAL_SUITE);
		return;
	}

	// add tests to suite
	CU_add_test(suite, "Test traceeval alloc and release", test_eval);
}
