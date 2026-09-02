// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef TIP_ALL_TESTS_H_
#define TIP_ALL_TESTS_H_

#include "testing.h"
#include "platform_all_tests.h"
#include "riot/riot_tip_all_tests.h"


/**
 * Add all tests for TIP components that are not directly associated with any hardware.
 *
 * Be sure to keep the test suites in alphabetical order for easier management.
 *
 * @param suite Suite to add the tests to.
 */
void add_all_tip_tests (CuSuite *suite)
{
	add_all_tip_riot_tests (suite);
}


#endif /* TIP_ALL_TESTS_H_ */
