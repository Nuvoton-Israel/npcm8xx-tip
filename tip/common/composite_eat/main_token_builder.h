/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef COMPOSITE_EAT_MAIN_TOKEN_BUILDER_H_
#define COMPOSITE_EAT_MAIN_TOKEN_BUILDER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "composite_eat/generation_request.h"
#include "composite_eat/types.h"


#define COMPOSITE_EAT_ES384_SIGNATURE_LENGTH 96u
#define COMPOSITE_EAT_MAX_CERTIFICATES 4u
#define COMPOSITE_EAT_MAX_CERTIFICATE_LENGTH 4096u
#define COMPOSITE_EAT_MAX_PROFILE_LENGTH 128u
#define COMPOSITE_EAT_MAX_LOCAL_EVIDENCE 4u
#define COMPOSITE_EAT_MAX_LOCAL_EVIDENCE_LENGTH 4096u
#define COMPOSITE_EAT_PAYLOAD_MAX 9216u
#define COMPOSITE_EAT_PROTECTED_MAX 160u
#define COMPOSITE_EAT_MAX_RESPONSE_LENGTH 26624u
#define COMPOSITE_EAT_COSE_ALGORITHM_ES384 (-35)

/* ES384 carries the fixed-width raw COSE r || s signature. */
_Static_assert (COMPOSITE_EAT_ES384_SIGNATURE_LENGTH == (2u * COMPOSITE_EAT_SHA384_LENGTH),
	"Unexpected ES384 signature length");

enum composite_eat_main_token_status {
	COMPOSITE_EAT_MAIN_TOKEN_OK = 0,
	COMPOSITE_EAT_MAIN_TOKEN_BAD_ARGUMENT,
	COMPOSITE_EAT_MAIN_TOKEN_STATE_ERROR,
	COMPOSITE_EAT_MAIN_TOKEN_BUFFER_TOO_SMALL,
	COMPOSITE_EAT_MAIN_TOKEN_ENCODING_ERROR,
	COMPOSITE_EAT_MAIN_TOKEN_WRITE_ERROR,
};

typedef int (*composite_eat_main_token_write) (void *context, const uint8_t *data,
	size_t length);

typedef int (*composite_eat_local_evidence_write) (void *context,
	composite_eat_main_token_write write, void *write_context);

struct composite_eat_local_evidence {
	uint16_t content_format;
	/* Set exactly one source: non-empty encoded bytes, or write plus encoded_length. */
	struct composite_eat_buffer encoded;
	size_t encoded_length;
	/* A stream must emit exactly encoded_length immutable, deterministic bytes per call. */
	composite_eat_local_evidence_write write;
	void *write_context;
};

struct composite_eat_evidence_snapshot {
	struct composite_eat_buffer ueid;
	struct composite_eat_buffer profile;
	const struct composite_eat_local_evidence *local_evidence;
	size_t local_evidence_count;
};

struct composite_eat_der_certificate {
	const uint8_t *data;
	size_t length;
};

struct composite_eat_attestation_identity {
	const struct composite_eat_der_certificate *certificates;
	size_t certificate_count;
};

struct composite_eat_main_token_workspace {
	uint8_t protected_headers[COMPOSITE_EAT_PROTECTED_MAX];
	size_t protected_headers_length;
};

/*
 * Prepared state borrows request, evidence, identity, certificate, and stream-context data.
 * Every borrowed object must remain immutable and valid through signing and finish.
 */
struct composite_eat_main_token_prepared {
	struct composite_eat_main_token_workspace *workspace;
	const struct composite_eat_generation_request *request;
	const struct composite_eat_evidence_snapshot *evidence;
	const struct composite_eat_attestation_identity *identity;
	size_t payload_length;
	size_t response_length;
	bool ready;
	bool signing_complete;
};

enum composite_eat_main_token_status composite_eat_main_token_prepare (
	const struct composite_eat_generation_request *request,
	const struct composite_eat_evidence_snapshot *evidence,
	const struct composite_eat_attestation_identity *identity,
	const uint8_t leaf_thumbprint_sha384[COMPOSITE_EAT_SHA384_LENGTH],
	struct composite_eat_main_token_workspace *workspace,
	struct composite_eat_main_token_prepared *prepared);

enum composite_eat_main_token_status composite_eat_main_token_write_signing_input (
	struct composite_eat_main_token_prepared *prepared, composite_eat_main_token_write write,
	void *context);

enum composite_eat_main_token_status composite_eat_main_token_finish (
	const struct composite_eat_main_token_prepared *prepared,
	const uint8_t signature[COMPOSITE_EAT_ES384_SIGNATURE_LENGTH], uint8_t *response,
	size_t response_capacity, size_t *response_length);

enum composite_eat_main_token_status composite_eat_main_token_finish_write (
	const struct composite_eat_main_token_prepared *prepared,
	const uint8_t signature[COMPOSITE_EAT_ES384_SIGNATURE_LENGTH],
	composite_eat_main_token_write write, void *context, size_t response_capacity,
	size_t *response_length);

void composite_eat_main_token_clear (struct composite_eat_main_token_workspace *workspace,
	struct composite_eat_main_token_prepared *prepared);


#endif /* COMPOSITE_EAT_MAIN_TOKEN_BUILDER_H_ */