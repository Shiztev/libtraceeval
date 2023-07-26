// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023 Google Inc, Stevie Alvarez <stevie.6strings@gmail.com>
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <histograms.h>

#define TRACEEVAL_SUITE		"traceeval library"
#define TRACEEVAL_SUCCESS	0
#define TRACEEVAL_FAILURE	-1
#define TRACEEVAL_NOT_SAME	1


/**
 * Test traceeval_init(), traceeval_release(), and traceeval_compare() with
 * NULL values.
 */
void test_eval_null(void)
{
	// set up
	char *name = "test null";
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
	
	// analyze init
	CU_ASSERT(!result_null);
	CU_ASSERT(!result_key);
	CU_ASSERT(!result_val);

	// test release
	int release_result = traceeval_release(NULL);
	
	// analyze release
	CU_ASSERT(release_result == -1);
}

/**
 * Use provided data to test traceeval_init(), traceeval_compare(), and
 * traceeval_release().
 */
void test_eval_base(const struct traceeval_type *keys1,
		const struct traceeval_type *vals1,
		const struct traceeval_type *keys2,
		const struct traceeval_type *vals2,
		bool init_not_null1, bool init_not_null2,
		int compare_result, int release_result1, int release_result2)
{
	struct traceeval *init1;
	struct traceeval *init2;
	int result;

	// test init
	init1 = traceeval_init(keys1, vals1);
	init2 = traceeval_init(keys2, vals2);

	result = init1 != NULL;
	if (init_not_null1) {
		CU_ASSERT(result);
	} else {
		CU_ASSERT(!result);
	}

	result = init2 != NULL;
	if (init_not_null2) {
		CU_ASSERT(result);
	} else {
		CU_ASSERT(!result);
	}

	// test compare
	result = traceeval_compare(init1, init2);
	
	// analyze compare
	CU_ASSERT(result == compare_result);
	
	// test and analyze release
	CU_ASSERT(traceeval_release(init1) == release_result1);
	CU_ASSERT(traceeval_release(init2) == release_result2);
}

/**
 * Test traceeval_init(), traceeval_release(), and traceeval_compare() with
 * TRACEEVAL_TYPE_NONE.
 */
void test_eval_none(void)
{
	// set up
	char *name = "test none";
	char *name2 = "test none (some)";
	const struct traceeval_type test_data_none[] =  {
		{
			.type = TRACEEVAL_TYPE_NONE,
			.name = name
		}
	};
	const struct traceeval_type test_data_some[] =  {
		{
			.type = TRACEEVAL_TYPE_NUMBER,
			.name = name2
		},
		{
			.type = TRACEEVAL_TYPE_NONE,
			.name = NULL
		}
	};

	test_eval_base(test_data_some, test_data_none, test_data_some,
			test_data_none, true, true, TRACEEVAL_SUCCESS,
			TRACEEVAL_SUCCESS, TRACEEVAL_SUCCESS);
	test_eval_base(test_data_none, test_data_none, test_data_some,
			test_data_none, false, true, TRACEEVAL_FAILURE,
			TRACEEVAL_FAILURE, TRACEEVAL_SUCCESS);
	test_eval_base(test_data_none, test_data_none, test_data_none,
			test_data_none, false, false, TRACEEVAL_FAILURE,
			TRACEEVAL_FAILURE, TRACEEVAL_FAILURE);
}

/**
 * Test traceeval_init() and traceeval_release() with equivalent values.
 */
void test_eval_same(void)
{
	// set up
	char *name = "test data";
	char *name2 = "test done";
	const struct traceeval_type test_data[] =  {
		{
			.type = TRACEEVAL_TYPE_NUMBER,
			.name = name
		},
		{
			.type = TRACEEVAL_TYPE_NONE,
			.name = name2
		}
	};
	
	test_eval_base(test_data, test_data, test_data, test_data, true, true,
			TRACEEVAL_SUCCESS, TRACEEVAL_SUCCESS,
			TRACEEVAL_SUCCESS);
}

/**
 * Test traceeval_init() and traceeval_release() with non-equivalent values.
 */
void test_eval_not_same(void)
{
	const struct traceeval_type test_data1[] =  {
		{
			.type = TRACEEVAL_TYPE_NUMBER,
			.name = "test data 1"
		},
		{
			.type = TRACEEVAL_TYPE_NONE,
			.name = NULL
		}
	};
	const struct traceeval_type test_data2[] =  {
		{
			.type = TRACEEVAL_TYPE_NUMBER,
			.name = "test data 2"
		},
		{
			.type = TRACEEVAL_TYPE_NONE,
			.name = NULL
		}
	};

	// type 1 key diff
	test_eval_base(test_data2, test_data1, test_data1, test_data1, true,
			true, TRACEEVAL_NOT_SAME, TRACEEVAL_SUCCESS,
			TRACEEVAL_SUCCESS);
	// type 1 data diff
	test_eval_base(test_data1, test_data2, test_data1, test_data1, true,
			true, TRACEEVAL_NOT_SAME, TRACEEVAL_SUCCESS,
			TRACEEVAL_SUCCESS);
	// type 2 key diff
	test_eval_base(test_data1, test_data1, test_data2, test_data1, true,
			true, TRACEEVAL_NOT_SAME, TRACEEVAL_SUCCESS,
			TRACEEVAL_SUCCESS);
	// type 2 data diff
	test_eval_base(test_data1, test_data1, test_data1, test_data2, true,
			true, TRACEEVAL_NOT_SAME, TRACEEVAL_SUCCESS,
			TRACEEVAL_SUCCESS);
}

/**
 * Tests the traceeval_init() and traceeval_release() functions.
 */
void test_eval(void)
{
	test_eval_null();
	test_eval_none();
	test_eval_same();
	test_eval_not_same();
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
