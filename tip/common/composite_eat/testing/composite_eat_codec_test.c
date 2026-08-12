/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "testing.h"
#include "composite_eat/composite_eat_codec.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_encode.h"
#include "qcbor/qcbor_spiffy_decode.h"


TEST_SUITE_LABEL ("composite_eat_codec");


static void composite_eat_codec_test_init_request (struct composite_eat_request *request,
	size_t record_count)
{
	size_t i;
	int length;

	memset (request, 0, sizeof (*request));
	request->version = COMPOSITE_EAT_REQUEST_VERSION;
	request->nonce_length = 16;
	memset (request->nonce, 0x5a, request->nonce_length);
	request->record_count = record_count;

	for (i = 0; i < record_count; ++i) {
		length = snprintf ((char *) request->records[i].environment,
			sizeof (request->records[i].environment), "e%zu", i);
		request->records[i].environment_length = (size_t) length;
		memset (request->records[i].digest, (int) i, sizeof (request->records[i].digest));
	}
}

static size_t composite_eat_codec_test_encode_raw_request (uint8_t *encoded,
	size_t encoded_capacity, int64_t version, const uint8_t *nonce, size_t nonce_length,
	const char *const *environments, size_t record_count, int64_t algorithm, size_t digest_length)
{
	QCBOREncodeContext encoder;
	UsefulBufC result;
	uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH + 1] = {0};
	size_t i;

	QCBOREncode_Init (&encoder, (UsefulBuf){encoded, encoded_capacity});
	QCBOREncode_OpenMap (&encoder);
	QCBOREncode_AddInt64ToMapN (&encoder, 1, version);
	QCBOREncode_AddBytesToMapN (&encoder, 2, (UsefulBufC){nonce, nonce_length});
	QCBOREncode_OpenArrayInMapN (&encoder, 3);
	for (i = 0; i < record_count; ++i) {
		QCBOREncode_OpenMap (&encoder);
		QCBOREncode_AddSZStringToMapN (&encoder, 1, environments[i]);
		QCBOREncode_OpenArrayInMapN (&encoder, 2);
		QCBOREncode_AddInt64 (&encoder, algorithm);
		QCBOREncode_AddBytes (&encoder, (UsefulBufC){digest, digest_length});
		QCBOREncode_CloseArray (&encoder);
		QCBOREncode_CloseMap (&encoder);
	}
	QCBOREncode_CloseArray (&encoder);
	QCBOREncode_CloseMap (&encoder);

	if (QCBOREncode_Finish (&encoder, &result) != QCBOR_SUCCESS) {
		return 0;
	}

	return result.len;
}

static void composite_eat_codec_test_empty_request (CuTest *test)
{
	static const uint8_t expected[] = {
		0xa3,
		0x01,
		0x01,
		0x02,
		0x50,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x5a,
		0x03,
		0x80,
	};
	struct composite_eat_request request;
	struct composite_eat_request decoded;
	uint8_t encoded[64];
	size_t encoded_length = 0;
	enum composite_eat_codec_status status;

	TEST_START;
	composite_eat_codec_test_init_request (&request, 0);

	status =
		composite_eat_codec_encode_request (&request, encoded, sizeof (encoded), &encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	CuAssertIntEquals (test, sizeof (expected), encoded_length);
	CuAssertIntEquals (test, 0, memcmp (expected, encoded, sizeof (expected)));

	status = composite_eat_codec_decode_request (encoded, encoded_length, &decoded);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	CuAssertIntEquals (test, 0, decoded.record_count);
	CuAssertIntEquals (test, request.nonce_length, decoded.nonce_length);
	CuAssertIntEquals (test, 0, memcmp (request.nonce, decoded.nonce, request.nonce_length));
}

static void composite_eat_codec_test_single_request (CuTest *test)
{
	struct composite_eat_request request;
	struct composite_eat_request decoded;
	uint8_t encoded[256];
	size_t encoded_length = 0;
	enum composite_eat_codec_status status;

	TEST_START;
	composite_eat_codec_test_init_request (&request, 1);

	status =
		composite_eat_codec_encode_request (&request, encoded, sizeof (encoded), &encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	status = composite_eat_codec_decode_request (encoded, encoded_length, &decoded);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	CuAssertIntEquals (test, 1, decoded.record_count);
	CuAssertIntEquals (test, request.records[0].environment_length,
		decoded.records[0].environment_length);
	CuAssertIntEquals (test, 0,
		memcmp (request.records[0].environment, decoded.records[0].environment,
			request.records[0].environment_length));
	CuAssertIntEquals (test, 0,
		memcmp (request.records[0].digest, decoded.records[0].digest,
			sizeof (request.records[0].digest)));
}

static void composite_eat_codec_test_max_request (CuTest *test)
{
	static struct composite_eat_request request;
	static struct composite_eat_request decoded;
	static uint8_t encoded[8192];
	size_t encoded_length = 0;
	enum composite_eat_codec_status status;

	TEST_START;
	composite_eat_codec_test_init_request (&request, COMPOSITE_EAT_MAX_DEVICE_RECORDS);

	status =
		composite_eat_codec_encode_request (&request, encoded, sizeof (encoded), &encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	status = composite_eat_codec_decode_request (encoded, encoded_length, &decoded);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	CuAssertIntEquals (test, COMPOSITE_EAT_MAX_DEVICE_RECORDS, decoded.record_count);
	CuAssertIntEquals (test, 0,
		memcmp (request.records[63].digest, decoded.records[63].digest,
			sizeof (request.records[63].digest)));
}

static void composite_eat_codec_test_strict_top_level (CuTest *test)
{
	static const uint8_t tagged_root[] = {
		0xd9,
		0x03,
		0xe8,
		0xa3,
		0x01,
		0x01,
		0x02,
		0x48,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0x03,
		0x80,
	};
	static const uint8_t tagged_nonce[] = {
		0xa3,
		0x01,
		0x01,
		0x02,
		0xd9,
		0x03,
		0xe8,
		0x48,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0x03,
		0x80,
	};
	static const uint8_t duplicate_nonce[] = {
		0xa3,
		0x01,
		0x01,
		0x02,
		0x48,
		0,
		0,
		0,
		0,
		0,
		0,
		0x02,
		0x48,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
	};
	static const uint8_t unknown_field[] = {
		0xa3,
		0x01,
		0x01,
		0x02,
		0x48,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0x04,
		0x80,
	};
	static const uint8_t wrong_nonce_type[] = {
		0xa3,
		0x01,
		0x01,
		0x02,
		0x01,
		0x03,
		0x80,
	};
	struct composite_eat_request request;
	uint8_t encoded[64];
	size_t encoded_length;
	enum composite_eat_codec_status status;

	TEST_START;
	status = composite_eat_codec_decode_request (tagged_root, sizeof (tagged_root), &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);
	status = composite_eat_codec_decode_request (tagged_nonce, sizeof (tagged_nonce), &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);

	status =
		composite_eat_codec_decode_request (duplicate_nonce, sizeof (duplicate_nonce), &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);
	status = composite_eat_codec_decode_request (unknown_field, sizeof (unknown_field), &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);
	status =
		composite_eat_codec_decode_request (wrong_nonce_type, sizeof (wrong_nonce_type), &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);

	composite_eat_codec_test_init_request (&request, 0);
	status =
		composite_eat_codec_encode_request (&request, encoded, sizeof (encoded), &encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);

	encoded[2] = 2;
	status = composite_eat_codec_decode_request (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_BAD_VERSION, status);
	encoded[2] = COMPOSITE_EAT_REQUEST_VERSION;
	status = composite_eat_codec_decode_request (encoded, encoded_length - 1, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);
}

static void composite_eat_codec_test_strict_records (CuTest *test)
{
	static uint8_t encoded[16384];
	static const uint8_t nonce[16] = {0};
	static const char *duplicate_environments[] = {"same", "same"};
	static const char *one_environment[] = {"one"};
	struct composite_eat_request request;
	size_t encoded_length;
	enum composite_eat_codec_status status;

	TEST_START;

	encoded_length = composite_eat_codec_test_encode_raw_request (encoded, sizeof (encoded),
		COMPOSITE_EAT_REQUEST_VERSION, nonce, sizeof (nonce), duplicate_environments, 2,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_codec_decode_request (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);

	encoded_length = composite_eat_codec_test_encode_raw_request (encoded, sizeof (encoded),
		COMPOSITE_EAT_REQUEST_VERSION, nonce, sizeof (nonce), one_environment, 1, -16,
		COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_codec_decode_request (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);

	encoded_length = composite_eat_codec_test_encode_raw_request (encoded, sizeof (encoded),
		COMPOSITE_EAT_REQUEST_VERSION, nonce, sizeof (nonce), one_environment, 1,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH - 1);
	status = composite_eat_codec_decode_request (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);

	memset (encoded, 0, sizeof (encoded));
	encoded[0] = 0xa3;
	encoded[1] = 0x01;
	encoded[2] = 0x01;
	encoded[3] = 0x02;
	encoded[4] = 0x48;
	encoded[13] = 0x03;
	encoded[14] = 0x98;
	encoded[15] = 65;
	status = composite_eat_codec_decode_request (encoded, 16, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_TOO_MANY_RECORDS, status);
}

static void composite_eat_codec_test_buffer_boundaries (CuTest *test)
{
	struct composite_eat_request request;
	uint8_t encoded[256];
	uint8_t untouched[256];
	size_t required = 0;
	size_t actual = 0;
	enum composite_eat_codec_status status;

	TEST_START;
	composite_eat_codec_test_init_request (&request, 1);
	memset (encoded, 0xa5, sizeof (encoded));
	memset (untouched, 0xa5, sizeof (untouched));

	status = composite_eat_codec_encode_request (&request, NULL, 0, &required);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_BUFFER_TOO_SMALL, status);
	CuAssertTrue (test, required > 0);

	status = composite_eat_codec_encode_request (&request, encoded, required - 1, &actual);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_BUFFER_TOO_SMALL, status);
	CuAssertIntEquals (test, required, actual);
	CuAssertIntEquals (test, 0, memcmp (encoded, untouched, sizeof (encoded)));

	status = composite_eat_codec_encode_request (&request, encoded, required, &actual);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	CuAssertIntEquals (test, required, actual);
}

static void composite_eat_codec_test_payload (CuTest *test)
{
	static const uint8_t ueid[] = {0x01, 0x02, 0x03};
	static const uint8_t profile[] = "tag:test";
	static const uint8_t measurements[] = {0xa1, 0x00, 0x80};
	struct composite_eat_request request;
	struct composite_eat_claims claims = {
		.ueid = {ueid, sizeof (ueid)},
		.profile = {profile, sizeof (profile) - 1},
		.measurement_content_format = 10571,
		.measurements = {measurements, sizeof (measurements)},
	};
	uint8_t first[512];
	uint8_t second[512];
	size_t first_length = 0;
	size_t second_length = 0;
	QCBORDecodeContext decoder;
	UsefulBufC nonce;
	UsefulBufC decoded_ueid;
	UsefulBufC decoded_profile;
	UsefulBufC digest;
	UsefulBufC evidence;
	int64_t algorithm = 0;
	uint64_t content_format = 0;
	enum composite_eat_codec_status status;

	TEST_START;
	composite_eat_codec_test_init_request (&request, 1);

	status = composite_eat_codec_encode_payload (&request, &claims, first, sizeof (first),
		&first_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	status = composite_eat_codec_encode_payload (&request, &claims, second, sizeof (second),
		&second_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	CuAssertIntEquals (test, first_length, second_length);
	CuAssertIntEquals (test, 0, memcmp (first, second, first_length));

	QCBORDecode_Init (&decoder, (UsefulBufC){first, first_length}, QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_GetByteStringInMapN (&decoder, 10, &nonce);
	QCBORDecode_GetByteStringInMapN (&decoder, 256, &decoded_ueid);
	QCBORDecode_GetTextStringInMapN (&decoder, 265, &decoded_profile);
	QCBORDecode_EnterMapFromMapN (&decoder, 266);
	QCBORDecode_EnterArrayFromMapSZ (&decoder, "e0");
	QCBORDecode_GetInt64 (&decoder, &algorithm);
	QCBORDecode_GetByteString (&decoder, &digest);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_EnterArrayFromMapN (&decoder, 273);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetUInt64 (&decoder, &content_format);
	QCBORDecode_GetByteString (&decoder, &evidence);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitMap (&decoder);

	CuAssertIntEquals (test, QCBOR_SUCCESS, QCBORDecode_Finish (&decoder));
	CuAssertIntEquals (test, request.nonce_length, nonce.len);
	CuAssertIntEquals (test, sizeof (ueid), decoded_ueid.len);
	CuAssertIntEquals (test, sizeof (profile) - 1, decoded_profile.len);
	CuAssertIntEquals (test, COMPOSITE_EAT_COSE_ALGORITHM_SHA384, algorithm);
	CuAssertIntEquals (test, COMPOSITE_EAT_SHA384_LENGTH, digest.len);
	CuAssertIntEquals (test, 10571, content_format);
	CuAssertIntEquals (test, sizeof (measurements), evidence.len);
}

static void composite_eat_codec_test_payload_deterministic_submodules (CuTest *test)
{
	static const uint8_t ueid[] = {0x01};
	static const uint8_t profile[] = "tag:test";
	static const uint8_t measurements[] = {0xa0};
	struct composite_eat_request first_request;
	struct composite_eat_request second_request;
	struct composite_eat_device_record saved_record;
	struct composite_eat_claims claims = {
		.ueid = {ueid, sizeof (ueid)},
		.profile = {profile, sizeof (profile) - 1},
		.measurement_content_format = 10571,
		.measurements = {measurements, sizeof (measurements)},
	};
	uint8_t first[512];
	uint8_t second[512];
	size_t first_length = 0;
	size_t second_length = 0;
	enum composite_eat_codec_status status;

	TEST_START;
	composite_eat_codec_test_init_request (&first_request, 2);
	memcpy (first_request.records[0].environment, "aa", 2);
	first_request.records[0].environment_length = 2;
	memcpy (first_request.records[1].environment, "z", 1);
	first_request.records[1].environment_length = 1;
	second_request = first_request;
	saved_record = second_request.records[0];
	second_request.records[0] = second_request.records[1];
	second_request.records[1] = saved_record;

	status = composite_eat_codec_encode_payload (&first_request, &claims, first, sizeof (first),
		&first_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	status = composite_eat_codec_encode_payload (&second_request, &claims, second, sizeof (second),
		&second_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	CuAssertIntEquals (test, first_length, second_length);
	CuAssertIntEquals (test, 0, memcmp (first, second, first_length));
}

static void composite_eat_codec_test_utf8_validation (CuTest *test)
{
	static const char invalid_environment[] = {(char) 0xc0, (char) 0x80, '\0'};
	static const char *environments[] = {invalid_environment};
	static const uint8_t nonce[16] = {0};
	static const uint8_t ueid[] = {0x01};
	static const uint8_t invalid_profile[] = {0xed, 0xa0, 0x80};
	static const uint8_t measurements[] = {0xa0};
	struct composite_eat_request request;
	struct composite_eat_claims claims = {
		.ueid = {ueid, sizeof (ueid)},
		.profile = {invalid_profile, sizeof (invalid_profile)},
		.measurement_content_format = 10571,
		.measurements = {measurements, sizeof (measurements)},
	};
	uint8_t encoded[256];
	size_t encoded_length;
	enum composite_eat_codec_status status;

	TEST_START;
	encoded_length = composite_eat_codec_test_encode_raw_request (encoded, sizeof (encoded),
		COMPOSITE_EAT_REQUEST_VERSION, nonce, sizeof (nonce), environments, 1,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_codec_decode_request (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_MALFORMED_REQUEST, status);

	composite_eat_codec_test_init_request (&request, 1);
	request.records[0].environment[0] = 0xc0;
	request.records[0].environment[1] = 0x80;
	request.records[0].environment_length = 2;
	status =
		composite_eat_codec_encode_request (&request, encoded, sizeof (encoded), &encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_BAD_ARGUMENT, status);

	composite_eat_codec_test_init_request (&request, 0);
	status = composite_eat_codec_encode_payload (&request, &claims, encoded, sizeof (encoded),
		&encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_BAD_ARGUMENT, status);
}

static void composite_eat_codec_test_cose (CuTest *test)
{
	uint8_t thumbprint[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t protected_headers[128];
	uint8_t payload[] = {0xa0};
	uint8_t signature[COMPOSITE_EAT_ES384_SIGNATURE_LENGTH];
	uint8_t certificate[] = {0x30, 0x01, 0x00};
	uint8_t certificate2[] = {0x30, 0x01, 0x01};
	uint8_t encoded[512];
	size_t protected_length = 0;
	size_t encoded_length = 0;
	struct composite_eat_cose_sign1 cose;
	QCBORDecodeContext decoder;
	UsefulBufC decoded_protected;
	UsefulBufC decoded_certificate;
	UsefulBufC decoded_certificate2;
	UsefulBufC decoded_payload;
	UsefulBufC decoded_signature;
	enum composite_eat_codec_status status;

	TEST_START;
	memset (thumbprint, 0xab, sizeof (thumbprint));
	memset (signature, 0xcd, sizeof (signature));

	status = composite_eat_codec_encode_protected_headers (thumbprint, protected_headers,
		sizeof (protected_headers), &protected_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	CuAssertIntEquals (test, 59, protected_length);
	CuAssertIntEquals (test, 0xa2, protected_headers[0]);

	memset (&cose, 0, sizeof (cose));
	cose.protected_headers.data = protected_headers;
	cose.protected_headers.length = protected_length;
	cose.payload.data = payload;
	cose.payload.length = sizeof (payload);
	cose.signature.data = signature;
	cose.signature.length = sizeof (signature);
	cose.certificates[0].data = certificate;
	cose.certificates[0].length = sizeof (certificate);
	cose.certificate_count = 1;

	status =
		composite_eat_codec_encode_cose_sign1 (&cose, encoded, sizeof (encoded), &encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);
	CuAssertIntEquals (test, 0xd2, encoded[0]);

	QCBORDecode_Init (&decoder, (UsefulBufC){encoded + 1, encoded_length - 1},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetByteString (&decoder, &decoded_protected);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_GetByteStringInMapN (&decoder, 33, &decoded_certificate);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_GetByteString (&decoder, &decoded_payload);
	QCBORDecode_GetByteString (&decoder, &decoded_signature);
	QCBORDecode_ExitArray (&decoder);

	CuAssertIntEquals (test, QCBOR_SUCCESS, QCBORDecode_Finish (&decoder));
	CuAssertIntEquals (test, protected_length, decoded_protected.len);
	CuAssertIntEquals (test, sizeof (certificate), decoded_certificate.len);
	CuAssertIntEquals (test, sizeof (payload), decoded_payload.len);
	CuAssertIntEquals (test, sizeof (signature), decoded_signature.len);

	cose.certificates[1].data = certificate2;
	cose.certificates[1].length = sizeof (certificate2);
	cose.certificate_count = 2;
	status =
		composite_eat_codec_encode_cose_sign1 (&cose, encoded, sizeof (encoded), &encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_CODEC_OK, status);

	QCBORDecode_Init (&decoder, (UsefulBufC){encoded + 1, encoded_length - 1},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetByteString (&decoder, &decoded_protected);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterArrayFromMapN (&decoder, 33);
	QCBORDecode_GetByteString (&decoder, &decoded_certificate);
	QCBORDecode_GetByteString (&decoder, &decoded_certificate2);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_GetByteString (&decoder, &decoded_payload);
	QCBORDecode_GetByteString (&decoder, &decoded_signature);
	QCBORDecode_ExitArray (&decoder);

	CuAssertIntEquals (test, QCBOR_SUCCESS, QCBORDecode_Finish (&decoder));
	CuAssertIntEquals (test, sizeof (certificate), decoded_certificate.len);
	CuAssertIntEquals (test, sizeof (certificate2), decoded_certificate2.len);
}


TEST_SUITE_START (composite_eat_codec);

TEST (composite_eat_codec_test_empty_request);
TEST (composite_eat_codec_test_single_request);
TEST (composite_eat_codec_test_max_request);
TEST (composite_eat_codec_test_strict_top_level);
TEST (composite_eat_codec_test_strict_records);
TEST (composite_eat_codec_test_buffer_boundaries);
TEST (composite_eat_codec_test_payload);
TEST (composite_eat_codec_test_payload_deterministic_submodules);
TEST (composite_eat_codec_test_utf8_validation);
TEST (composite_eat_codec_test_cose);

TEST_SUITE_END;