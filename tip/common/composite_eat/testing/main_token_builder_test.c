/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "testing.h"
#include "composite_eat/main_token_builder.h"
#include "composite_eat/profile_evidence.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_spiffy_decode.h"


TEST_SUITE_LABEL ("composite_eat_main_token_builder");

static const uint8_t composite_eat_main_token_expected[] = {
	0xd8, 0x3d, 0xd2, 0x84, 0x58, 0x50, 0xa3, 0x01, 0x38, 0x22, 0x03, 0x73,
	0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2f,
	0x65, 0x61, 0x74, 0x2b, 0x63, 0x77, 0x74, 0x18, 0x22, 0x82, 0x38, 0x2a,
	0x58, 0x30, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	0x33, 0x33, 0xa1, 0x18, 0x21, 0x45, 0x30, 0x03, 0x02, 0x01, 0x01, 0x58,
	0xb4, 0xa5, 0x0a, 0x58, 0x20, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
	0x11, 0x19, 0x01, 0x00, 0x49, 0x01, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
	0x16, 0x17, 0x19, 0x01, 0x09, 0x78, 0x2c, 0x68, 0x74, 0x74, 0x70, 0x73,
	0x3a, 0x2f, 0x2f, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x6f,
	0x72, 0x67, 0x2f, 0x63, 0x6f, 0x6d, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x65,
	0x2d, 0x65, 0x61, 0x74, 0x2f, 0x70, 0x72, 0x6f, 0x66, 0x69, 0x6c, 0x65,
	0x2f, 0x76, 0x31, 0x19, 0x01, 0x0a, 0xa1, 0x6c, 0x65, 0x6e, 0x76, 0x2e,
	0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2e, 0x30, 0x82, 0x38, 0x2a, 0x58,
	0x30, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
	0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
	0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
	0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
	0x22, 0x19, 0x01, 0x11, 0x81, 0x82, 0x19, 0x29, 0x4b, 0x43, 0xa1, 0x00,
	0x80, 0x58, 0x60, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44,
};

struct composite_eat_main_token_builder_test_output {
	uint8_t *data;
	size_t capacity;
	size_t length;
};

static int composite_eat_main_token_builder_test_write (void *context, const uint8_t *data,
	size_t length)
{
	struct composite_eat_main_token_builder_test_output *output = context;

	if ((output == NULL) || ((output->capacity - output->length) < length)) {
		return -1;
	}
	memcpy (&output->data[output->length], data, length);
	output->length += length;
	return 0;
}


static void composite_eat_main_token_builder_test_inputs (
	struct composite_eat_generation_request *request,
	struct composite_eat_evidence_snapshot *evidence,
	struct composite_eat_attestation_identity *identity)
{
	static const uint8_t ueid[] = {
		0x01, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
	};
	static const uint8_t profile[] = "https://example.org/composite-eat/profile/v1";
	static const uint8_t concise_evidence[] = {0xa1, 0x00, 0x80};
	static const struct composite_eat_local_evidence local_evidence[] = {{
		.content_format = COMPOSITE_EAT_TCG_CONCISE_EVIDENCE_FORMAT,
		.encoded = {concise_evidence, sizeof (concise_evidence)},
	}};
	static const uint8_t leaf_certificate[] = {0x30, 0x03, 0x02, 0x01, 0x01};
	static const struct composite_eat_der_certificate certificates[] = {
		{leaf_certificate, sizeof (leaf_certificate)},
	};
	static const char environment[] = "env.device.0";
	static uint8_t nonce[COMPOSITE_EAT_NONCE_LENGTH];
	static uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH];

	memset (request, 0, sizeof (*request));
	request->version = COMPOSITE_EAT_GENERATION_REQUEST_VERSION;
	memset (nonce, 0x11, sizeof (nonce));
	request->nonce = nonce;
	request->nonce_length = COMPOSITE_EAT_NONCE_LENGTH;
	request->record_count = 1;
	request->records[0].environment = (const uint8_t *) environment;
	request->records[0].environment_length = sizeof (environment) - 1;
	memset (digest, 0x22, sizeof (digest));
	request->records[0].digest = digest;
	request->records[0].digest_length = sizeof (digest);

	evidence->ueid = (struct composite_eat_buffer){ueid, sizeof (ueid)};
	evidence->profile = (struct composite_eat_buffer){profile, sizeof (profile) - 1};
	evidence->local_evidence = local_evidence;
	evidence->local_evidence_count = sizeof (local_evidence) / sizeof (local_evidence[0]);
	identity->certificates = certificates;
	identity->certificate_count = sizeof (certificates) / sizeof (certificates[0]);
}

static void composite_eat_main_token_builder_test_prepare (CuTest *test)
{
	struct composite_eat_generation_request request;
	struct composite_eat_evidence_snapshot evidence;
	struct composite_eat_attestation_identity identity;
	struct composite_eat_main_token_workspace workspace;
	struct composite_eat_main_token_prepared prepared;
	uint8_t thumbprint[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t signature[COMPOSITE_EAT_ES384_SIGNATURE_LENGTH] = {0};
	uint8_t response[1024];
	uint8_t signing_input[1024];
	struct composite_eat_main_token_builder_test_output signing_output = {
		signing_input, sizeof (signing_input), 0
	};
	QCBORDecodeContext decoder;
	UsefulBufC content_type;
	UsefulBufC decoded_thumbprint;
	UsefulBufC protected_headers;
	UsefulBufC payload;
	UsefulBufC nonce;
	UsefulBufC decoded_evidence;
	size_t response_length;
	int64_t protected_algorithm;
	int64_t thumbprint_algorithm;
	uint64_t content_format;
	enum composite_eat_main_token_status status;

	TEST_START;
	composite_eat_main_token_builder_test_inputs (&request, &evidence, &identity);
	memset (thumbprint, 0x33, sizeof (thumbprint));
	status = composite_eat_main_token_prepare (&request, &evidence, &identity, thumbprint,
		&workspace, &prepared);
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK, status);

	QCBORDecode_Init (&decoder,
		(UsefulBufC){workspace.protected_headers, workspace.protected_headers_length},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_GetInt64InMapN (&decoder, 1, &protected_algorithm);
	QCBORDecode_GetTextStringInMapN (&decoder, 3, &content_type);
	QCBORDecode_EnterArrayFromMapN (&decoder, 34);
	QCBORDecode_GetInt64 (&decoder, &thumbprint_algorithm);
	QCBORDecode_GetByteString (&decoder, &decoded_thumbprint);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitMap (&decoder);
	CuAssertIntEquals (test, QCBOR_SUCCESS, QCBORDecode_Finish (&decoder));
	CuAssertIntEquals (test, COMPOSITE_EAT_COSE_ALGORITHM_ES384, protected_algorithm);
	CuAssertIntEquals (test, COMPOSITE_EAT_COSE_ALGORITHM_SHA384, thumbprint_algorithm);
	CuAssertIntEquals (test, strlen ("application/eat+cwt"), content_type.len);
	CuAssertIntEquals (test, 0, memcmp ("application/eat+cwt", content_type.ptr,
		content_type.len));
	CuAssertIntEquals (test, COMPOSITE_EAT_SHA384_LENGTH, decoded_thumbprint.len);
	CuAssertIntEquals (test, 0, memcmp (thumbprint, decoded_thumbprint.ptr,
		decoded_thumbprint.len));

	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK,
		composite_eat_main_token_write_signing_input (&prepared,
			composite_eat_main_token_builder_test_write, &signing_output));
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK,
		composite_eat_main_token_finish (&prepared, signature, response, sizeof (response),
			&response_length));
	QCBORDecode_Init (&decoder, (UsefulBufC){response + 3, response_length - 3},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetByteString (&decoder, &protected_headers);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_GetByteString (&decoder, &payload);

	QCBORDecode_Init (&decoder, payload,
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_GetByteStringInMapN (&decoder, 10, &nonce);
	QCBORDecode_EnterArrayFromMapN (&decoder, 273);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetUInt64 (&decoder, &content_format);
	QCBORDecode_GetByteString (&decoder, &decoded_evidence);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitMap (&decoder);
	CuAssertIntEquals (test, QCBOR_SUCCESS, QCBORDecode_Finish (&decoder));
	CuAssertIntEquals (test, COMPOSITE_EAT_NONCE_LENGTH, nonce.len);
	CuAssertIntEquals (test, COMPOSITE_EAT_TCG_CONCISE_EVIDENCE_FORMAT, content_format);
	CuAssertIntEquals (test, evidence.local_evidence[0].encoded.length, decoded_evidence.len);
}

static void composite_eat_main_token_builder_test_sign_and_finish (CuTest *test)
{
	struct composite_eat_generation_request request;
	struct composite_eat_evidence_snapshot evidence;
	struct composite_eat_attestation_identity identity;
	struct composite_eat_main_token_workspace workspace;
	struct composite_eat_main_token_prepared prepared;
	uint8_t thumbprint[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t signature[COMPOSITE_EAT_ES384_SIGNATURE_LENGTH];
	uint8_t response[1024];
	uint8_t signing_input[1024];
	uint8_t untouched[8];
	struct composite_eat_main_token_builder_test_output signing_output = {
		signing_input, sizeof (signing_input), 0
	};
	QCBORDecodeContext decoder;
	UsefulBufC protected_headers;
	UsefulBufC certificate;
	UsefulBufC payload;
	UsefulBufC decoded_signature;
	size_t response_length = 0;
	enum composite_eat_main_token_status status;

	TEST_START;
	composite_eat_main_token_builder_test_inputs (&request, &evidence, &identity);
	memset (thumbprint, 0x33, sizeof (thumbprint));
	memset (signature, 0x44, sizeof (signature));
	status = composite_eat_main_token_prepare (&request, &evidence, &identity, thumbprint,
		&workspace, &prepared);
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK, status);
	status = composite_eat_main_token_finish (&prepared, signature, response, sizeof (response),
		&response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_STATE_ERROR, status);

	status = composite_eat_main_token_write_signing_input (&prepared,
		composite_eat_main_token_builder_test_write, &signing_output);
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK, status);
	CuAssertTrue (test, signing_output.length > prepared.payload_length);

	status = composite_eat_main_token_finish (&prepared, signature, NULL, 0, &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_BUFFER_TOO_SMALL, status);
	CuAssertTrue (test, response_length < sizeof (response));
	memset (response, 0xa5, sizeof (untouched));
	memcpy (untouched, response, sizeof (untouched));
	status = composite_eat_main_token_finish (&prepared, signature, response,
		response_length - 1, &response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_BUFFER_TOO_SMALL, status);
	CuAssertIntEquals (test, 0, memcmp (response, untouched, sizeof (untouched)));
	status = composite_eat_main_token_finish (&prepared, signature, response, sizeof (response),
		&response_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK, status);
	CuAssertIntEquals (test, sizeof (composite_eat_main_token_expected), response_length);
	CuAssertIntEquals (test, 0, memcmp (composite_eat_main_token_expected, response,
		response_length));
	CuAssertIntEquals (test, 0xd8, response[0]);
	CuAssertIntEquals (test, 0x3d, response[1]);
	CuAssertIntEquals (test, 0xd2, response[2]);

	QCBORDecode_Init (&decoder, (UsefulBufC){response + 3, response_length - 3},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetByteString (&decoder, &protected_headers);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_GetByteStringInMapN (&decoder, 33, &certificate);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_GetByteString (&decoder, &payload);
	QCBORDecode_GetByteString (&decoder, &decoded_signature);
	QCBORDecode_ExitArray (&decoder);
	CuAssertIntEquals (test, QCBOR_SUCCESS, QCBORDecode_Finish (&decoder));
	CuAssertIntEquals (test, workspace.protected_headers_length, protected_headers.len);
	CuAssertIntEquals (test, identity.certificates[0].length, certificate.len);
	CuAssertIntEquals (test, prepared.payload_length, payload.len);
	CuAssertIntEquals (test, sizeof (signature), decoded_signature.len);
	CuAssertIntEquals (test, 0, memcmp (signature, decoded_signature.ptr,
		decoded_signature.len));
}

static void composite_eat_main_token_builder_test_deterministic_records (CuTest *test)
{
	struct composite_eat_generation_request first;
	struct composite_eat_generation_request second;
	struct composite_eat_device_record saved;
	struct composite_eat_evidence_snapshot evidence;
	struct composite_eat_attestation_identity identity;
	struct composite_eat_main_token_workspace first_workspace;
	struct composite_eat_main_token_workspace second_workspace;
	struct composite_eat_main_token_prepared first_prepared;
	struct composite_eat_main_token_prepared second_prepared;
	uint8_t first_signing_input[2048];
	uint8_t second_signing_input[2048];
	struct composite_eat_main_token_builder_test_output first_output = {
		first_signing_input, sizeof (first_signing_input), 0
	};
	struct composite_eat_main_token_builder_test_output second_output = {
		second_signing_input, sizeof (second_signing_input), 0
	};
	uint8_t thumbprint[COMPOSITE_EAT_SHA384_LENGTH] = {0};
	static const char second_environment[] = "env.device.1";
	static uint8_t second_digest[COMPOSITE_EAT_SHA384_LENGTH];

	TEST_START;
	composite_eat_main_token_builder_test_inputs (&first, &evidence, &identity);
	first.records[1] = first.records[0];
	first.records[1].environment = (const uint8_t *) second_environment;
	first.records[1].environment_length = sizeof (second_environment) - 1;
	memset (second_digest, 0x55, sizeof (second_digest));
	first.records[1].digest = second_digest;
	first.records[1].digest_length = sizeof (second_digest);
	first.record_count = 2;
	second = first;
	saved = second.records[0];
	second.records[0] = second.records[1];
	second.records[1] = saved;

	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK,
		composite_eat_main_token_prepare (&first, &evidence, &identity, thumbprint,
			&first_workspace, &first_prepared));
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK,
		composite_eat_main_token_prepare (&second, &evidence, &identity, thumbprint,
			&second_workspace, &second_prepared));
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK,
		composite_eat_main_token_write_signing_input (&first_prepared,
			composite_eat_main_token_builder_test_write, &first_output));
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK,
		composite_eat_main_token_write_signing_input (&second_prepared,
			composite_eat_main_token_builder_test_write, &second_output));
	CuAssertIntEquals (test, first_output.length, second_output.length);
	CuAssertIntEquals (test, 0, memcmp (first_output.data, second_output.data,
		first_output.length));
}

static void composite_eat_main_token_builder_test_clear_and_validation (CuTest *test)
{
	struct composite_eat_generation_request request;
	struct composite_eat_evidence_snapshot evidence;
	struct composite_eat_attestation_identity identity;
	struct composite_eat_main_token_workspace workspace;
	struct composite_eat_main_token_prepared prepared;
	uint8_t thumbprint[COMPOSITE_EAT_SHA384_LENGTH] = {0};
	const uint8_t *workspace_bytes = (const uint8_t *) &workspace;
	size_t i;

	TEST_START;
	composite_eat_main_token_builder_test_inputs (&request, &evidence, &identity);
	request.nonce_length--;
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_BAD_ARGUMENT,
		composite_eat_main_token_prepare (&request, &evidence, &identity, thumbprint,
			&workspace, &prepared));
	CuAssertTrue (test, !prepared.ready);
	for (i = 0; i < sizeof (workspace); ++i) {
		CuAssertIntEquals (test, 0, workspace_bytes[i]);
	}

	request.nonce_length = COMPOSITE_EAT_NONCE_LENGTH;
	CuAssertIntEquals (test, COMPOSITE_EAT_MAIN_TOKEN_OK,
		composite_eat_main_token_prepare (&request, &evidence, &identity, thumbprint,
			&workspace, &prepared));
	composite_eat_main_token_clear (&workspace, &prepared);
	CuAssertTrue (test, !prepared.ready);
	for (i = 0; i < sizeof (workspace); ++i) {
		CuAssertIntEquals (test, 0, workspace_bytes[i]);
	}
}


TEST_SUITE_START (composite_eat_main_token_builder);

TEST (composite_eat_main_token_builder_test_prepare);
TEST (composite_eat_main_token_builder_test_sign_and_finish);
TEST (composite_eat_main_token_builder_test_deterministic_records);
TEST (composite_eat_main_token_builder_test_clear_and_validation);

TEST_SUITE_END;