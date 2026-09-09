/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "composite_eat/tip_evidence_adapter.h"
#include "tip_rom_dme_handoff.h"


#ifndef COMPOSITE_EAT_TIP_EVIDENCE_NO_DEFAULT_OPS
static int composite_eat_tip_evidence_begin_snapshot (struct pcr_store *store)
{
	size_t pcr;

	for (pcr = 0; pcr < store->num_pcrs; ++pcr) {
		if (platform_mutex_lock (&store->pcrs[pcr].lock) != 0) {
			while (pcr > 0) {
				platform_mutex_unlock (&store->pcrs[--pcr].lock);
			}
			return -1;
		}
	}
	return 0;
}

static int composite_eat_tip_evidence_end_snapshot (struct pcr_store *store)
{
	size_t pcr = store->num_pcrs;
	int status = 0;

	while (pcr > 0) {
		if (platform_mutex_unlock (&store->pcrs[--pcr].lock) != 0) {
			status = -1;
		}
	}
	return status;
}

static int composite_eat_tip_evidence_compute_pcr (struct pcr_store *store,
	const struct hash_engine *hash, uint8_t pcr, uint8_t *measurement, size_t length)
{
	if (pcr >= store->num_pcrs) {
		return PCR_INVALID_PCR;
	}
	return pcr_compute (&store->pcrs[pcr], hash, false, measurement, length);
}

static int composite_eat_tip_evidence_get_dme_pcr0 (uint8_t *measurement, size_t length)
{
	struct tip_rom_dme_public_evidence evidence;

	if ((measurement == NULL) || (length != TCG_CONCISE_EVIDENCE_SHA512_LENGTH) ||
		(tip_rom_dme_handoff_get (&evidence) != TIP_ROM_DME_HANDOFF_OK) ||
		(evidence.dme_pcr0.length != TCG_CONCISE_EVIDENCE_SHA512_LENGTH)) {
		return -1;
	}

	memcpy (measurement, evidence.dme_pcr0.data, evidence.dme_pcr0.length);
	return (int) evidence.dme_pcr0.length;
}

static const struct composite_eat_tip_evidence_ops composite_eat_tip_evidence_default_ops = {
	.get_pcr_count = pcr_store_get_num_pcrs,
	.get_pcr_digest_length = pcr_store_get_pcr_digest_length,
	.begin_snapshot = composite_eat_tip_evidence_begin_snapshot,
	.end_snapshot = composite_eat_tip_evidence_end_snapshot,
	.compute_pcr = composite_eat_tip_evidence_compute_pcr,
	.get_dme_pcr0 = composite_eat_tip_evidence_get_dme_pcr0,
};
#endif

static const struct composite_eat_profile_evidence_policy composite_eat_tip_evidence_policy = {
	.max_triples = 2,
	.max_measurements_per_triple = COMPOSITE_EAT_TIP_EVIDENCE_MAX_PCRS,
	.max_digests_per_measurement = 2,
	.max_identifier_length = 64,
	.max_text_length = 64,
	.max_encoded_length = COMPOSITE_EAT_TIP_EVIDENCE_MAX_LENGTH,
	.allowed_mkey_types = COMPOSITE_EAT_PROFILE_MKEY_UINT |
		COMPOSITE_EAT_PROFILE_MKEY_TEXT,
	.allowed_algorithms = COMPOSITE_EAT_PROFILE_ALGORITHM_SHA256 |
		COMPOSITE_EAT_PROFILE_ALGORITHM_SHA384 |
		COMPOSITE_EAT_PROFILE_ALGORITHM_SHA512,
	.require_measurement_key = true,
	.require_class = true,
	.require_class_id = true,
	.require_vendor = true,
	.require_model = true,
	.allow_flags = true,
	.allow_name = true,
};

static bool composite_eat_tip_evidence_environment_valid (
	const struct tcg_concise_evidence_environment *environment)
{
	return composite_eat_profile_validate_environment (environment,
			&composite_eat_tip_evidence_policy) ==
			COMPOSITE_EAT_PROFILE_EVIDENCE_OK;
}

enum composite_eat_tip_evidence_status composite_eat_tip_evidence_adapter_init (
	struct composite_eat_tip_evidence_adapter *adapter, struct pcr_store *pcr_store,
	const struct hash_engine *hash,
	const struct tcg_concise_evidence_environment *environment)
{
#ifdef COMPOSITE_EAT_TIP_EVIDENCE_NO_DEFAULT_OPS
	(void) adapter;
	(void) pcr_store;
	(void) hash;
	(void) environment;
	return COMPOSITE_EAT_TIP_EVIDENCE_BAD_ARGUMENT;
#else
	return composite_eat_tip_evidence_adapter_init_with_ops (adapter, pcr_store, hash,
		environment, &composite_eat_tip_evidence_default_ops);
#endif
}

enum composite_eat_tip_evidence_status composite_eat_tip_evidence_adapter_init_with_ops (
	struct composite_eat_tip_evidence_adapter *adapter, struct pcr_store *pcr_store,
	const struct hash_engine *hash,
	const struct tcg_concise_evidence_environment *environment,
	const struct composite_eat_tip_evidence_ops *ops)
{
	if ((adapter == NULL) || (pcr_store == NULL) || (hash == NULL) ||
		!composite_eat_tip_evidence_environment_valid (environment) || (ops == NULL) ||
		(ops->get_pcr_count == NULL) || (ops->get_pcr_digest_length == NULL) ||
		(ops->begin_snapshot == NULL) || (ops->end_snapshot == NULL) ||
		(ops->compute_pcr == NULL)) {
		return COMPOSITE_EAT_TIP_EVIDENCE_BAD_ARGUMENT;
	}

	adapter->pcr_store = pcr_store;
	adapter->hash = hash;
	adapter->environment = *environment;
	adapter->include_dme = false;
	memset (&adapter->dme_environment, 0, sizeof (adapter->dme_environment));
	adapter->ops = ops;
	return COMPOSITE_EAT_TIP_EVIDENCE_OK;
}

enum composite_eat_tip_evidence_status composite_eat_tip_evidence_enable_dme (
	struct composite_eat_tip_evidence_adapter *adapter,
	const struct tcg_concise_evidence_environment *environment)
{
	if ((adapter == NULL) || !composite_eat_tip_evidence_environment_valid (environment) ||
		(adapter->ops == NULL) ||
		(adapter->ops->get_dme_pcr0 == NULL)) {
		return COMPOSITE_EAT_TIP_EVIDENCE_BAD_ARGUMENT;
	}

	adapter->dme_environment = *environment;
	adapter->include_dme = true;
	return COMPOSITE_EAT_TIP_EVIDENCE_OK;
}

enum composite_eat_tip_evidence_status composite_eat_tip_evidence_capture (
	const struct composite_eat_tip_evidence_adapter *adapter,
	struct composite_eat_tip_evidence_workspace *workspace,
	struct composite_eat_tip_local_evidence *evidence)
{
	int count;
	int digest_length;
	int result;
	int pcr;
	enum composite_eat_profile_evidence_status profile_status;

	if ((workspace == NULL) || (evidence == NULL)) {
		return COMPOSITE_EAT_TIP_EVIDENCE_BAD_ARGUMENT;
	}
	composite_eat_tip_evidence_clear (workspace);
	evidence->document = NULL;
	evidence->encoded_length = 0;
	if ((adapter == NULL) || (adapter->pcr_store == NULL) || (adapter->hash == NULL) ||
		(adapter->ops == NULL)) {
		return COMPOSITE_EAT_TIP_EVIDENCE_BAD_ARGUMENT;
	}

	count = adapter->ops->get_pcr_count (adapter->pcr_store);
	if ((count <= 0) || (count > (int) COMPOSITE_EAT_TIP_EVIDENCE_MAX_PCRS)) {
		return COMPOSITE_EAT_TIP_EVIDENCE_SNAPSHOT_ERROR;
	}
	if (adapter->ops->begin_snapshot (adapter->pcr_store) != 0) {
		return COMPOSITE_EAT_TIP_EVIDENCE_SNAPSHOT_ERROR;
	}

	for (pcr = 0; pcr < count; ++pcr) {
		digest_length = adapter->ops->get_pcr_digest_length (adapter->pcr_store,
			(uint8_t) pcr);
		if ((digest_length != SHA256_HASH_LENGTH) &&
			(digest_length != SHA384_HASH_LENGTH)) {
			goto snapshot_error;
		}

		result = adapter->ops->compute_pcr (adapter->pcr_store, adapter->hash,
			(uint8_t) pcr, workspace->digests[pcr], (size_t) digest_length);
		if (result != digest_length) {
			goto snapshot_error;
		}

		workspace->digest_entries[pcr].algorithm.type =
			(digest_length == SHA256_HASH_LENGTH) ?
			TCG_CONCISE_EVIDENCE_ALGORITHM_SHA256 :
			TCG_CONCISE_EVIDENCE_ALGORITHM_SHA384;
		workspace->digest_entries[pcr].value.data = workspace->digests[pcr];
		workspace->digest_entries[pcr].value.length = (size_t) digest_length;
		workspace->measurements[pcr].has_key = true;
		workspace->measurements[pcr].key.type =
			TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_UINT;
		workspace->measurements[pcr].key.value.uint_value = (uint64_t) pcr;
		workspace->measurements[pcr].values.digests = &workspace->digest_entries[pcr];
		workspace->measurements[pcr].values.digest_count = 1;
	}
	if (adapter->ops->end_snapshot (adapter->pcr_store) != 0) {
		composite_eat_tip_evidence_clear (workspace);
		return COMPOSITE_EAT_TIP_EVIDENCE_SNAPSHOT_ERROR;
	}

	workspace->triples[0].environment = adapter->environment;
	workspace->triples[0].measurements = workspace->measurements;
	workspace->triples[0].measurement_count = (size_t) count;
	workspace->document.evidence_triples = workspace->triples;
	workspace->document.evidence_triple_count = 1;
	if (adapter->include_dme) {
		static const uint8_t dme_pcr0_key[] = "dme-pcr0";

		result = adapter->ops->get_dme_pcr0 (workspace->dme_pcr0,
			sizeof (workspace->dme_pcr0));
		if (result != (int) sizeof (workspace->dme_pcr0)) {
			composite_eat_tip_evidence_clear (workspace);
			return COMPOSITE_EAT_TIP_EVIDENCE_SNAPSHOT_ERROR;
		}
		workspace->dme_digest.algorithm.type = TCG_CONCISE_EVIDENCE_ALGORITHM_SHA512;
		workspace->dme_digest.value.data = workspace->dme_pcr0;
		workspace->dme_digest.value.length = sizeof (workspace->dme_pcr0);
		workspace->dme_measurement.has_key = true;
		workspace->dme_measurement.key.type = TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_TEXT;
		workspace->dme_measurement.key.value.text.data = dme_pcr0_key;
		workspace->dme_measurement.key.value.text.length = sizeof (dme_pcr0_key) - 1;
		workspace->dme_measurement.values.digests = &workspace->dme_digest;
		workspace->dme_measurement.values.digest_count = 1;
		workspace->dme_measurement.values.has_flags = true;
		workspace->dme_measurement.values.flags.has_is_integrity_protected = true;
		workspace->dme_measurement.values.flags.is_integrity_protected = true;
		workspace->dme_measurement.values.flags.has_is_immutable = true;
		workspace->dme_measurement.values.flags.is_immutable = true;
		workspace->dme_measurement.values.flags.has_is_tcb = true;
		workspace->dme_measurement.values.flags.is_tcb = true;
		workspace->triples[1].environment = adapter->dme_environment;
		workspace->triples[1].measurements = &workspace->dme_measurement;
		workspace->triples[1].measurement_count = 1;
		workspace->document.evidence_triple_count = 2;
	}
	profile_status = composite_eat_profile_validate_local_evidence (&workspace->document,
		&composite_eat_tip_evidence_policy, &evidence->encoded_length);
	if (profile_status != COMPOSITE_EAT_PROFILE_EVIDENCE_OK) {
		composite_eat_tip_evidence_clear (workspace);
		evidence->encoded_length = 0;
		return COMPOSITE_EAT_TIP_EVIDENCE_ENCODING_ERROR;
	}

	evidence->document = &workspace->document;
	return COMPOSITE_EAT_TIP_EVIDENCE_OK;

snapshot_error:
	adapter->ops->end_snapshot (adapter->pcr_store);
	composite_eat_tip_evidence_clear (workspace);
	evidence->encoded_length = 0;
	return COMPOSITE_EAT_TIP_EVIDENCE_SNAPSHOT_ERROR;
}

void composite_eat_tip_evidence_clear (struct composite_eat_tip_evidence_workspace *workspace)
{
	volatile uint8_t *cursor;
	size_t length;

	if (workspace == NULL) {
		return;
	}

	cursor = (volatile uint8_t *) workspace;
	length = sizeof (*workspace);
	while (length-- > 0) {
		*cursor++ = 0;
	}
}