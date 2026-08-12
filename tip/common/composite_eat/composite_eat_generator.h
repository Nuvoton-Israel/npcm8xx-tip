/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef COMPOSITE_EAT_GENERATOR_H_
#define COMPOSITE_EAT_GENERATOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attestation/pcr_store.h"
#include "crypto/ecc.h"
#include "crypto/hash.h"
#include "riot/riot_key_manager.h"
#include "composite_eat/composite_eat_codec.h"


#define COMPOSITE_EAT_PAYLOAD_MAX 9216u
#define COMPOSITE_EAT_EVIDENCE_MAX 1024u
#define COMPOSITE_EAT_PROTECTED_MAX 128u
#define COMPOSITE_EAT_MAX_PCR_DIGESTS 8u

enum composite_eat_generator_status {
	COMPOSITE_EAT_GENERATOR_OK = 0,
	COMPOSITE_EAT_GENERATOR_BAD_ARGUMENT,
	COMPOSITE_EAT_GENERATOR_BAD_REQUEST,
	COMPOSITE_EAT_GENERATOR_BUFFER_TOO_SMALL,
	COMPOSITE_EAT_GENERATOR_ATTESTATION_ERROR,
	COMPOSITE_EAT_GENERATOR_HASH_ERROR,
	COMPOSITE_EAT_GENERATOR_SIGN_ERROR,
	COMPOSITE_EAT_GENERATOR_ENCODING_ERROR,
	COMPOSITE_EAT_GENERATOR_BUSY,
};

struct composite_eat_generator_workspace {
	struct composite_eat_request request;
	uint8_t payload[COMPOSITE_EAT_PAYLOAD_MAX];
	uint8_t evidence[COMPOSITE_EAT_EVIDENCE_MAX];
	uint8_t protected_headers[COMPOSITE_EAT_PROTECTED_MAX];
	bool active;
};

struct composite_eat_attestation_ops {
	const struct riot_keys *(*get_keys) (const struct riot_key_manager *riot);
	void (*release_keys) (const struct riot_key_manager *riot, const struct riot_keys *keys);
	const struct der_cert *(*get_intermediate_ca) (const struct riot_key_manager *riot);
	const struct der_cert *(*get_root_ca) (const struct riot_key_manager *riot);
	int (*get_pcr_count) (struct pcr_store *store);
	int (*get_pcr_digest_length) (struct pcr_store *store, uint8_t pcr);
	int (*compute_pcr) (struct pcr_store *store, const struct hash_engine *hash, uint8_t pcr,
		uint8_t *measurement, size_t length);
};

struct composite_eat_generator {
	const struct ecc_engine *ecc;
	const struct hash_engine *hash;
	const struct riot_key_manager *riot;
	struct pcr_store *pcr_store;
	struct composite_eat_generator_workspace *workspace;
	const struct composite_eat_attestation_ops *attestation;
};

enum composite_eat_generator_status
composite_eat_generator_init (struct composite_eat_generator *generator,
	const struct ecc_engine *ecc, const struct hash_engine *hash,
	const struct riot_key_manager *riot, struct pcr_store *pcr_store,
	struct composite_eat_generator_workspace *workspace);

enum composite_eat_generator_status
composite_eat_generator_init_with_attestation (struct composite_eat_generator *generator,
	const struct ecc_engine *ecc, const struct hash_engine *hash,
	const struct riot_key_manager *riot, struct pcr_store *pcr_store,
	struct composite_eat_generator_workspace *workspace,
	const struct composite_eat_attestation_ops *attestation);

enum composite_eat_generator_status
composite_eat_generate (const struct composite_eat_generator *generator, const uint8_t *request,
	size_t request_length, uint8_t *response, size_t response_capacity, size_t *response_length);


#endif /* COMPOSITE_EAT_GENERATOR_H_ */