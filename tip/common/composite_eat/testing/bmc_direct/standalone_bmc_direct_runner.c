/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdio.h>

#include "CuTest/CuTest.h"


CuSuite *get_bmc_direct_composite_eat_status_suite (void);
CuSuite *get_bmc_direct_composite_eat_transport_suite (void);

int main (void)
{
	CuString *output = CuStringNew ();
	CuSuite *suite = CuSuiteNew ();
	int failures;

	CuSuiteAddSuite (suite, get_bmc_direct_composite_eat_status_suite ());
	CuSuiteAddSuite (suite, get_bmc_direct_composite_eat_transport_suite ());
	CuSuiteRun (suite);
	CuSuiteSummary (suite, output);
	CuSuiteDetails (suite, output);
	printf ("%s\n", output->buffer);
	failures = suite->failCount;
	CuStringDelete (output);
	CuSuiteDelete (suite);
	return failures;
}
