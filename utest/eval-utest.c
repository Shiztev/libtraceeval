
// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023 Google Inc, Stevie Alvarez <stevie.6strings@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "eval-test.h"

int main(int argc, char **argv)
{
	if (CU_initialize_registry() != CUE_SUCCESS) {
		printf("Test registry cannot be initialized\n");
		return EXIT_FAILURE;
	}

	test_traceeval_lib();

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return EXIT_SUCCESS;
}
