/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "testing.h"
#include "composite_eat/generation_request.h"
#include "qcbor/qcbor_encode.h"


TEST_SUITE_LABEL ("composite_eat_generation_request");


static size_t composite_eat_generation_request_test_encode (uint8_t *encoded,
	size_t encoded_capacity, int64_t version, size_t nonce_length,
	const char *const *environments, size_t record_count, int64_t algorithm, size_t digest_length)
{
	QCBOREncodeContext encoder;
	UsefulBufC result;
	uint8_t nonce[COMPOSITE_EAT_NONCE_LENGTH + 1] = {0};
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

static void composite_eat_generation_request_test_zero_records (CuTest *test)
{
	struct composite_eat_generation_request request;
	uint8_t encoded[64];
	size_t encoded_length;
	enum composite_eat_generation_request_status status;

	TEST_START;
	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, NULL, 0,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);

	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);
	CuAssertIntEquals (test, 0, request.version);
	CuAssertIntEquals (test, 0, request.nonce_length);
	CuAssertIntEquals (test, 0, request.record_count);
}

static void composite_eat_generation_request_test_one_record (CuTest *test)
{
	static const char *environments[] = {"env.accelerator.0"};
	struct composite_eat_generation_request request;
	uint8_t encoded[256];
	size_t encoded_length;
	enum composite_eat_generation_request_status status;

	TEST_START;
	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, environments, 1,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);

	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_OK, status);
	CuAssertIntEquals (test, 1, request.record_count);
	CuAssertIntEquals (test, strlen (environments[0]), request.records[0].environment_length);
	CuAssertIntEquals (test, 0, memcmp (environments[0], request.records[0].environment,
		request.records[0].environment_length));
}

static void composite_eat_generation_request_test_nonce_length (CuTest *test)
{
	struct composite_eat_generation_request request;
	uint8_t encoded[128];
	size_t encoded_length;
	enum composite_eat_generation_request_status status;

	TEST_START;
	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH - 1, NULL, 0,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH + 1, NULL, 0,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);
}

static void composite_eat_generation_request_test_record_validation (CuTest *test)
{
	static const char *duplicate_environments[] = {"env.same", "env.same"};
	static const char *bad_environment[] = {"device.0"};
	static const char invalid_utf8[] = {'e', 'n', 'v', '.', (char) 0xc0, (char) 0x80, '\0'};
	static const char *invalid_utf8_environment[] = {invalid_utf8};
	static const char *one_environment[] = {"env.device.0"};
	struct composite_eat_generation_request request;
	uint8_t encoded[512];
	size_t encoded_length;
	enum composite_eat_generation_request_status status;

	TEST_START;
	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH,
		duplicate_environments, 2, COMPOSITE_EAT_COSE_ALGORITHM_SHA384,
		COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, bad_environment,
		1, COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH,
		invalid_utf8_environment, 1, COMPOSITE_EAT_COSE_ALGORITHM_SHA384,
		COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, one_environment,
		1, -16, COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, one_environment,
		1, COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH - 1);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);
}

static void composite_eat_generation_request_test_record_limit (CuTest *test)
{
	static const char *environments[COMPOSITE_EAT_MAX_DEVICE_RECORDS + 1];
	static char labels[COMPOSITE_EAT_MAX_DEVICE_RECORDS + 1][16];
	static uint8_t encoded[16384];
	static struct composite_eat_generation_request request;
	size_t encoded_length;
	size_t i;
	enum composite_eat_generation_request_status status;

	TEST_START;
	for (i = 0; i < (COMPOSITE_EAT_MAX_DEVICE_RECORDS + 1); ++i) {
		snprintf (labels[i], sizeof (labels[i]), "env.%zu", i);
		environments[i] = labels[i];
	}

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, environments,
		COMPOSITE_EAT_MAX_DEVICE_RECORDS, COMPOSITE_EAT_COSE_ALGORITHM_SHA384,
		COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_OK, status);
	CuAssertIntEquals (test, COMPOSITE_EAT_MAX_DEVICE_RECORDS, request.record_count);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, environments,
		COMPOSITE_EAT_MAX_DEVICE_RECORDS + 1, COMPOSITE_EAT_COSE_ALGORITHM_SHA384,
		COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_TOO_MANY_RECORDS, status);
	CuAssertIntEquals (test, 0, request.record_count);
}

static void composite_eat_generation_request_test_strict_input (CuTest *test)
{
	static const uint8_t duplicate_nonce[] = {
		0xa3, 0x01, 0x01, 0x02, 0x58, 0x20,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x02, 0x40,
	};
	struct composite_eat_generation_request request;
	uint8_t encoded[128];
	size_t encoded_length;
	enum composite_eat_generation_request_status status;

	TEST_START;
	status = composite_eat_generation_request_decode (duplicate_nonce, sizeof (duplicate_nonce),
		&request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION + 1, COMPOSITE_EAT_NONCE_LENGTH, NULL, 0,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_BAD_VERSION, status);
	CuAssertIntEquals (test, 0, request.version);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, NULL, 0,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	encoded[encoded_length - 2] = 4;
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, NULL, 0,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	encoded[encoded_length - 1] = 0x9f;
	encoded[encoded_length++] = 0xff;
	status = composite_eat_generation_request_decode (encoded, encoded_length, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	encoded_length = composite_eat_generation_request_test_encode (encoded + 3, sizeof (encoded) - 3,
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, NULL, 0,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	encoded[0] = 0xd9;
	encoded[1] = 0x03;
	encoded[2] = 0xe8;
	status = composite_eat_generation_request_decode (encoded, encoded_length + 3, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	encoded_length = composite_eat_generation_request_test_encode (encoded, sizeof (encoded),
		COMPOSITE_EAT_GENERATION_REQUEST_VERSION, COMPOSITE_EAT_NONCE_LENGTH, NULL, 0,
		COMPOSITE_EAT_COSE_ALGORITHM_SHA384, COMPOSITE_EAT_SHA384_LENGTH);
	encoded[encoded_length] = 0;
	status = composite_eat_generation_request_decode (encoded, encoded_length + 1, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED, status);

	status = composite_eat_generation_request_decode (NULL, 0, &request);
	CuAssertIntEquals (test, COMPOSITE_EAT_GENERATION_REQUEST_BAD_ARGUMENT, status);
}


TEST_SUITE_START (composite_eat_generation_request);

TEST (composite_eat_generation_request_test_zero_records);
TEST (composite_eat_generation_request_test_one_record);
TEST (composite_eat_generation_request_test_nonce_length);
TEST (composite_eat_generation_request_test_record_validation);
TEST (composite_eat_generation_request_test_record_limit);
TEST (composite_eat_generation_request_test_strict_input);

TEST_SUITE_END;