/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdio.h>

#include "CuTest/CuTest.h"


CuSuite *get_composite_eat_tip_main_token_generator_suite (void);

int main (void)
{
	CuString *output = CuStringNew ();
	CuSuite *suite = CuSuiteNew ();
	int failures;

	CuSuiteAddSuite (suite, get_composite_eat_tip_main_token_generator_suite ());
	CuSuiteRun (suite);
	CuSuiteSummary (suite, output);
	CuSuiteDetails (suite, output);
	printf ("%s\n", output->buffer);
	failures = suite->failCount;
	CuStringDelete (output);
	CuSuiteDelete (suite);
	return failures;
}
