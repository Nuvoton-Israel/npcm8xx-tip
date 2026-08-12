/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <openssl/evp.h>

#include "testing.h"
#include "asn1/ecc_der_util.h"
#include "crypto/ecc_openssl.h"
#include "crypto/hash_openssl.h"
#include "composite_eat/composite_eat_generator.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_spiffy_decode.h"


TEST_SUITE_LABEL ("composite_eat_generator");

static const uint8_t test_ecc384_public_key[] = {
    0x30, 0x76, 0x30, 0x10, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x05,
    0x2b, 0x81, 0x04, 0x00, 0x22, 0x03, 0x62, 0x00, 0x04, 0xcf, 0x6b, 0x8d, 0x9a, 0x48, 0x75,
    0xa9, 0x5a, 0x19, 0x89, 0x72, 0x18, 0xa4, 0x94, 0x4d, 0xef, 0x0a, 0x93, 0xce, 0x5b, 0x8b,
    0x8d, 0xf1, 0x37, 0x54, 0x09, 0x17, 0x89, 0xbc, 0xef, 0x69, 0xdb, 0x6c, 0xa7, 0x9e, 0xf6,
    0xb6, 0x4b, 0x5c, 0x13, 0xed, 0x3c, 0xbf, 0xed, 0x0b, 0x3d, 0xf1, 0x7e, 0x53, 0xbf, 0xf4,
    0x76, 0x31, 0x31, 0x33, 0xa3, 0x58, 0x3c, 0x11, 0x3d, 0xeb, 0x8d, 0xb6, 0xb7, 0x47, 0x4a,
    0xe3, 0x51, 0xd0, 0x38, 0x26, 0xac, 0xec, 0x11, 0x34, 0x33, 0x04, 0x0d, 0xc6, 0xc3, 0x75,
    0x37, 0xa1, 0x89, 0xdd, 0x4f, 0x66, 0x57, 0x72, 0xac, 0xc5, 0x3b, 0xb6, 0xc6, 0xb8, 0x0c,
};

static const uint8_t test_ecc384_private_key[] = {
    0x30, 0x81, 0xa4, 0x02, 0x01, 0x01, 0x04, 0x30, 0x5d, 0x57, 0x6f, 0x7d, 0xd2, 0x90, 0xd7, 0x94, 0x53,
    0x02, 0x74, 0x38, 0xb3, 0xe6, 0x81, 0xa1, 0x10, 0xc5, 0xd0, 0x95, 0x31, 0x3d, 0xcb, 0x69, 0x78, 0x34,
    0x41, 0x62, 0x2f, 0x87, 0xc7, 0x1e, 0x12, 0x80, 0x4c, 0xb0, 0x97, 0x2e, 0x63, 0xbd, 0xff, 0x8a, 0x25,
    0x6b, 0x63, 0xd4, 0x90, 0x8d, 0xa0, 0x07, 0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x22, 0xa1, 0x64, 0x03,
    0x62, 0x00, 0x04, 0xcf, 0x6b, 0x8d, 0x9a, 0x48, 0x75, 0xa9, 0x5a, 0x19, 0x89, 0x72, 0x18, 0xa4, 0x94,
    0x4d, 0xef, 0x0a, 0x93, 0xce, 0x5b, 0x8b, 0x8d, 0xf1, 0x37, 0x54, 0x09, 0x17, 0x89, 0xbc, 0xef, 0x69,
    0xdb, 0x6c, 0xa7, 0x9e, 0xf6, 0xb6, 0x4b, 0x5c, 0x13, 0xed, 0x3c, 0xbf, 0xed, 0x0b, 0x3d, 0xf1, 0x7e,
    0x53, 0xbf, 0xf4, 0x76, 0x31, 0x31, 0x33, 0xa3, 0x58, 0x3c, 0x11, 0x3d, 0xeb, 0x8d, 0xb6, 0xb7, 0x47,
    0x4a, 0xe3, 0x51, 0xd0, 0x38, 0x26, 0xac, 0xec, 0x11, 0x34, 0x33, 0x04, 0x0d, 0xc6, 0xc3, 0x75, 0x37,
    0xa1, 0x89, 0xdd, 0x4f, 0x66, 0x57, 0x72, 0xac, 0xc5, 0x3b, 0xb6, 0xc6, 0xb8, 0x0c,
};

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

static void composite_eat_generator_test_es384_interoperability (CuTest *test)
{
	static struct composite_eat_generator_workspace workspace;
	struct composite_eat_generator generator;
	struct ecc_engine_openssl ecc;
	struct hash_engine_openssl hash;
	struct hash_engine_openssl_state hash_state;
	struct ecc_public_key public_key;
	uint8_t request[256];
	uint8_t response[4096];
	uint8_t sig_structure[COMPOSITE_EAT_PAYLOAD_MAX + COMPOSITE_EAT_PROTECTED_MAX + 32];
	uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t der_signature[ECC_DER_ECDSA_MAX_LENGTH];
	uint8_t bstr_header[3];
	size_t request_length;
	size_t response_length;
	size_t sig_structure_length = 0;
	size_t header_length;
	unsigned int digest_length = 0;
	int der_length;
	int verify_status;
	QCBORDecodeContext decoder;
	UsefulBufC protected_headers;
	UsefulBufC payload;
	UsefulBufC signature;
	enum composite_eat_generator_status status;

	TEST_START;
	composite_eat_generator_test_reset ();
	test_keys.alias_key = test_ecc384_private_key;
	test_keys.alias_key_length = sizeof (test_ecc384_private_key);
	request_length = composite_eat_generator_test_request (request, sizeof (request));
	CuAssertIntEquals (test, 0, ecc_openssl_init (&ecc));
	CuAssertIntEquals (test, 0, hash_openssl_init (&hash, &hash_state));
	status = composite_eat_generator_init_with_attestation (&generator, &ecc.base, &hash.base,
		&test_riot, &test_pcr_store, &workspace, &test_attestation);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_OK, status);
	status = composite_eat_generate (&generator, request, request_length, response,
		sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATOR_OK, status);

	QCBORDecode_Init (&decoder, (UsefulBufC){response + 1, response_length - 1},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetByteString (&decoder, &protected_headers);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_GetByteString (&decoder, &payload);
	QCBORDecode_GetByteString (&decoder, &signature);
	QCBORDecode_ExitArray (&decoder);
	CuAssertIntEquals (test, QCBOR_SUCCESS, QCBORDecode_Finish (&decoder));

	memcpy (&sig_structure[sig_structure_length],
		(const uint8_t[]){0x84, 0x6a, 'S', 'i', 'g', 'n', 'a', 't', 'u', 'r', 'e', '1'}, 12);
	sig_structure_length += 12;
	header_length = composite_eat_generator_test_bstr_header (protected_headers.len, bstr_header);
	memcpy (&sig_structure[sig_structure_length], bstr_header, header_length);
	sig_structure_length += header_length;
	memcpy (&sig_structure[sig_structure_length], protected_headers.ptr, protected_headers.len);
	sig_structure_length += protected_headers.len;
	sig_structure[sig_structure_length++] = 0x40;
	header_length = composite_eat_generator_test_bstr_header (payload.len, bstr_header);
	memcpy (&sig_structure[sig_structure_length], bstr_header, header_length);
	sig_structure_length += header_length;
	memcpy (&sig_structure[sig_structure_length], payload.ptr, payload.len);
	sig_structure_length += payload.len;
	CuAssertIntEquals (test, 1,
		EVP_Digest (sig_structure, sig_structure_length, digest, &digest_length, EVP_sha384 (),
			NULL));
	CuAssertIntEquals (test, COMPOSITE_EAT_SHA384_LENGTH, digest_length);

	der_length = ecc_der_encode_ecdsa_signature (signature.ptr,
		&((const uint8_t *) signature.ptr)[COMPOSITE_EAT_SHA384_LENGTH],
		COMPOSITE_EAT_SHA384_LENGTH, der_signature, sizeof (der_signature));
	CuAssertTrue (test, der_length > 0);
	CuAssertIntEquals (test, 0,
		ecc.base.init_public_key (&ecc.base, test_ecc384_public_key,
			sizeof (test_ecc384_public_key), &public_key));
	verify_status = ecc.base.verify (&ecc.base, &public_key, digest, sizeof (digest), der_signature,
		(size_t) der_length);
	CuAssertIntEquals (test, 0, verify_status);
	ecc.base.release_key_pair (&ecc.base, NULL, &public_key);
	hash_openssl_release (&hash);
	ecc_openssl_release (&ecc);
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
TEST (composite_eat_generator_test_es384_interoperability);
TEST (composite_eat_generator_test_failures);
TEST (composite_eat_generator_test_hash_cleanup);
TEST (composite_eat_generator_test_init_and_busy);

TEST_SUITE_END;