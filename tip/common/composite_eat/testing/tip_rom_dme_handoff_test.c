/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "testing.h"
#include "tip_rom_dme_handoff.h"
#include "tip_utils.h"


TEST_SUITE_LABEL ("tip_rom_dme_handoff");

static void composite_eat_tip_rom_evidence_test_a1 (CuTest *test)
{
	SEC_DME_DICE_T_A1 handoff;
	struct tip_rom_dme_public_evidence evidence;
	enum tip_rom_dme_handoff_status status;

	TEST_START;
	memset (&handoff, 0, sizeof (handoff));
	memset (handoff.dme_pcr0, 0x51, sizeof (handoff.dme_pcr0));
	status = tip_rom_dme_handoff_parse (ARBEL_VERSION_A1, &handoff, &evidence);
	CuAssertIntEquals (test, TIP_ROM_DME_HANDOFF_OK, status);
	CuAssertPtrEquals (test, handoff.dme_pcr0, (void *) evidence.dme_pcr0.data);
	CuAssertIntEquals (test, SEC_PCR0_LENGTH, evidence.dme_pcr0.length);
	CuAssertPtrEquals (test, NULL, (void *) evidence.dme_challenge.data);
	CuAssertIntEquals (test, 0, evidence.dme_challenge.length);
	CuAssertIntEquals (test, 0x51, evidence.dme_pcr0.data[0]);
}

static void composite_eat_tip_rom_evidence_test_a2_a3 (CuTest *test)
{
	SEC_DME_DICE_T_A2 handoff;
	struct tip_rom_dme_public_evidence evidence;
	enum tip_rom_dme_handoff_status status;

	TEST_START;
	memset (&handoff, 0, sizeof (handoff));
	memset (handoff.dme_challenge, 0x43, sizeof (handoff.dme_challenge));
	memset (handoff.dme_pcr0, 0x52, sizeof (handoff.dme_pcr0));
	status = tip_rom_dme_handoff_parse (ARBEL_VERSION_A2, &handoff, &evidence);
	CuAssertIntEquals (test, TIP_ROM_DME_HANDOFF_OK, status);
	CuAssertPtrEquals (test, handoff.dme_challenge, (void *) evidence.dme_challenge.data);
	CuAssertIntEquals (test, sizeof (handoff.dme_challenge), evidence.dme_challenge.length);
	CuAssertPtrEquals (test, handoff.dme_pcr0, (void *) evidence.dme_pcr0.data);
	CuAssertIntEquals (test, 0x52, evidence.dme_pcr0.data[0]);

	status = tip_rom_dme_handoff_parse (ARBEL_VERSION_A3, &handoff, &evidence);
	CuAssertIntEquals (test, TIP_ROM_DME_HANDOFF_OK, status);
	CuAssertPtrEquals (test, handoff.dme_pcr0, (void *) evidence.dme_pcr0.data);
}

static void composite_eat_tip_rom_evidence_test_invalid (CuTest *test)
{
	SEC_DME_DICE_T_A2 handoff;
	struct tip_rom_dme_public_evidence evidence;
	enum tip_rom_dme_handoff_status status;

	TEST_START;
	memset (&handoff, 0, sizeof (handoff));
	memset (&evidence, 0xa5, sizeof (evidence));
	status = tip_rom_dme_handoff_parse (0, &handoff, &evidence);
	CuAssertIntEquals (test, TIP_ROM_DME_HANDOFF_UNSUPPORTED_REVISION, status);
	CuAssertPtrEquals (test, NULL, (void *) evidence.dme_pcr0.data);
	CuAssertIntEquals (test, TIP_ROM_DME_HANDOFF_BAD_ARGUMENT,
		tip_rom_dme_handoff_parse (ARBEL_VERSION_A2, NULL, &evidence));
	CuAssertIntEquals (test, TIP_ROM_DME_HANDOFF_BAD_ARGUMENT,
		tip_rom_dme_handoff_parse (ARBEL_VERSION_A2, &handoff, NULL));
}


TEST_SUITE_START (tip_rom_dme_handoff);

TEST (composite_eat_tip_rom_evidence_test_a1);
TEST (composite_eat_tip_rom_evidence_test_a2_a3);
TEST (composite_eat_tip_rom_evidence_test_invalid);

TEST_SUITE_END;
