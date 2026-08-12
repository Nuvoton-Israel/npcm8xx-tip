/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "testing.h"
#include "asn1/ecc_der_util.h"
#include "composite_eat/composite_eat_generator.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_spiffy_decode.h"


TEST_SUITE_LABEL ("composite_eat_generator");

static uint8_t alias_key[] = {0xa0, 0xa1, 0xa2, 0xa3};
static uint8_t alias_certificate[64];
static uint8_t device_certificate[80];
static struct riot_keys test_keys;
static struct riot_key_manager test_riot;
static struct pcr_store test_pcr_store;
static int release_count;
static int hash_failure;
static int hash_stream_failure;
static int hash_cancel_count;
static int sign_failure;
static int pcr_failure;
static int pcr_digest_length = SHA256_HASH_LENGTH;
static uint8_t hash_stream[COMPOSITE_EAT_PAYLOAD_MAX + COMPOSITE_EAT_PROTECTED_MAX + 32];
static size_t hash_stream_length;

static const struct riot_keys *composite_eat_generator_test_get_keys (
	const struct riot_key_manager *riot)
{
	(void) riot;
	return &test_keys;
}

static void composite_eat_generator_test_release_keys (const struct riot_key_manager *riot,
	const struct riot_keys *keys)
{
	(void) riot;
	(void) keys;
	release_count++;
}

static const struct der_cert *composite_eat_generator_test_get_certificate (
	const struct riot_key_manager *riot)
{
	(void) riot;
	return NULL;
}

static int composite_eat_generator_test_get_pcr_count (struct pcr_store *store)
{
	(void) store;
	return 3;
}

static int composite_eat_generator_test_get_pcr_digest_length (struct pcr_store *store, uint8_t pcr)
{
	(void) store;
	(void) pcr;
	return pcr_digest_length;
}

static int composite_eat_generator_test_compute_pcr (struct pcr_store *store,
	const struct hash_engine *hash, uint8_t pcr, uint8_t *measurement, size_t length)
{
	(void) store;
	(void) hash;
	if (pcr_failure) {
		return -1;
	}
	memset (measurement, 0x80 + pcr, length);
	return (int) length;
}

static const struct composite_eat_attestation_ops test_attestation = {
	.get_keys = composite_eat_generator_test_get_keys,
	.release_keys = composite_eat_generator_test_release_keys,
	.get_intermediate_ca = composite_eat_generator_test_get_certificate,
	.get_root_ca = composite_eat_generator_test_get_certificate,
	.get_pcr_count = composite_eat_generator_test_get_pcr_count,
	.get_pcr_digest_length = composite_eat_generator_test_get_pcr_digest_length,
	.compute_pcr = composite_eat_generator_test_compute_pcr,
};

static int composite_eat_generator_test_hash_calculate (const struct hash_engine *engine,
	const uint8_t *data, size_t length, uint8_t *hash, size_t hash_length)
{
	(void) engine;
	(void) data;
	(void) length;
	if (hash_failure) {
		return -1;
	}
	memset (hash, 0xab, hash_length);
	return 0;
}

static int composite_eat_generator_test_hash_start (const struct hash_engine *engine)
{
	(void) engine;
	hash_stream_length = 0;
	return (hash_stream_failure == 1) ? -1 : 0;
}

static int composite_eat_generator_test_hash_update (const struct hash_engine *engine,
	const uint8_t *data, size_t length)
{
	(void) engine;
	if (hash_stream_failure == 2) {
		return -1;
	}
	memcpy (&hash_stream[hash_stream_length], data, length);
	hash_stream_length += length;
	return 0;
}

static int composite_eat_generator_test_hash_finish (const struct hash_engine *engine,
	uint8_t *hash, size_t hash_length)
{
	(void) engine;
	if (hash_stream_failure == 3) {
		return -1;
	}
	memset (hash, 0xcd, hash_length);
	return 0;
}

static void composite_eat_generator_test_hash_cancel (const struct hash_engine *engine)
{
	(void) engine;
	hash_cancel_count++;
}

static const struct hash_engine test_hash = {
	.calculate_sha384 = composite_eat_generator_test_hash_calculate,
	.start_sha384 = composite_eat_generator_test_hash_start,
	.update = composite_eat_generator_test_hash_update,
	.finish = composite_eat_generator_test_hash_finish,
	.cancel = composite_eat_generator_test_hash_cancel,
};

static int composite_eat_generator_test_ecc_init (const struct ecc_engine *engine,
	const uint8_t *key, size_t key_length, struct ecc_private_key *private_key,
	struct ecc_public_key *public_key)
{
	(void) engine;
	(void) key;
	(void) key_length;
	(void) public_key;
	memset (private_key, 0, sizeof (*private_key));
	return sign_failure ? -1 : 0;
}

static int composite_eat_generator_test_ecc_sign (const struct ecc_engine *engine,
	const struct ecc_private_key *key, const uint8_t *digest, size_t length,
	const struct rng_engine *rng, uint8_t *signature, size_t signature_length)
{
	uint8_t r[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t s[COMPOSITE_EAT_SHA384_LENGTH];

	(void) engine;
	(void) key;
	(void) digest;
	(void) length;
	(void) rng;
	if (sign_failure) {
		return -1;
	}
	memset (r, 0x11, sizeof (r));
	memset (s, 0x22, sizeof (s));
	return ecc_der_encode_ecdsa_signature (r, s, sizeof (r), signature, signature_length);
}

static void composite_eat_generator_test_ecc_release (const struct ecc_engine *engine,
	struct ecc_private_key *private_key, struct ecc_public_key *public_key)
{
	(void) engine;
	(void) private_key;
	(void) public_key;
}

static const struct ecc_engine test_ecc = {
	.init_key_pair = composite_eat_generator_test_ecc_init,
	.sign = composite_eat_generator_test_ecc_sign,
	.release_key_pair = composite_eat_generator_test_ecc_release,
};

static void composite_eat_generator_test_reset (void)
{
	memset (alias_certificate, 0xc1, sizeof (alias_certificate));
	memset (device_certificate, 0xd1, sizeof (device_certificate));
	memset (&test_keys, 0, sizeof (test_keys));
	test_keys.alias_key = alias_key;
	test_keys.alias_key_length = sizeof (alias_key);
	test_keys.alias_cert = alias_certificate;
	test_keys.alias_cert_length = sizeof (alias_certificate);
	test_keys.devid_cert = device_certificate;
	test_keys.devid_cert_length = sizeof (device_certificate);
	release_count = 0;
	hash_failure = 0;
	hash_stream_failure = 0;
	hash_cancel_count = 0;
	hash_stream_length = 0;
	sign_failure = 0;
	pcr_failure = 0;
	pcr_digest_length = SHA256_HASH_LENGTH;
}

static size_t composite_eat_generator_test_request (uint8_t *encoded, size_t capacity)
{
	struct composite_eat_request request;
	size_t length = 0;

	memset (&request, 0, sizeof (request));
	request.version = COMPOSITE_EAT_REQUEST_VERSION;
	request.nonce_length = 16;
	memset (request.nonce, 0x5a, request.nonce_length);
	request.record_count = 1;
	memcpy (request.records[0].environment, "device0", 7);
	request.records[0].environment_length = 7;
	memset (request.records[0].digest, 0x33, sizeof (request.records[0].digest));
	if (composite_eat_codec_encode_request (&request, encoded, capacity, &length) !=
		COMPOSITE_EAT_CODEC_OK) {
		return 0;
	}
	return length;
}

static size_t composite_eat_generator_test_bstr_header (size_t length, uint8_t header[3])
{
	if (length < 24) {
		header[0] = (uint8_t) (0x40 | length);
		return 1;
	}
	if (length < 0x100) {
		header[0] = 0x58;
		header[1] = (uint8_t) length;
		return 2;
	}
	header[0] = 0x59;
	header[1] = (uint8_t) (length >> 8);
	header[2] = (uint8_t) length;
	return 3;
}

static void composite_eat_generator_test_happy_path (CuTest *test)
{
	static struct composite_eat_generator_workspace workspace;
	struct composite_eat_generator generator;
	uint8_t request[256];
	uint8_t response[4096];
	size_t request_length;
	size_t response_length = 0;
	QCBORDecodeContext decoder;
	UsefulBufC protected_headers;
	UsefulBufC certificate;
	UsefulBufC payload;
	UsefulBufC signature;
	uint8_t expected_sig_structure[COMPOSITE_EAT_PAYLOAD_MAX + COMPOSITE_EAT_PROTECTED_MAX + 32];
	uint8_t bstr_header[3];
	size_t expected_length = 0;
	size_t header_length;
	enum composite_eat_generator_status status;

	TEST_START;
	composite_eat_generator_test_reset ();
	request_length = composite_eat_generator_test_request (request, sizeof (request));
	status = composite_eat_generator_init_with_attestation (&generator, &test_ecc, &test_hash,
		&test_riot, &test_pcr_store, &workspace, &test_attestation);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_OK, status);
	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_OK, status);
	CuAssertTrue (test, response_length > 0);
	CuAssertIntEquals (test, 1, release_count);
	CuAssertIntEquals (test, 0xd2, response[0]);

	QCBORDecode_Init (&decoder, (UsefulBufC){response + 1, response_length - 1},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetByteString (&decoder, &protected_headers);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterArrayFromMapN (&decoder, 33);
	QCBORDecode_GetByteString (&decoder, &certificate);
	CuAssertIntEquals (test, sizeof (alias_certificate), certificate.len);
	QCBORDecode_GetByteString (&decoder, &certificate);
	CuAssertIntEquals (test, sizeof (device_certificate), certificate.len);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_GetByteString (&decoder, &payload);
	QCBORDecode_GetByteString (&decoder, &signature);
	QCBORDecode_ExitArray (&decoder);
	CuAssertIntEquals (test, QCBOR_SUCCESS, QCBORDecode_Finish (&decoder));
	CuAssertIntEquals (test, COMPOSITE_EAT_ES384_SIGNATURE_LENGTH, signature.len);
	CuAssertIntEquals (test, 0x11, ((const uint8_t *) signature.ptr)[0]);
	CuAssertIntEquals (test, 0x22, ((const uint8_t *) signature.ptr)[COMPOSITE_EAT_SHA384_LENGTH]);
	CuAssertTrue (test, protected_headers.len > 0);
	CuAssertTrue (test, payload.len > 0);

	memcpy (&expected_sig_structure[expected_length],
		(const uint8_t[]){0x84, 0x6a, 'S', 'i', 'g', 'n', 'a', 't', 'u', 'r', 'e', '1'}, 12);
	expected_length += 12;
	header_length = composite_eat_generator_test_bstr_header (protected_headers.len, bstr_header);
	memcpy (&expected_sig_structure[expected_length], bstr_header, header_length);
	expected_length += header_length;
	memcpy (&expected_sig_structure[expected_length], protected_headers.ptr, protected_headers.len);
	expected_length += protected_headers.len;
	expected_sig_structure[expected_length++] = 0x40;
	header_length = composite_eat_generator_test_bstr_header (payload.len, bstr_header);
	memcpy (&expected_sig_structure[expected_length], bstr_header, header_length);
	expected_length += header_length;
	memcpy (&expected_sig_structure[expected_length], payload.ptr, payload.len);
	expected_length += payload.len;
	CuAssertIntEquals (test, expected_length, hash_stream_length);
	CuAssertIntEquals (test, 0, memcmp (expected_sig_structure, hash_stream, expected_length));
}

static void composite_eat_generator_test_buffer_too_small (CuTest *test)
{
	static struct composite_eat_generator_workspace workspace;
	struct composite_eat_generator generator;
	uint8_t request[256];
	uint8_t response[8];
	uint8_t untouched[8];
	size_t response_length = 0;
	enum composite_eat_generator_status status;

	TEST_START;
	composite_eat_generator_test_reset ();
	memset (response, 0xa5, sizeof (response));
	memset (untouched, 0xa5, sizeof (untouched));
	composite_eat_generator_init_with_attestation (&generator, &test_ecc, &test_hash, &test_riot,
		&test_pcr_store, &workspace, &test_attestation);
	status = composite_eat_generate (&generator, request,
		composite_eat_generator_test_request (request, sizeof (request)), response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_BUFFER_TOO_SMALL, status);
	CuAssertTrue (test, response_length > sizeof (response));
	CuAssertIntEquals (test, 0, memcmp (response, untouched, sizeof (response)));
	CuAssertIntEquals (test, 1, release_count);
}

static void composite_eat_generator_test_pcr_evidence_algorithms (CuTest *test)
{
	static struct composite_eat_generator_workspace workspace;
	struct composite_eat_generator generator;
	uint8_t request[256];
	uint8_t response[4096];
	size_t request_length;
	size_t response_length;
	QCBORDecodeContext decoder;
	UsefulBufC payload;
	UsefulBufC evidence;
	UsefulBufC digest;
	uint64_t content_format;
	int64_t algorithm;
	enum composite_eat_generator_status status;

	TEST_START;
	composite_eat_generator_test_reset ();
	request_length = composite_eat_generator_test_request (request, sizeof (request));
	composite_eat_generator_init_with_attestation (&generator, &test_ecc, &test_hash, &test_riot,
		&test_pcr_store, &workspace, &test_attestation);

	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_OK, status);
	QCBORDecode_Init (&decoder, (UsefulBufC){response + 1, response_length - 1},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetByteString (&decoder, &payload);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_GetByteString (&decoder, &payload);
	QCBORDecode_Init (&decoder, payload, QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterArrayFromMapN (&decoder, 273);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetUInt64 (&decoder, &content_format);
	QCBORDecode_GetByteString (&decoder, &evidence);
	QCBORDecode_Init (&decoder, evidence, QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterMapFromMapN (&decoder, 0);
	QCBORDecode_EnterArrayFromMapN (&decoder, 0);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterMapFromMapN (&decoder, 0);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterMapFromMapN (&decoder, 1);
	QCBORDecode_EnterArrayFromMapN (&decoder, 2);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetInt64 (&decoder, &algorithm);
	QCBORDecode_GetByteString (&decoder, &digest);
	CuAssertIntEquals (test, 1, algorithm);
	CuAssertIntEquals (test, SHA256_HASH_LENGTH, digest.len);
	CuAssertIntEquals (test, 0x80, ((const uint8_t *) digest.ptr)[0]);

	pcr_digest_length = COMPOSITE_EAT_SHA384_LENGTH;
	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_OK, status);
	QCBORDecode_Init (&decoder, (UsefulBufC){response + 1, response_length - 1},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetByteString (&decoder, &payload);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_GetByteString (&decoder, &payload);
	QCBORDecode_Init (&decoder, payload, QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterArrayFromMapN (&decoder, 273);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetUInt64 (&decoder, &content_format);
	QCBORDecode_GetByteString (&decoder, &evidence);
	QCBORDecode_Init (&decoder, evidence, QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterMapFromMapN (&decoder, 0);
	QCBORDecode_EnterArrayFromMapN (&decoder, 0);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterMapFromMapN (&decoder, 0);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterMapFromMapN (&decoder, 1);
	QCBORDecode_EnterArrayFromMapN (&decoder, 2);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetInt64 (&decoder, &algorithm);
	QCBORDecode_GetByteString (&decoder, &digest);
	CuAssertIntEquals (test, 7, algorithm);
	CuAssertIntEquals (test, COMPOSITE_EAT_SHA384_LENGTH, digest.len);
}

static void composite_eat_generator_test_failures (CuTest *test)
{
	static struct composite_eat_generator_workspace workspace;
	struct composite_eat_generator generator;
	uint8_t request[256];
	uint8_t response[4096];
	size_t request_length;
	size_t response_length;
	enum composite_eat_generator_status status;

	TEST_START;
	composite_eat_generator_test_reset ();
	request_length = composite_eat_generator_test_request (request, sizeof (request));
	composite_eat_generator_init_with_attestation (&generator, &test_ecc, &test_hash, &test_riot,
		&test_pcr_store, &workspace, &test_attestation);

	response_length = 9;
	status = composite_eat_generate (&generator, request, request_length - 1, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_BAD_REQUEST, status);
	CuAssertIntEquals (test, 0, response_length);
	CuAssertIntEquals (test, 0, release_count);

	hash_failure = 1;
	response_length = 9;
	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_HASH_ERROR, status);
	CuAssertIntEquals (test, 0, response_length);
	CuAssertIntEquals (test, 1, release_count);

	hash_failure = 0;
	sign_failure = 1;
	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_SIGN_ERROR, status);
	CuAssertIntEquals (test, 2, release_count);

	sign_failure = 0;
	pcr_failure = 1;
	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_ATTESTATION_ERROR, status);
	CuAssertIntEquals (test, 3, release_count);
}

static void composite_eat_generator_test_hash_cleanup (CuTest *test)
{
	static struct composite_eat_generator_workspace workspace;
	struct composite_eat_generator generator;
	uint8_t request[256];
	uint8_t response[4096];
	size_t request_length;
	size_t response_length;
	enum composite_eat_generator_status status;

	TEST_START;
	composite_eat_generator_test_reset ();
	request_length = composite_eat_generator_test_request (request, sizeof (request));
	composite_eat_generator_init_with_attestation (&generator, &test_ecc, &test_hash, &test_riot,
		&test_pcr_store, &workspace, &test_attestation);

	hash_stream_failure = 1;
	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_HASH_ERROR, status);
	CuAssertIntEquals (test, 0, hash_cancel_count);

	hash_stream_failure = 2;
	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_HASH_ERROR, status);
	CuAssertIntEquals (test, 1, hash_cancel_count);

	hash_stream_failure = 3;
	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_HASH_ERROR, status);
	CuAssertIntEquals (test, 2, hash_cancel_count);
}

static void composite_eat_generator_test_init_and_busy (CuTest *test)
{
	static struct composite_eat_generator_workspace workspace;
	struct composite_eat_generator generator;
	uint8_t request[256];
	uint8_t response[8];
	size_t response_length = 9;
	enum composite_eat_generator_status status;

	TEST_START;
	composite_eat_generator_test_reset ();
	status = composite_eat_generator_init_with_attestation (&generator, &test_ecc, &test_hash,
		&test_riot, NULL, &workspace, &test_attestation);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_BAD_ARGUMENT, status);
	status = composite_eat_generator_init_with_attestation (&generator, &test_ecc, &test_hash,
		&test_riot, &test_pcr_store, &workspace, &test_attestation);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_OK, status);
	workspace.active = true;
	status = composite_eat_generate (&generator, request,
		composite_eat_generator_test_request (request, sizeof (request)), response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_BUSY, status);
	CuAssertIntEquals (test, 0, response_length);
	CuAssertTrue (test, workspace.active);
}


TEST_SUITE_START (composite_eat_generator);

TEST (composite_eat_generator_test_happy_path);
TEST (composite_eat_generator_test_buffer_too_small);
TEST (composite_eat_generator_test_pcr_evidence_algorithms);
TEST (composite_eat_generator_test_failures);
TEST (composite_eat_generator_test_hash_cleanup);
TEST (composite_eat_generator_test_init_and_busy);

TEST_SUITE_END;