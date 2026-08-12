/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef COMPOSITE_EAT_CODEC_H_
#define COMPOSITE_EAT_CODEC_H_

#include <stddef.h>
#include <stdint.h>


#define COMPOSITE_EAT_REQUEST_VERSION 1u
#define COMPOSITE_EAT_MAX_DEVICE_RECORDS 64u
#define COMPOSITE_EAT_MIN_NONCE_LENGTH 8u
#define COMPOSITE_EAT_MAX_NONCE_LENGTH 64u
#define COMPOSITE_EAT_MAX_ENVIRONMENT_LENGTH 64u
#define COMPOSITE_EAT_SHA384_LENGTH 48u
#define COMPOSITE_EAT_ES384_SIGNATURE_LENGTH 96u
#define COMPOSITE_EAT_MAX_CERTIFICATES 4u

#define COMPOSITE_EAT_COSE_ALGORITHM_ES384 (-35)
#define COMPOSITE_EAT_COSE_ALGORITHM_SHA384 (-43)

enum composite_eat_codec_status {
	COMPOSITE_EAT_CODEC_OK = 0,
	COMPOSITE_EAT_CODEC_BAD_ARGUMENT,
	COMPOSITE_EAT_CODEC_MALFORMED_REQUEST,
	COMPOSITE_EAT_CODEC_BAD_VERSION,
	COMPOSITE_EAT_CODEC_TOO_MANY_RECORDS,
	COMPOSITE_EAT_CODEC_BUFFER_TOO_SMALL,
	COMPOSITE_EAT_CODEC_ENCODING_ERROR,
};

struct composite_eat_device_record {
	uint8_t environment[COMPOSITE_EAT_MAX_ENVIRONMENT_LENGTH];
	size_t environment_length;
	uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH];
};

struct composite_eat_request {
	uint32_t version;
	uint8_t nonce[COMPOSITE_EAT_MAX_NONCE_LENGTH];
	size_t nonce_length;
	struct composite_eat_device_record records[COMPOSITE_EAT_MAX_DEVICE_RECORDS];
	size_t record_count;
};

struct composite_eat_buffer {
	const uint8_t *data;
	size_t length;
};

struct composite_eat_claims {
	struct composite_eat_buffer ueid;
	struct composite_eat_buffer profile;
	uint64_t measurement_content_format;
	struct composite_eat_buffer measurements;
};

struct composite_eat_cose_sign1 {
	struct composite_eat_buffer protected_headers;
	struct composite_eat_buffer payload;
	struct composite_eat_buffer signature;
	struct composite_eat_buffer certificates[COMPOSITE_EAT_MAX_CERTIFICATES];
	size_t certificate_count;
};

enum composite_eat_codec_status composite_eat_codec_decode_request (const uint8_t *encoded,
	size_t encoded_length, struct composite_eat_request *request);

enum composite_eat_codec_status
composite_eat_codec_encode_request (const struct composite_eat_request *request, uint8_t *encoded,
	size_t encoded_capacity, size_t *encoded_length);

enum composite_eat_codec_status
composite_eat_codec_encode_payload (const struct composite_eat_request *request,
	const struct composite_eat_claims *claims, uint8_t *encoded, size_t encoded_capacity,
	size_t *encoded_length);

enum composite_eat_codec_status
composite_eat_codec_encode_protected_headers (const uint8_t certificate_thumbprint
												  [COMPOSITE_EAT_SHA384_LENGTH],
	uint8_t *encoded, size_t encoded_capacity, size_t *encoded_length);

enum composite_eat_codec_status
composite_eat_codec_encode_cose_sign1 (const struct composite_eat_cose_sign1 *cose,
	uint8_t *encoded, size_t encoded_capacity, size_t *encoded_length);


#endif /* COMPOSITE_EAT_CODEC_H_ */