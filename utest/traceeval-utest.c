/* SPDX-License-Identifier: MIT */
/*
 * Copyright (C) 2023 Google Inc, Stevie Alvarez <stevie.6strings@gmail.com>
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <traceeval-hist.h>
#include <traceeval-test.h>

#define TRACEEVAL_SUITE		"traceeval library"
#define TRACEEVAL_SUCCESS	0
#define TRACEEVAL_FAILURE	-1
#define TRACEEVAL_NOT_SAME	1

/*
 * Test traceeval_init(), traceeval_release(), and traceeval_compare() with
 * NULL values.
 */
static void test_eval_null(void)
{
	/* set up */
	const struct traceeval_type test_data[] =  {
		{
			.type = TRACEEVAL_TYPE_NUMBER,
			.name = "test null"
		},
		{
			.type = TRACEEVAL_TYPE_NONE,
			.name = NULL
		}
	};

	/* test init */
	struct traceeval *result_null = traceeval_init(NULL, NULL);
	struct traceeval *result_key = traceeval_init(test_data, NULL);
	struct traceeval *result_val = traceeval_init(NULL, test_data);

	/* analyze init */
	CU_ASSERT(!result_null);
	CU_ASSERT(result_key != NULL);
	CU_ASSERT(!result_val);

	/* release */
	traceeval_release(NULL);
	traceeval_release(result_key);
}

/*
 * Use provided data to test traceeval_init(), traceeval_compare(), and
 * traceeval_release().
 */
static void test_eval_base(const struct traceeval_type *keys1,
		const struct traceeval_type *vals1,
		const struct traceeval_type *keys2,
		const struct traceeval_type *vals2,
		bool init_not_null1, bool init_not_null2,
		int compare_result)
{
	struct traceeval *init1;
	struct traceeval *init2;
	int result;

	/* test init */
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

	/* test compare */
	result = traceeval_compare(init1, init2);

	/* analyze compare */
	CU_ASSERT(result == compare_result);

	/* release */
	traceeval_release(init1);
	traceeval_release(init2);
}

/*
 * Test traceeval_init(), traceeval_release(), and traceeval_compare() with
 * TRACEEVAL_TYPE_NONE.
 */
static void test_eval_none(void)
{
	/* set up */
	const struct traceeval_type test_data_none[] =  {
		{
			.type = TRACEEVAL_TYPE_NONE,
			.name = "test none"
		}
	};
	const struct traceeval_type test_data_some[] =  {
		{
			.type = TRACEEVAL_TYPE_NUMBER,
			.name = "test none (some)"
		},
		{
			.type = TRACEEVAL_TYPE_NONE,
			.name = NULL
		}
	};

	test_eval_base(test_data_some, test_data_none, test_data_some,
			test_data_none, true, true, TRACEEVAL_SUCCESS);
	test_eval_base(test_data_none, test_data_none, test_data_some,
			test_data_none, false, true, TRACEEVAL_FAILURE);
	test_eval_base(test_data_none, test_data_none, test_data_none,
			test_data_none, false, false, TRACEEVAL_FAILURE);
}

/*
 * Test traceeval_init() and traceeval_release() with equivalent values.
 */
static void test_eval_same(void)
{
	/* set up */
	const struct traceeval_type test_data[] =  {
		{
			.type = TRACEEVAL_TYPE_NUMBER,
			.name = "test data"
		},
		{
			.type = TRACEEVAL_TYPE_NONE,
			.name = NULL
		}
	};

	test_eval_base(test_data, test_data, test_data, test_data, true, true,
			TRACEEVAL_SUCCESS);
}

/*
 * Test traceeval_init() and traceeval_release() with non-equivalent values.
 */
static void test_eval_not_same(void)
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

	/* type 1 key diff */
	test_eval_base(test_data2, test_data1, test_data1, test_data1, true,
			true, TRACEEVAL_NOT_SAME);
	/* type 1 data diff */
	test_eval_base(test_data1, test_data2, test_data1, test_data1, true,
			true, TRACEEVAL_NOT_SAME);
	/* type 2 key diff */
	test_eval_base(test_data1, test_data1, test_data2, test_data1, true,
			true, TRACEEVAL_NOT_SAME);
	/* type 2 data diff */
	test_eval_base(test_data1, test_data1, test_data1, test_data2, true,
			true, TRACEEVAL_NOT_SAME);
}

/*
 * Tests the traceeval_init() and traceeval_release() functions.
 */
static void test_eval(void)
{
	test_eval_null();
	test_eval_none();
	test_eval_same();
	test_eval_not_same();
}

/*
 * Register tests with CUnit.
 */
void test_traceeval_lib(void)
{
	CU_pSuite suite = NULL;

	/* set up suite */
	suite = CU_add_suite(TRACEEVAL_SUITE, NULL, NULL);
	if (suite == NULL) {
		fprintf(stderr, "Suite %s cannot be created\n", TRACEEVAL_SUITE);
		return;
	}

	/* add tests to suite */
	CU_add_test(suite, "Test traceeval alloc and release", test_eval);
}
