// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023 Google Inc, Stevie Alvarez <stevie.6strings@gmail.com>
 */

#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <histograms.h>

#define TRACEEVAL_SUITE		"traceeval library"


void test_type_alloc_type_none(void)
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

	// test
	const struct traceeval_type *result = type_alloc(test_data);

	// analyze
	CU_ASSERT(result->type == test_data->type);
	CU_ASSERT(strcmp(result->name, test_data->name) == 0);
	CU_ASSERT(result->name != test_data->name);
	CU_ASSERT(result->flags == test_data->flags);
	CU_ASSERT(result->dyn_release == test_data->dyn_release);
}

/**
 * Tests the type_alloc() and type_release() functions.
 */
void test_type(void)
{
	test_type_alloc_type_none();
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
	CU_add_test(suite, "Test traceeval_type alloc and release", test_type);
}
