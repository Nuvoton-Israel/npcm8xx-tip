/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef COMPOSITE_EAT_TIP_MAIN_TOKEN_GENERATOR_H_
#define COMPOSITE_EAT_TIP_MAIN_TOKEN_GENERATOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crypto/ecc.h"
#include "crypto/hash.h"
#include "riot/riot_key_manager.h"
#include "composite_eat/generation_request.h"
#include "composite_eat/main_token_builder.h"
#include "composite_eat/tip_evidence_adapter.h"


#define COMPOSITE_EAT_TIP_UEID_LENGTH 17u
#define COMPOSITE_EAT_TIP_REQUEST_SNAPSHOT_MAX 4096u
#define COMPOSITE_EAT_TIP_MAX_RESPONSE_LENGTH 16384u

enum composite_eat_tip_main_token_status {
	COMPOSITE_EAT_TIP_MAIN_TOKEN_OK = 0,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_ARGUMENT,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_REQUEST,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_VERSION,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_TOO_MANY_RECORDS,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_BUSY,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_IDENTITY_ERROR,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_EVIDENCE_ERROR,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_HASH_ERROR,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_SIGN_ERROR,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_BUFFER_TOO_SMALL,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_RESPONSE_TOO_LARGE,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_CANCELED,
	COMPOSITE_EAT_TIP_MAIN_TOKEN_ENCODING_ERROR,
};

struct composite_eat_tip_main_token_ops {
	const struct riot_keys *(*get_keys) (const struct riot_key_manager *riot);
	void (*release_keys) (const struct riot_key_manager *riot, const struct riot_keys *keys);
	const struct der_cert *(*get_intermediate_ca) (const struct riot_key_manager *riot);
	const struct der_cert *(*get_root_ca) (const struct riot_key_manager *riot);
};

/* Caller-owned feature-on static RAM; one workspace supports one in-flight operation. */
struct composite_eat_tip_main_token_workspace {
	bool active;
	struct composite_eat_generation_request request;
	uint8_t request_snapshot[COMPOSITE_EAT_TIP_REQUEST_SNAPSHOT_MAX];
	struct composite_eat_tip_evidence_workspace evidence;
	struct composite_eat_main_token_workspace token;
	struct composite_eat_main_token_prepared prepared;
	struct composite_eat_der_certificate certificates[COMPOSITE_EAT_MAX_CERTIFICATES];
	uint8_t ueid[COMPOSITE_EAT_TIP_UEID_LENGTH];
	uint8_t leaf_thumbprint[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t signature_digest[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t signature[COMPOSITE_EAT_ES384_SIGNATURE_LENGTH];
};

struct composite_eat_tip_main_token_generator {
	const struct ecc_engine *ecc;
	const struct hash_engine *hash;
	const struct riot_key_manager *riot;
	const struct composite_eat_tip_evidence_adapter *evidence;
	struct composite_eat_buffer profile;
	struct composite_eat_tip_main_token_workspace *workspace;
	const struct composite_eat_tip_main_token_ops *ops;
};

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_generator_init (
	struct composite_eat_tip_main_token_generator *generator, const struct ecc_engine *ecc,
	const struct hash_engine *hash, const struct riot_key_manager *riot,
	const struct composite_eat_tip_evidence_adapter *evidence,
	const struct composite_eat_buffer *profile,
	struct composite_eat_tip_main_token_workspace *workspace);

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_generator_init_with_ops (
	struct composite_eat_tip_main_token_generator *generator, const struct ecc_engine *ecc,
	const struct hash_engine *hash, const struct riot_key_manager *riot,
	const struct composite_eat_tip_evidence_adapter *evidence,
	const struct composite_eat_buffer *profile,
	struct composite_eat_tip_main_token_workspace *workspace,
	const struct composite_eat_tip_main_token_ops *ops);

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_generate (
	const struct composite_eat_tip_main_token_generator *generator, const uint8_t *request,
	size_t request_length, uint8_t *response, size_t response_capacity,
	size_t *response_length);

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_generate_write (
	const struct composite_eat_tip_main_token_generator *generator, const uint8_t *request,
	size_t request_length, composite_eat_main_token_write write, void *context,
	size_t response_capacity, size_t *response_length);

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_snapshot_request (
	const struct composite_eat_tip_main_token_generator *generator, const uint8_t *request,
	size_t request_length, const uint8_t **snapshot);


#endif /* COMPOSITE_EAT_TIP_MAIN_TOKEN_GENERATOR_H_ */