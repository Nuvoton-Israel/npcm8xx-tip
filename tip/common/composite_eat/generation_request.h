/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef COMPOSITE_EAT_GENERATION_REQUEST_H_
#define COMPOSITE_EAT_GENERATION_REQUEST_H_

#include <stddef.h>
#include <stdint.h>

#include "composite_eat/types.h"


#define COMPOSITE_EAT_GENERATION_REQUEST_VERSION 1u
#define COMPOSITE_EAT_NONCE_LENGTH 32u
#define COMPOSITE_EAT_MAX_DEVICE_RECORDS 64u
#define COMPOSITE_EAT_MAX_ENVIRONMENT_LENGTH 64u
#define COMPOSITE_EAT_COSE_ALGORITHM_SHA384 (-43)

enum composite_eat_generation_request_status {
	COMPOSITE_EAT_GENERATION_REQUEST_OK = 0,
	COMPOSITE_EAT_GENERATION_REQUEST_BAD_ARGUMENT,
	COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED,
	COMPOSITE_EAT_GENERATION_REQUEST_BAD_VERSION,
	COMPOSITE_EAT_GENERATION_REQUEST_TOO_MANY_RECORDS,
};

/* Decoded byte fields are immutable views into the encoded request buffer. */
struct composite_eat_device_record {
	const uint8_t *environment;
	size_t environment_length;
	const uint8_t *digest;
	size_t digest_length;
};

struct composite_eat_generation_request {
	uint32_t version;
	const uint8_t *nonce;
	size_t nonce_length;
	struct composite_eat_device_record records[COMPOSITE_EAT_MAX_DEVICE_RECORDS];
	size_t record_count;
};

enum composite_eat_generation_request_status composite_eat_generation_request_validate (
	const struct composite_eat_generation_request *request);

/**
 * Decode and validate a generation request without copying byte-string contents.
 *
 * On success, byte fields in request point into encoded.  The encoded buffer must remain
 * immutable and valid until request is no longer used.
 */
enum composite_eat_generation_request_status composite_eat_generation_request_decode (
	const uint8_t *encoded, size_t encoded_length, struct composite_eat_generation_request *request);


#endif /* COMPOSITE_EAT_GENERATION_REQUEST_H_ */