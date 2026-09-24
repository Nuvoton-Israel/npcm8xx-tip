/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef COMPOSITE_EAT_TIP_EVIDENCE_ADAPTER_H_
#define COMPOSITE_EAT_TIP_EVIDENCE_ADAPTER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attestation/pcr_store.h"
#include "crypto/hash.h"
#include "composite_eat/profile_evidence.h"
#include "composite_eat/tcg_concise_evidence.h"
#include "composite_eat/types.h"


#define COMPOSITE_EAT_TIP_EVIDENCE_MAX_LENGTH 1024u
#define COMPOSITE_EAT_TIP_EVIDENCE_MAX_PCRS 8u

enum composite_eat_tip_evidence_status {
	COMPOSITE_EAT_TIP_EVIDENCE_OK = 0,
	COMPOSITE_EAT_TIP_EVIDENCE_BAD_ARGUMENT,
	COMPOSITE_EAT_TIP_EVIDENCE_SNAPSHOT_ERROR,
	COMPOSITE_EAT_TIP_EVIDENCE_ENCODING_ERROR,
};

struct composite_eat_tip_evidence_ops {
	int (*get_pcr_count) (struct pcr_store *store);
	int (*get_pcr_digest_length) (struct pcr_store *store, uint8_t pcr);
	int (*begin_snapshot) (struct pcr_store *store);
	int (*end_snapshot) (struct pcr_store *store);
	int (*compute_pcr) (struct pcr_store *store, const struct hash_engine *hash,
		uint8_t pcr, uint8_t *measurement, size_t length);
	int (*get_dme_pcr0) (uint8_t *measurement, size_t length);
};

struct composite_eat_tip_evidence_adapter {
	struct pcr_store *pcr_store;
	const struct hash_engine *hash;
	struct tcg_concise_evidence_environment environment;
	bool include_dme;
	struct tcg_concise_evidence_environment dme_environment;
	const struct composite_eat_tip_evidence_ops *ops;
};

struct composite_eat_tip_evidence_workspace {
	uint8_t digests[COMPOSITE_EAT_TIP_EVIDENCE_MAX_PCRS]
		[COMPOSITE_EAT_SHA384_LENGTH];
	struct tcg_concise_evidence_digest digest_entries[COMPOSITE_EAT_TIP_EVIDENCE_MAX_PCRS];
	struct tcg_concise_evidence_measurement measurements[COMPOSITE_EAT_TIP_EVIDENCE_MAX_PCRS];
	uint8_t dme_pcr0[TCG_CONCISE_EVIDENCE_SHA512_LENGTH];
	struct tcg_concise_evidence_digest dme_digest;
	struct tcg_concise_evidence_measurement dme_measurement;
	struct tcg_concise_evidence_triple triples[2];
	struct tcg_concise_evidence_document document;
};

struct composite_eat_tip_local_evidence {
	const struct tcg_concise_evidence_document *document;
	size_t encoded_length;
};

enum composite_eat_tip_evidence_status composite_eat_tip_evidence_adapter_init (
	struct composite_eat_tip_evidence_adapter *adapter, struct pcr_store *pcr_store,
	const struct hash_engine *hash,
	const struct tcg_concise_evidence_environment *environment);

enum composite_eat_tip_evidence_status composite_eat_tip_evidence_adapter_init_with_ops (
	struct composite_eat_tip_evidence_adapter *adapter, struct pcr_store *pcr_store,
	const struct hash_engine *hash,
	const struct tcg_concise_evidence_environment *environment,
	const struct composite_eat_tip_evidence_ops *ops);

enum composite_eat_tip_evidence_status composite_eat_tip_evidence_capture (
	const struct composite_eat_tip_evidence_adapter *adapter,
	struct composite_eat_tip_evidence_workspace *workspace,
	struct composite_eat_tip_local_evidence *evidence);

enum composite_eat_tip_evidence_status composite_eat_tip_evidence_enable_dme (
	struct composite_eat_tip_evidence_adapter *adapter,
	const struct tcg_concise_evidence_environment *environment);

void composite_eat_tip_evidence_clear (
	struct composite_eat_tip_evidence_workspace *workspace);


#endif /* COMPOSITE_EAT_TIP_EVIDENCE_ADAPTER_H_ */