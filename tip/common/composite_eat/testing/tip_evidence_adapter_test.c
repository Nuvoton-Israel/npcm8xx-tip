/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "testing.h"
#include "composite_eat/tip_evidence_adapter.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_spiffy_decode.h"


TEST_SUITE_LABEL ("composite_eat_tip_evidence_adapter");

static int test_pcr_count;
static int test_digest_lengths[COMPOSITE_EAT_TIP_EVIDENCE_MAX_PCRS];
static int test_compute_failure;
static int test_snapshot_active;
static int test_snapshot_begin_count;
static int test_snapshot_end_count;
static int test_dme_failure;


static int composite_eat_tip_evidence_test_get_count (struct pcr_store *store)
{
	(void) store;
	return test_pcr_count;
}

static int composite_eat_tip_evidence_test_get_length (struct pcr_store *store, uint8_t pcr)
{
	(void) store;
	return test_digest_lengths[pcr];
}

static int composite_eat_tip_evidence_test_compute (struct pcr_store *store,
	const struct hash_engine *hash, uint8_t pcr, uint8_t *measurement, size_t length)
{
	(void) store;
	(void) hash;
	if (!test_snapshot_active) {
		return -1;
	}
	if (test_compute_failure == (int) pcr) {
		return -1;
	}
	memset (measurement, 0x40 + pcr, length);
	return (int) length;
}

static int composite_eat_tip_evidence_test_begin_snapshot (struct pcr_store *store)
{
	(void) store;
	test_snapshot_active = 1;
	test_snapshot_begin_count++;
	return 0;
}

static int composite_eat_tip_evidence_test_end_snapshot (struct pcr_store *store)
{
	(void) store;
	test_snapshot_active = 0;
	test_snapshot_end_count++;
	return 0;
}

static int composite_eat_tip_evidence_test_get_dme_pcr0 (uint8_t *measurement,
	size_t length)
{
	if (test_dme_failure || (measurement == NULL) ||
		(length != TCG_CONCISE_EVIDENCE_SHA512_LENGTH)) {
		return -1;
	}
	memset (measurement, 0x5a, length);
	return (int) length;
}

static const struct composite_eat_tip_evidence_ops test_ops = {
	.get_pcr_count = composite_eat_tip_evidence_test_get_count,
	.get_pcr_digest_length = composite_eat_tip_evidence_test_get_length,
	.begin_snapshot = composite_eat_tip_evidence_test_begin_snapshot,
	.end_snapshot = composite_eat_tip_evidence_test_end_snapshot,
	.compute_pcr = composite_eat_tip_evidence_test_compute,
	.get_dme_pcr0 = composite_eat_tip_evidence_test_get_dme_pcr0,
};

static void composite_eat_tip_evidence_test_reset (void)
{
	memset (test_digest_lengths, 0, sizeof (test_digest_lengths));
	test_pcr_count = 2;
	test_digest_lengths[0] = SHA256_HASH_LENGTH;
	test_digest_lengths[1] = SHA384_HASH_LENGTH;
	test_compute_failure = -1;
	test_snapshot_active = 0;
	test_snapshot_begin_count = 0;
	test_snapshot_end_count = 0;
	test_dme_failure = 0;
}

static struct tcg_concise_evidence_environment composite_eat_tip_evidence_test_environment (
	const uint8_t *class_id, size_t class_id_length, const uint8_t *vendor,
	size_t vendor_length, const uint8_t *model, size_t model_length)
{
	struct tcg_concise_evidence_environment environment = {
		.has_class = true,
		.class_info = {
			.has_class_id = true,
			.class_id = {TCG_CONCISE_EVIDENCE_TAGGED_BYTES,
				{class_id, class_id_length}},
			.has_vendor = true,
			.vendor = {vendor, vendor_length},
			.has_model = true,
			.model = {model, model_length},
		},
	};

	return environment;
}

static void composite_eat_tip_evidence_test_capture (CuTest *test)
{
	static const uint8_t class_id[] = {0x01, 0x02, 0x03, 0x04};
	static const uint8_t vendor[] = "Example Vendor";
	static const uint8_t model[] = "Example Model";
	const struct tcg_concise_evidence_environment environment =
		composite_eat_tip_evidence_test_environment (class_id, sizeof (class_id), vendor,
			sizeof (vendor) - 1, model, sizeof (model) - 1);
	struct composite_eat_tip_evidence_adapter adapter;
	struct composite_eat_tip_evidence_workspace workspace;
	struct composite_eat_tip_local_evidence evidence;
	struct pcr_store store;
	struct hash_engine hash;
	enum composite_eat_tip_evidence_status status;

	TEST_START;
	composite_eat_tip_evidence_test_reset ();
	status = composite_eat_tip_evidence_adapter_init_with_ops (&adapter, &store, &hash,
		&environment, &test_ops);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_EVIDENCE_OK, status);
	status = composite_eat_tip_evidence_capture (&adapter, &workspace, &evidence);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_EVIDENCE_OK, status);
	CuAssertPtrEquals (test, &workspace.document, (void *) evidence.document);
	CuAssertTrue (test, evidence.encoded_length > 0);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_ALGORITHM_SHA256,
		workspace.digest_entries[0].algorithm.type);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_ALGORITHM_SHA384,
		workspace.digest_entries[1].algorithm.type);
	CuAssertIntEquals (test, 0x40, workspace.digests[0][0]);
	CuAssertIntEquals (test, 0x41, workspace.digests[1][0]);
	CuAssertIntEquals (test, 1, test_snapshot_begin_count);
	CuAssertIntEquals (test, 1, test_snapshot_end_count);
	CuAssertIntEquals (test, 0, test_snapshot_active);
}

static void composite_eat_tip_evidence_test_failure_clears (CuTest *test)
{
	static const uint8_t class_id[] = {0x01};
	static const uint8_t vendor[] = "Vendor";
	static const uint8_t model[] = "Model";
	const struct tcg_concise_evidence_environment environment =
		composite_eat_tip_evidence_test_environment (class_id, sizeof (class_id), vendor,
			sizeof (vendor) - 1, model, sizeof (model) - 1);
	struct composite_eat_tip_evidence_adapter adapter;
	struct composite_eat_tip_evidence_workspace workspace;
	struct composite_eat_tip_local_evidence evidence;
	struct pcr_store store;
	struct hash_engine hash;
	const uint8_t *bytes = (const uint8_t *) &workspace;
	size_t i;
	enum composite_eat_tip_evidence_status status;

	TEST_START;
	composite_eat_tip_evidence_test_reset ();
	test_compute_failure = 1;
	memset (&workspace, 0xa5, sizeof (workspace));
	composite_eat_tip_evidence_adapter_init_with_ops (&adapter, &store, &hash,
		&environment, &test_ops);
	status = composite_eat_tip_evidence_capture (&adapter, &workspace, &evidence);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_EVIDENCE_SNAPSHOT_ERROR, status);
	CuAssertPtrEquals (test, NULL, (void *) evidence.document);
	CuAssertIntEquals (test, 0, evidence.encoded_length);
	for (i = 0; i < sizeof (workspace); ++i) {
		CuAssertIntEquals (test, 0, bytes[i]);
	}
	CuAssertIntEquals (test, 1, test_snapshot_begin_count);
	CuAssertIntEquals (test, 1, test_snapshot_end_count);
	CuAssertIntEquals (test, 0, test_snapshot_active);
}

static void composite_eat_tip_evidence_test_dme_capture (CuTest *test)
{
	static const uint8_t class_id[] = {0x01};
	static const uint8_t dme_class_id[] = {0x02};
	static const uint8_t vendor[] = "Vendor";
	static const uint8_t model[] = "Model";
	static const uint8_t dme_model[] = "ROM DME";
	const struct tcg_concise_evidence_environment environment =
		composite_eat_tip_evidence_test_environment (class_id, sizeof (class_id), vendor,
			sizeof (vendor) - 1, model, sizeof (model) - 1);
	const struct tcg_concise_evidence_environment dme_environment =
		composite_eat_tip_evidence_test_environment (dme_class_id, sizeof (dme_class_id),
			vendor, sizeof (vendor) - 1, dme_model, sizeof (dme_model) - 1);
	struct composite_eat_tip_evidence_adapter adapter;
	struct composite_eat_tip_evidence_workspace workspace;
	struct composite_eat_tip_local_evidence evidence;
	struct pcr_store store;
	struct hash_engine hash;
	enum composite_eat_tip_evidence_status status;

	TEST_START;
	composite_eat_tip_evidence_test_reset ();
	status = composite_eat_tip_evidence_adapter_init_with_ops (&adapter, &store, &hash,
		&environment, &test_ops);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_EVIDENCE_OK, status);
	status = composite_eat_tip_evidence_enable_dme (&adapter, &dme_environment);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_EVIDENCE_OK, status);
	status = composite_eat_tip_evidence_capture (&adapter, &workspace, &evidence);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_EVIDENCE_OK, status);
	CuAssertIntEquals (test, 2, workspace.document.evidence_triple_count);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_TEXT,
		workspace.dme_measurement.key.type);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_ALGORITHM_SHA512,
		workspace.dme_digest.algorithm.type);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_SHA512_LENGTH,
		workspace.dme_digest.value.length);
	CuAssertIntEquals (test, 0x5a, workspace.dme_pcr0[0]);
	CuAssertTrue (test, workspace.dme_measurement.values.flags.is_immutable);
	CuAssertTrue (test, workspace.dme_measurement.values.flags.is_tcb);

	test_dme_failure = 1;
	status = composite_eat_tip_evidence_capture (&adapter, &workspace, &evidence);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_EVIDENCE_SNAPSHOT_ERROR, status);
	CuAssertPtrEquals (test, NULL, (void *) evidence.document);
	CuAssertIntEquals (test, 0, evidence.encoded_length);
}


TEST_SUITE_START (composite_eat_tip_evidence_adapter);

TEST (composite_eat_tip_evidence_test_capture);
TEST (composite_eat_tip_evidence_test_failure_clears);
TEST (composite_eat_tip_evidence_test_dme_capture);

TEST_SUITE_END;