/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdio.h>
#include <string.h>

#include "composite_eat/tip_evidence_adapter.h"
#include "tip_rom_dme_handoff.h"
#include "tip_utils.h"


#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf (stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
			return 1; \
		} \
	} while (0)

static int snapshot_active;
static int dme_failure;

static int test_get_pcr_count (struct pcr_store *store)
{
	(void) store;
	return 2;
}

static int test_get_pcr_digest_length (struct pcr_store *store, uint8_t pcr)
{
	(void) store;
	return (pcr == 0) ? TCG_CONCISE_EVIDENCE_SHA256_LENGTH :
		TCG_CONCISE_EVIDENCE_SHA384_LENGTH;
}

static int test_begin_snapshot (struct pcr_store *store)
{
	(void) store;
	snapshot_active = 1;
	return 0;
}

static int test_end_snapshot (struct pcr_store *store)
{
	(void) store;
	snapshot_active = 0;
	return 0;
}

static int test_compute_pcr (struct pcr_store *store, const struct hash_engine *hash,
	uint8_t pcr, uint8_t *measurement, size_t length)
{
	(void) store;
	(void) hash;
	if (!snapshot_active) {
		return -1;
	}
	memset (measurement, 0x40 + pcr, length);
	return (int) length;
}

static int test_get_dme_pcr0 (uint8_t *measurement, size_t length)
{
	if (dme_failure || (length != TCG_CONCISE_EVIDENCE_SHA512_LENGTH)) {
		return -1;
	}
	memset (measurement, 0x5a, length);
	return (int) length;
}

static const struct composite_eat_tip_evidence_ops test_ops = {
	.get_pcr_count = test_get_pcr_count,
	.get_pcr_digest_length = test_get_pcr_digest_length,
	.begin_snapshot = test_begin_snapshot,
	.end_snapshot = test_end_snapshot,
	.compute_pcr = test_compute_pcr,
	.get_dme_pcr0 = test_get_dme_pcr0,
};

static struct tcg_concise_evidence_environment test_environment (const uint8_t *class_id,
	size_t class_id_length, const uint8_t *model, size_t model_length)
{
	static const uint8_t vendor[] = "Nuvoton";
	struct tcg_concise_evidence_environment environment = {
		.has_class = true,
		.class_info = {
			.has_class_id = true,
			.class_id = {TCG_CONCISE_EVIDENCE_TAGGED_BYTES,
				{class_id, class_id_length}},
			.has_vendor = true,
			.vendor = {vendor, sizeof (vendor) - 1},
			.has_model = true,
			.model = {model, model_length},
		},
	};

	return environment;
}

static int test_handoff_parser (void)
{
	SEC_DME_DICE_T_A1 a1;
	SEC_DME_DICE_T_A2 a2;
	struct tip_rom_dme_public_evidence evidence;

	memset (&a1, 0, sizeof (a1));
	memset (&a2, 0, sizeof (a2));
	a1.dme_pcr0[0] = 0xa1;
	a2.dme_pcr0[0] = 0xa2;
	a2.dme_challenge[0] = 0xc2;
	CHECK (tip_rom_dme_handoff_parse (ARBEL_VERSION_A1, &a1, &evidence) ==
		TIP_ROM_DME_HANDOFF_OK);
	CHECK (evidence.dme_pcr0.data == a1.dme_pcr0);
	CHECK (evidence.dme_pcr0.length == SEC_PCR0_LENGTH);
	CHECK (evidence.dme_challenge.length == 0);
	CHECK (tip_rom_dme_handoff_parse (ARBEL_VERSION_A2, &a2, &evidence) ==
		TIP_ROM_DME_HANDOFF_OK);
	CHECK (evidence.dme_pcr0.data == a2.dme_pcr0);
	CHECK (evidence.dme_challenge.data == a2.dme_challenge);
	CHECK (tip_rom_dme_handoff_parse (ARBEL_VERSION_A3, &a2, &evidence) ==
		TIP_ROM_DME_HANDOFF_OK);
	CHECK (tip_rom_dme_handoff_parse (ARBEL_VERSION_Z1, &a2, &evidence) ==
		TIP_ROM_DME_HANDOFF_UNSUPPORTED_REVISION);
	CHECK (evidence.dme_pcr0.data == NULL);
	return 0;
}

static int test_adapter (void)
{
	static const uint8_t tip_class[] = "npcm850-tip";
	static const uint8_t tip_model[] = "NPCM850 TIP";
	static const uint8_t dme_class[] = "npcm850-rom-dme";
	static const uint8_t dme_model[] = "NPCM850 ROM DME";
	struct tcg_concise_evidence_environment tip_environment = test_environment (tip_class,
		sizeof (tip_class) - 1, tip_model, sizeof (tip_model) - 1);
	struct tcg_concise_evidence_environment dme_environment = test_environment (dme_class,
		sizeof (dme_class) - 1, dme_model, sizeof (dme_model) - 1);
	struct tcg_concise_evidence_environment invalid_environment = tip_environment;
	struct composite_eat_tip_evidence_adapter adapter;
	struct composite_eat_tip_evidence_workspace workspace;
	struct composite_eat_tip_local_evidence evidence;
	struct pcr_store store;
	struct hash_engine hash;

	memset (&store, 0, sizeof (store));
	memset (&hash, 0, sizeof (hash));
	invalid_environment.class_info.has_vendor = false;
	CHECK (composite_eat_tip_evidence_adapter_init_with_ops (&adapter, &store, &hash,
		&invalid_environment, &test_ops) == COMPOSITE_EAT_TIP_EVIDENCE_BAD_ARGUMENT);
	CHECK (composite_eat_tip_evidence_adapter_init_with_ops (&adapter, &store, &hash,
		&tip_environment, &test_ops) == COMPOSITE_EAT_TIP_EVIDENCE_OK);
	invalid_environment = dme_environment;
	invalid_environment.class_info.has_model = false;
	CHECK (composite_eat_tip_evidence_enable_dme (&adapter, &invalid_environment) ==
		COMPOSITE_EAT_TIP_EVIDENCE_BAD_ARGUMENT);
	CHECK (composite_eat_tip_evidence_enable_dme (&adapter, &dme_environment) ==
		COMPOSITE_EAT_TIP_EVIDENCE_OK);
	CHECK (composite_eat_tip_evidence_capture (&adapter, &workspace, &evidence) ==
		COMPOSITE_EAT_TIP_EVIDENCE_OK);
	CHECK (workspace.document.evidence_triple_count == 2);
	CHECK (workspace.dme_digest.algorithm.type == TCG_CONCISE_EVIDENCE_ALGORITHM_SHA512);
	CHECK (workspace.dme_digest.value.length == TCG_CONCISE_EVIDENCE_SHA512_LENGTH);
	CHECK (workspace.dme_pcr0[0] == 0x5a);
	CHECK (workspace.dme_measurement.key.type ==
		TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_TEXT);
	CHECK (workspace.dme_measurement.values.flags.is_immutable);
	CHECK (workspace.dme_measurement.values.flags.is_tcb);
	CHECK (evidence.document == &workspace.document);
	CHECK (evidence.encoded_length > 0);

	dme_failure = 1;
	CHECK (composite_eat_tip_evidence_capture (&adapter, &workspace, &evidence) ==
		COMPOSITE_EAT_TIP_EVIDENCE_SNAPSHOT_ERROR);
	CHECK (evidence.document == NULL);
	CHECK (evidence.encoded_length == 0);
	return 0;
}

int main (void)
{
	CHECK (test_handoff_parser () == 0);
	CHECK (test_adapter () == 0);
	return 0;
}
