/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include "testing.h"
#include "composite_eat/bmc_direct/bmc_direct_composite_eat_status.h"


TEST_SUITE_LABEL ("bmc_direct_composite_eat_status");


static void bmc_direct_composite_eat_status_test_mapping (CuTest *test)
{
	TEST_START;
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_OK,
		bmc_direct_composite_eat_map_main_token_status (COMPOSITE_EAT_TIP_MAIN_TOKEN_OK));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_BAD_VERSION,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_VERSION));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_BAD_REQUEST,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_REQUEST));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_TOO_MANY_RECORDS,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_TOO_MANY_RECORDS));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_RESPONSE_TOO_SMALL,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_BUFFER_TOO_SMALL));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_RESPONSE_TOO_SMALL,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_RESPONSE_TOO_LARGE));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_SIGN_FAILED,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_SIGN_ERROR));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_BUSY,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_BUSY));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_INTERNAL,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_CANCELED));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_INTERNAL,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_IDENTITY_ERROR));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_INTERNAL,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_ARGUMENT));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_INTERNAL,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_EVIDENCE_ERROR));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_INTERNAL,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_HASH_ERROR));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_INTERNAL,
		bmc_direct_composite_eat_map_main_token_status (
			COMPOSITE_EAT_TIP_MAIN_TOKEN_ENCODING_ERROR));
}


TEST_SUITE_START (bmc_direct_composite_eat_status);

TEST (bmc_direct_composite_eat_status_test_mapping);

TEST_SUITE_END;
