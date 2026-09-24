/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include <openssl/evp.h>

#include "testing.h"
#include "asn1/ecc_der_util.h"
#include "crypto/ecc_openssl.h"
#include "crypto/hash_openssl.h"
#include "composite_eat/tip_main_token_generator.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_encode.h"
#include "qcbor/qcbor_spiffy_decode.h"


TEST_SUITE_LABEL ("composite_eat_tip_main_token_generator");

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
	0x30, 0x81, 0xa4, 0x02, 0x01, 0x01, 0x04, 0x30, 0x5d, 0x57, 0x6f, 0x7d, 0xd2, 0x90, 0xd7,
	0x94, 0x53, 0x02, 0x74, 0x38, 0xb3, 0xe6, 0x81, 0xa1, 0x10, 0xc5, 0xd0, 0x95, 0x31, 0x3d,
	0xcb, 0x69, 0x78, 0x34, 0x41, 0x62, 0x2f, 0x87, 0xc7, 0x1e, 0x12, 0x80, 0x4c, 0xb0, 0x97,
	0x2e, 0x63, 0xbd, 0xff, 0x8a, 0x25, 0x6b, 0x63, 0xd4, 0x90, 0x8d, 0xa0, 0x07, 0x06, 0x05,
	0x2b, 0x81, 0x04, 0x00, 0x22, 0xa1, 0x64, 0x03, 0x62, 0x00, 0x04, 0xcf, 0x6b, 0x8d, 0x9a,
	0x48, 0x75, 0xa9, 0x5a, 0x19, 0x89, 0x72, 0x18, 0xa4, 0x94, 0x4d, 0xef, 0x0a, 0x93, 0xce,
	0x5b, 0x8b, 0x8d, 0xf1, 0x37, 0x54, 0x09, 0x17, 0x89, 0xbc, 0xef, 0x69, 0xdb, 0x6c, 0xa7,
	0x9e, 0xf6, 0xb6, 0x4b, 0x5c, 0x13, 0xed, 0x3c, 0xbf, 0xed, 0x0b, 0x3d, 0xf1, 0x7e, 0x53,
	0xbf, 0xf4, 0x76, 0x31, 0x31, 0x33, 0xa3, 0x58, 0x3c, 0x11, 0x3d, 0xeb, 0x8d, 0xb6, 0xb7,
	0x47, 0x4a, 0xe3, 0x51, 0xd0, 0x38, 0x26, 0xac, 0xec, 0x11, 0x34, 0x33, 0x04, 0x0d, 0xc6,
	0xc3, 0x75, 0x37, 0xa1, 0x89, 0xdd, 0x4f, 0x66, 0x57, 0x72, 0xac, 0xc5, 0x3b, 0xb6, 0xc6,
	0xb8, 0x0c,
};

static uint8_t test_alias_key[] = {0xa0, 0xa1, 0xa2, 0xa3};
static uint8_t test_alias_certificate[COMPOSITE_EAT_MAX_CERTIFICATE_LENGTH];
static uint8_t test_device_certificate[COMPOSITE_EAT_MAX_CERTIFICATE_LENGTH];
static uint8_t test_ca_certificate[COMPOSITE_EAT_MAX_CERTIFICATE_LENGTH];
static struct der_cert test_ca;
static struct riot_keys test_keys;
static bool test_include_ca;
static int test_release_count;
static int test_hash_failure;
static int test_hash_cancel_count;
static int test_hash_update_count;
static int test_sign_failure;
static int test_output_write_count;


static bool composite_eat_tip_main_token_test_contains (const uint8_t *data, size_t length,
	const uint8_t *value, size_t value_length)
{
	size_t offset;

	if ((data == NULL) || (value == NULL) || (value_length == 0) || (length < value_length)) {
		return false;
	}
	for (offset = 0; offset <= (length - value_length); ++offset) {
		if (memcmp (&data[offset], value, value_length) == 0) {
			return true;
		}
	}
	return false;
}

static const struct riot_keys *composite_eat_tip_main_token_test_get_keys (
	const struct riot_key_manager *riot)
{
	(void) riot;
	return &test_keys;
}

static void composite_eat_tip_main_token_test_release_keys (
	const struct riot_key_manager *riot, const struct riot_keys *keys)
{
	(void) riot;
	(void) keys;
	test_release_count++;
}

static const struct der_cert *composite_eat_tip_main_token_test_get_ca (
	const struct riot_key_manager *riot)
{
	(void) riot;
	return test_include_ca ? &test_ca : NULL;
}

static const struct composite_eat_tip_main_token_ops test_main_token_ops = {
	.get_keys = composite_eat_tip_main_token_test_get_keys,
	.release_keys = composite_eat_tip_main_token_test_release_keys,
	.get_intermediate_ca = composite_eat_tip_main_token_test_get_ca,
	.get_root_ca = composite_eat_tip_main_token_test_get_ca,
};

static int composite_eat_tip_main_token_test_cancel_output (void *context,
	const uint8_t *data, size_t length)
{
	(void) context;
	(void) data;
	(void) length;
	test_output_write_count++;
	return (test_output_write_count > 1) ? -1 : 0;
}

static int composite_eat_tip_main_token_test_hash_calculate (const struct hash_engine *engine,
	const uint8_t *data, size_t length, uint8_t *hash, size_t hash_length)
{
	(void) engine;
	(void) data;
	(void) length;
	if (test_hash_failure == 4) {
		return -1;
	}
	memset (hash, 0xab, hash_length);
	return 0;
}

static int composite_eat_tip_main_token_test_hash_start (const struct hash_engine *engine)
{
	(void) engine;
	test_hash_update_count = 0;
	return (test_hash_failure == 1) ? -1 : 0;
}

static int composite_eat_tip_main_token_test_hash_update (const struct hash_engine *engine,
	const uint8_t *data, size_t length)
{
	(void) engine;
	(void) data;
	(void) length;
	test_hash_update_count++;
	return (test_hash_failure == 2) ? -1 : 0;
}

static int composite_eat_tip_main_token_test_hash_finish (const struct hash_engine *engine,
	uint8_t *hash, size_t hash_length)
{
	(void) engine;
	if (test_hash_failure == 3) {
		return -1;
	}
	memset (hash, 0xcd, hash_length);
	return 0;
}

static void composite_eat_tip_main_token_test_hash_cancel (const struct hash_engine *engine)
{
	(void) engine;
	test_hash_cancel_count++;
}

static const struct hash_engine test_hash = {
	.calculate_sha384 = composite_eat_tip_main_token_test_hash_calculate,
	.start_sha384 = composite_eat_tip_main_token_test_hash_start,
	.update = composite_eat_tip_main_token_test_hash_update,
	.finish = composite_eat_tip_main_token_test_hash_finish,
	.cancel = composite_eat_tip_main_token_test_hash_cancel,
};

static int composite_eat_tip_main_token_test_ecc_init (const struct ecc_engine *engine,
	const uint8_t *key, size_t key_length, struct ecc_private_key *private_key,
	struct ecc_public_key *public_key)
{
	(void) engine;
	(void) key;
	(void) key_length;
	(void) public_key;
	memset (private_key, 0, sizeof (*private_key));
	return test_sign_failure ? -1 : 0;
}

static int composite_eat_tip_main_token_test_ecc_sign (const struct ecc_engine *engine,
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
	if (test_sign_failure) {
		return -1;
	}
	memset (r, 0x11, sizeof (r));
	memset (s, 0x22, sizeof (s));
	return ecc_der_encode_ecdsa_signature (r, s, sizeof (r), signature, signature_length);
}

static void composite_eat_tip_main_token_test_ecc_release (const struct ecc_engine *engine,
	struct ecc_private_key *private_key, struct ecc_public_key *public_key)
{
	(void) engine;
	(void) private_key;
	(void) public_key;
}

static const struct ecc_engine test_ecc = {
	.init_key_pair = composite_eat_tip_main_token_test_ecc_init,
	.sign = composite_eat_tip_main_token_test_ecc_sign,
	.release_key_pair = composite_eat_tip_main_token_test_ecc_release,
};

static int test_pcr_count (struct pcr_store *store)
{
	(void) store;
	return 1;
}

static int test_pcr_length (struct pcr_store *store, uint8_t pcr)
{
	(void) store;
	(void) pcr;
	return SHA384_HASH_LENGTH;
}

static int test_pcr_compute (struct pcr_store *store, const struct hash_engine *hash,
	uint8_t pcr, uint8_t *measurement, size_t length)
{
	(void) store;
	(void) hash;
	(void) pcr;
	memset (measurement, 0x55, length);
	return (int) length;
}

static int test_pcr_begin_snapshot (struct pcr_store *store)
{
	(void) store;
	return 0;
}

static int test_pcr_end_snapshot (struct pcr_store *store)
{
	(void) store;
	return 0;
}

static int test_dme_pcr0 (uint8_t *measurement, size_t length)
{
	if ((measurement == NULL) || (length != TCG_CONCISE_EVIDENCE_SHA512_LENGTH)) {
		return -1;
	}
	memset (measurement, 0x5a, length);
	return (int) length;
}

static const struct composite_eat_tip_evidence_ops test_evidence_ops = {
	.get_pcr_count = test_pcr_count,
	.get_pcr_digest_length = test_pcr_length,
	.begin_snapshot = test_pcr_begin_snapshot,
	.end_snapshot = test_pcr_end_snapshot,
	.compute_pcr = test_pcr_compute,
	.get_dme_pcr0 = test_dme_pcr0,
};

static size_t composite_eat_tip_main_token_test_request (uint8_t *encoded, size_t capacity)
{
	QCBOREncodeContext encoder;
	UsefulBufC output;
	uint8_t nonce[COMPOSITE_EAT_NONCE_LENGTH] = {0};
	uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH] = {0};
	static const char environment[] = "env.device.0";

	QCBOREncode_Init (&encoder, (UsefulBuf){encoded, capacity});
	QCBOREncode_OpenMap (&encoder);
	QCBOREncode_AddInt64ToMapN (&encoder, 1, COMPOSITE_EAT_GENERATION_REQUEST_VERSION);
	QCBOREncode_AddBytesToMapN (&encoder, 2, (UsefulBufC){nonce, sizeof (nonce)});
	QCBOREncode_OpenArrayInMapN (&encoder, 3);
	QCBOREncode_OpenMap (&encoder);
	QCBOREncode_AddSZStringToMapN (&encoder, 1, environment);
	QCBOREncode_OpenArrayInMapN (&encoder, 2);
	QCBOREncode_AddInt64 (&encoder, COMPOSITE_EAT_COSE_ALGORITHM_SHA384);
	QCBOREncode_AddBytes (&encoder, (UsefulBufC){digest, sizeof (digest)});
	QCBOREncode_CloseArray (&encoder);
	QCBOREncode_CloseMap (&encoder);
	QCBOREncode_CloseArray (&encoder);
	QCBOREncode_CloseMap (&encoder);
	return (QCBOREncode_Finish (&encoder, &output) == QCBOR_SUCCESS) ? output.len : 0;
}

static void composite_eat_tip_main_token_test_reset (void)
{
	memset (test_alias_certificate, 0xc1, sizeof (test_alias_certificate));
	memset (test_device_certificate, 0xd1, sizeof (test_device_certificate));
	memset (&test_keys, 0, sizeof (test_keys));
	test_keys.alias_key = test_alias_key;
	test_keys.alias_key_length = sizeof (test_alias_key);
	test_keys.alias_cert = test_alias_certificate;
	test_keys.alias_cert_length = 64;
	test_keys.devid_cert = test_device_certificate;
	test_keys.devid_cert_length = 80;
	memset (test_ca_certificate, 0xe1, sizeof (test_ca_certificate));
	test_ca.cert = test_ca_certificate;
	test_ca.length = sizeof (test_ca_certificate);
	test_include_ca = false;
	test_release_count = 0;
	test_hash_failure = 0;
	test_hash_cancel_count = 0;
	test_hash_update_count = 0;
	test_sign_failure = 0;
	test_output_write_count = 0;
}

static struct tcg_concise_evidence_environment composite_eat_tip_main_token_test_environment (
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

static void composite_eat_tip_main_token_test_generate (CuTest *test)
{
	static struct composite_eat_tip_main_token_workspace workspace;
	static const uint8_t class_id[] = {0x01, 0x02, 0x03, 0x04};
	static const uint8_t vendor[] = "Example Vendor";
	static const uint8_t model[] = "Example Model";
	static const uint8_t profile[] = "https://example.org/composite-eat/profile/v1";
	const struct tcg_concise_evidence_environment environment =
		composite_eat_tip_main_token_test_environment (class_id, sizeof (class_id), vendor,
			sizeof (vendor) - 1, model, sizeof (model) - 1);
	const struct composite_eat_buffer profile_buffer = {profile, sizeof (profile) - 1};
	struct composite_eat_tip_evidence_adapter evidence_adapter;
	struct composite_eat_tip_main_token_generator adapter;
	struct pcr_store store;
	struct riot_key_manager riot;
	uint8_t request[128];
	uint8_t response[2048];
	size_t request_length;
	size_t response_length;
	enum composite_eat_tip_main_token_status status;

	TEST_START;
	composite_eat_tip_main_token_test_reset ();
	composite_eat_tip_evidence_adapter_init_with_ops (&evidence_adapter, &store, &test_hash,
		&environment, &test_evidence_ops);
	status = composite_eat_tip_main_token_generator_init_with_ops (&adapter, &test_ecc,
		&test_hash, &riot, &evidence_adapter, &profile_buffer, &workspace,
		&test_main_token_ops);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_OK, status);
	request_length = composite_eat_tip_main_token_test_request (request, sizeof (request));
	status = composite_eat_tip_main_token_generate (&adapter, request, request_length,
		response, sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_OK, status);
	CuAssertTrue (test, response_length > 0);
	CuAssertIntEquals (test, 0xd8, response[0]);
	CuAssertIntEquals (test, 0x3d, response[1]);
	CuAssertIntEquals (test, 0xd2, response[2]);
	CuAssertTrue (test, test_hash_update_count > 6);
	CuAssertIntEquals (test, 1, test_release_count);
	CuAssertTrue (test, !workspace.active);
}

static void composite_eat_tip_main_token_test_failures (CuTest *test)
{
	static struct composite_eat_tip_main_token_workspace workspace;
	static const uint8_t class_id[] = {0x01};
	static const uint8_t vendor[] = "Vendor";
	static const uint8_t model[] = "Model";
	static const uint8_t profile[] = "https://example.org/composite-eat/profile/v1";
	const struct tcg_concise_evidence_environment environment =
		composite_eat_tip_main_token_test_environment (class_id, sizeof (class_id), vendor,
			sizeof (vendor) - 1, model, sizeof (model) - 1);
	const struct composite_eat_buffer profile_buffer = {profile, sizeof (profile) - 1};
	struct composite_eat_tip_evidence_adapter evidence_adapter;
	struct composite_eat_tip_main_token_generator adapter;
	struct pcr_store store;
	struct riot_key_manager riot;
	uint8_t request[128];
	uint8_t response[8];
	uint8_t untouched[8];
	static uint8_t full_response[2048];
	const uint8_t *snapshot;
	size_t request_length;
	size_t response_length;
	enum composite_eat_tip_main_token_status status;

	TEST_START;
	composite_eat_tip_main_token_test_reset ();
	composite_eat_tip_evidence_adapter_init_with_ops (&evidence_adapter, &store, &test_hash,
		&environment, &test_evidence_ops);
	composite_eat_tip_main_token_generator_init_with_ops (&adapter, &test_ecc, &test_hash,
		&riot, &evidence_adapter, &profile_buffer, &workspace, &test_main_token_ops);
	request_length = composite_eat_tip_main_token_test_request (request, sizeof (request));
	status = composite_eat_tip_main_token_snapshot_request (&adapter, request, request_length,
		&snapshot);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_OK, status);
	CuAssertPtrEquals (test, workspace.request_snapshot, (void *) snapshot);
	CuAssertIntEquals (test, 0, memcmp (request, snapshot, request_length));

	memset (response, 0xa5, sizeof (response));
	memcpy (untouched, response, sizeof (response));
	status = composite_eat_tip_main_token_generate (&adapter, snapshot, request_length,
		response, sizeof (response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_BUFFER_TOO_SMALL, status);
	CuAssertTrue (test, response_length > sizeof (response));
	CuAssertIntEquals (test, 0, memcmp (response, untouched, sizeof (response)));
	CuAssertIntEquals (test, 0, test_hash_update_count);

	composite_eat_tip_main_token_test_reset ();
	test_hash_failure = 2;
	status = composite_eat_tip_main_token_generate (&adapter, request, request_length,
		full_response, sizeof (full_response), &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_HASH_ERROR, status);
	CuAssertIntEquals (test, 1, test_hash_cancel_count);
	CuAssertIntEquals (test, 1, test_release_count);

	workspace.active = true;
	status = composite_eat_tip_main_token_generate (&adapter, request, request_length,
		NULL, 0, &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_BUSY, status);
	workspace.active = false;
	status = composite_eat_tip_main_token_generate (&adapter, request, request_length - 1,
		NULL, 0, &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_REQUEST, status);
}

static void composite_eat_tip_main_token_test_response_exceeds_platform_limit (CuTest *test)
{
	static struct composite_eat_tip_main_token_workspace workspace;
	static const uint8_t class_id[] = {0x01};
	static const uint8_t vendor[] = "Vendor";
	static const uint8_t model[] = "Model";
	static const uint8_t profile[] = "https://example.org/composite-eat/profile/v1";
	const struct tcg_concise_evidence_environment environment =
		composite_eat_tip_main_token_test_environment (class_id, sizeof (class_id), vendor,
			sizeof (vendor) - 1, model, sizeof (model) - 1);
	const struct composite_eat_buffer profile_buffer = {profile, sizeof (profile) - 1};
	struct composite_eat_tip_evidence_adapter evidence_adapter;
	struct composite_eat_tip_main_token_generator generator;
	struct pcr_store store;
	struct riot_key_manager riot;
	uint8_t request[128];
	static uint8_t response[COMPOSITE_EAT_TIP_MAX_RESPONSE_LENGTH];
	size_t request_length;
	size_t response_length;
	enum composite_eat_tip_main_token_status status;

	TEST_START;
	composite_eat_tip_main_token_test_reset ();
	test_keys.alias_cert_length = sizeof (test_alias_certificate);
	test_keys.devid_cert_length = sizeof (test_device_certificate);
	test_include_ca = true;
	composite_eat_tip_evidence_adapter_init_with_ops (&evidence_adapter, &store, &test_hash,
		&environment, &test_evidence_ops);
	composite_eat_tip_main_token_generator_init_with_ops (&generator, &test_ecc, &test_hash,
		&riot, &evidence_adapter, &profile_buffer, &workspace, &test_main_token_ops);
	request_length = composite_eat_tip_main_token_test_request (request, sizeof (request));
	status = composite_eat_tip_main_token_generate (&generator, request, request_length,
		response, sizeof (response), &response_length);

	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_RESPONSE_TOO_LARGE, status);
	CuAssertTrue (test, response_length > COMPOSITE_EAT_TIP_MAX_RESPONSE_LENGTH);
	CuAssertIntEquals (test, 0, test_hash_update_count);
	CuAssertIntEquals (test, 1, test_release_count);
	CuAssertTrue (test, !workspace.active);
}

static void composite_eat_tip_main_token_test_output_canceled (CuTest *test)
{
	static struct composite_eat_tip_main_token_workspace workspace;
	static const uint8_t class_id[] = {0x01};
	static const uint8_t vendor[] = "Vendor";
	static const uint8_t model[] = "Model";
	static const uint8_t profile[] = "https://example.org/composite-eat/profile/v1";
	const struct tcg_concise_evidence_environment environment =
		composite_eat_tip_main_token_test_environment (class_id, sizeof (class_id), vendor,
			sizeof (vendor) - 1, model, sizeof (model) - 1);
	const struct composite_eat_buffer profile_buffer = {profile, sizeof (profile) - 1};
	struct composite_eat_tip_evidence_adapter evidence_adapter;
	struct composite_eat_tip_main_token_generator generator;
	struct pcr_store store;
	struct riot_key_manager riot;
	uint8_t request[128];
	size_t request_length;
	size_t response_length;
	enum composite_eat_tip_main_token_status status;

	TEST_START;
	composite_eat_tip_main_token_test_reset ();
	composite_eat_tip_evidence_adapter_init_with_ops (&evidence_adapter, &store, &test_hash,
		&environment, &test_evidence_ops);
	composite_eat_tip_main_token_generator_init_with_ops (&generator, &test_ecc, &test_hash,
		&riot, &evidence_adapter, &profile_buffer, &workspace, &test_main_token_ops);
	request_length = composite_eat_tip_main_token_test_request (request, sizeof (request));
	status = composite_eat_tip_main_token_generate_write (&generator, request, request_length,
		composite_eat_tip_main_token_test_cancel_output, NULL,
		COMPOSITE_EAT_TIP_MAX_RESPONSE_LENGTH, &response_length);

	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_CANCELED, status);
	CuAssertIntEquals (test, 0, response_length);
	CuAssertIntEquals (test, 2, test_output_write_count);
	CuAssertTrue (test, test_hash_update_count > 6);
	CuAssertIntEquals (test, 1, test_release_count);
	CuAssertTrue (test, !workspace.active);
}

static size_t composite_eat_tip_main_token_test_bstr_header (size_t length,
	uint8_t header[5])
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

static void composite_eat_tip_main_token_test_es384_interoperability (CuTest *test)
{
	static struct composite_eat_tip_main_token_workspace workspace;
	static const uint8_t class_id[] = {0x01};
	static const uint8_t dme_class_id[] = {0x02};
	static const uint8_t vendor[] = "Vendor";
	static const uint8_t model[] = "Model";
	static const uint8_t dme_model[] = "ROM DME";
	static const uint8_t profile[] = "https://example.org/composite-eat/profile/v1";
	const struct tcg_concise_evidence_environment environment =
		composite_eat_tip_main_token_test_environment (class_id, sizeof (class_id), vendor,
			sizeof (vendor) - 1, model, sizeof (model) - 1);
	const struct tcg_concise_evidence_environment dme_environment =
		composite_eat_tip_main_token_test_environment (dme_class_id, sizeof (dme_class_id),
			vendor, sizeof (vendor) - 1, dme_model, sizeof (dme_model) - 1);
	const struct composite_eat_buffer profile_buffer = {profile, sizeof (profile) - 1};
	struct composite_eat_tip_evidence_adapter evidence_adapter;
	struct composite_eat_tip_main_token_generator adapter;
	struct hash_engine_openssl hash;
	struct hash_engine_openssl_state hash_state;
	struct ecc_engine_openssl ecc;
	struct ecc_public_key public_key;
	struct pcr_store store;
	struct riot_key_manager riot;
	uint8_t request[128];
	uint8_t response[2048];
	uint8_t sig_structure[COMPOSITE_EAT_PAYLOAD_MAX + COMPOSITE_EAT_PROTECTED_MAX + 32];
	uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t der_signature[ECC_DER_ECDSA_MAX_LENGTH];
	uint8_t header[5];
	QCBORDecodeContext decoder;
	UsefulBufC protected_headers;
	UsefulBufC payload;
	UsefulBufC signature;
	uint8_t dme_pcr0[TCG_CONCISE_EVIDENCE_SHA512_LENGTH];
	size_t request_length;
	size_t response_length;
	size_t sig_structure_length = 0;
	size_t header_length;
	unsigned int digest_length;
	int der_length;

	TEST_START;
	composite_eat_tip_main_token_test_reset ();
	test_keys.alias_key = test_ecc384_private_key;
	test_keys.alias_key_length = sizeof (test_ecc384_private_key);
	CuAssertIntEquals (test, 0, hash_openssl_init (&hash, &hash_state));
	CuAssertIntEquals (test, 0, ecc_openssl_init (&ecc));
	composite_eat_tip_evidence_adapter_init_with_ops (&evidence_adapter, &store, &hash.base,
		&environment, &test_evidence_ops);
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_EVIDENCE_OK,
		composite_eat_tip_evidence_enable_dme (&evidence_adapter, &dme_environment));
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_OK,
		composite_eat_tip_main_token_generator_init_with_ops (&adapter, &ecc.base, &hash.base,
			&riot, &evidence_adapter, &profile_buffer, &workspace, &test_main_token_ops));
	request_length = composite_eat_tip_main_token_test_request (request, sizeof (request));
	CuAssertIntEquals (test, COMPOSITE_EAT_TIP_MAIN_TOKEN_OK,
		composite_eat_tip_main_token_generate (&adapter, request, request_length, response,
			sizeof (response), &response_length));

	QCBORDecode_Init (&decoder, (UsefulBufC){response + 3, response_length - 3},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetByteString (&decoder, &protected_headers);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_GetByteString (&decoder, &payload);
	QCBORDecode_GetByteString (&decoder, &signature);
	QCBORDecode_ExitArray (&decoder);
	CuAssertIntEquals (test, QCBOR_SUCCESS, QCBORDecode_Finish (&decoder));
	memset (dme_pcr0, 0x5a, sizeof (dme_pcr0));
	CuAssertTrue (test, composite_eat_tip_main_token_test_contains (payload.ptr, payload.len,
		(const uint8_t *) "dme-pcr0", sizeof ("dme-pcr0") - 1));
	CuAssertTrue (test, composite_eat_tip_main_token_test_contains (payload.ptr, payload.len,
		dme_pcr0, sizeof (dme_pcr0)));

	memcpy (&sig_structure[sig_structure_length],
		(const uint8_t[]){0x84, 0x6a, 'S', 'i', 'g', 'n', 'a', 't', 'u', 'r', 'e', '1'}, 12);
	sig_structure_length += 12;
	header_length = composite_eat_tip_main_token_test_bstr_header (protected_headers.len,
		header);
	memcpy (&sig_structure[sig_structure_length], header, header_length);
	sig_structure_length += header_length;
	memcpy (&sig_structure[sig_structure_length], protected_headers.ptr, protected_headers.len);
	sig_structure_length += protected_headers.len;
	sig_structure[sig_structure_length++] = 0x40;
	header_length = composite_eat_tip_main_token_test_bstr_header (payload.len, header);
	memcpy (&sig_structure[sig_structure_length], header, header_length);
	sig_structure_length += header_length;
	memcpy (&sig_structure[sig_structure_length], payload.ptr, payload.len);
	sig_structure_length += payload.len;
	CuAssertIntEquals (test, 1, EVP_Digest (sig_structure, sig_structure_length, digest,
		&digest_length, EVP_sha384 (), NULL));
	CuAssertIntEquals (test, sizeof (digest), digest_length);

	der_length = ecc_der_encode_ecdsa_signature (signature.ptr,
		&((const uint8_t *) signature.ptr)[COMPOSITE_EAT_SHA384_LENGTH],
		COMPOSITE_EAT_SHA384_LENGTH, der_signature, sizeof (der_signature));
	CuAssertTrue (test, der_length > 0);
	CuAssertIntEquals (test, 0, ecc.base.init_public_key (&ecc.base, test_ecc384_public_key,
		sizeof (test_ecc384_public_key), &public_key));
	CuAssertIntEquals (test, 0, ecc.base.verify (&ecc.base, &public_key, digest, sizeof (digest),
		der_signature, (size_t) der_length));
	ecc.base.release_key_pair (&ecc.base, NULL, &public_key);
	hash_openssl_release (&hash);
	ecc_openssl_release (&ecc);
}


TEST_SUITE_START (composite_eat_tip_main_token_generator);

TEST (composite_eat_tip_main_token_test_generate);
TEST (composite_eat_tip_main_token_test_failures);
TEST (composite_eat_tip_main_token_test_response_exceeds_platform_limit);
TEST (composite_eat_tip_main_token_test_output_canceled);
TEST (composite_eat_tip_main_token_test_es384_interoperability);

TEST_SUITE_END;