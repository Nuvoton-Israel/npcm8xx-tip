/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "testing.h"
#include "composite_eat/profile_evidence.h"
#include "composite_eat/tcg_concise_evidence.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_spiffy_decode.h"


TEST_SUITE_LABEL ("tcg_concise_evidence");


static void composite_eat_concise_evidence_test_encode (CuTest *test)
{
	static const uint8_t expected[] = {
		0xa1, 0x00, 0xa1, 0x00, 0x81, 0x82, 0xa1, 0x00, 0xa3, 0x00, 0xd9, 0x02,
		0x30, 0x44, 0x01, 0x02, 0x03, 0x04, 0x01, 0x6e, 0x45, 0x78, 0x61, 0x6d,
		0x70, 0x6c, 0x65, 0x20, 0x56, 0x65, 0x6e, 0x64, 0x6f, 0x72, 0x02, 0x6d,
		0x45, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x4d, 0x6f, 0x64, 0x65,
		0x6c, 0x82, 0xa2, 0x00, 0x00, 0x01, 0xa1, 0x02, 0x81, 0x82, 0x67, 0x73,
		0x68, 0x61, 0x2d, 0x32, 0x35, 0x36, 0x58, 0x20, 0x25, 0x25, 0x25, 0x25,
		0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25,
		0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25,
		0x25, 0x25, 0x25, 0x25, 0xa2, 0x00, 0x01, 0x01, 0xa1, 0x02, 0x81, 0x82,
		0x67, 0x73, 0x68, 0x61, 0x2d, 0x33, 0x38, 0x34, 0x58, 0x30, 0x38, 0x38,
		0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
		0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
		0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
		0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
	};
	static const uint8_t class_id[] = {0x01, 0x02, 0x03, 0x04};
	static const uint8_t vendor[] = "Example Vendor";
	static const uint8_t model[] = "Example Model";
	uint8_t sha256[32];
	uint8_t sha384[48];
	const struct tcg_concise_evidence_digest digests[] = {
		{{TCG_CONCISE_EVIDENCE_ALGORITHM_SHA256, {0}}, {sha256, sizeof (sha256)}},
		{{TCG_CONCISE_EVIDENCE_ALGORITHM_SHA384, {0}}, {sha384, sizeof (sha384)}},
	};
	const struct tcg_concise_evidence_measurement measurements[] = {
		{true, {TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_UINT, {.uint_value = 0}},
			{&digests[0], 1, false, {0}, false, {0}}},
		{true, {TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_UINT, {.uint_value = 1}},
			{&digests[1], 1, false, {0}, false, {0}}},
	};
	const struct tcg_concise_evidence_triple triple = {
		.environment = {
			.has_class = true,
			.class_info = {
				.has_class_id = true,
				.class_id = {TCG_CONCISE_EVIDENCE_TAGGED_BYTES,
					{class_id, sizeof (class_id)}},
				.has_vendor = true,
				.vendor = {vendor, sizeof (vendor) - 1},
				.has_model = true,
				.model = {model, sizeof (model) - 1},
			},
		},
		.measurements = measurements,
		.measurement_count = 2,
	};
	const struct tcg_concise_evidence_document document = {&triple, 1};
	uint8_t encoded[512];
	size_t encoded_length;
	QCBORDecodeContext decoder;
	QCBORItem item;
	UsefulBufC decoded_class;
	UsefulBufC decoded_vendor;
	UsefulBufC decoded_model;
	UsefulBufC algorithm;
	UsefulBufC digest;
	uint64_t index;
	enum tcg_concise_evidence_status status;

	TEST_START;
	memset (sha256, 0x25, sizeof (sha256));
	memset (sha384, 0x38, sizeof (sha384));
	status = tcg_concise_evidence_encode (&document, encoded, sizeof (encoded),
		&encoded_length);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_OK, status);
	CuAssertIntEquals (test, sizeof (expected), encoded_length);
	CuAssertIntEquals (test, 0, memcmp (expected, encoded, sizeof (expected)));

	QCBORDecode_Init (&decoder, (UsefulBufC){encoded, encoded_length},
		QCBOR_DECODE_MODE_NORMAL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterMapFromMapN (&decoder, 0);
	QCBORDecode_EnterArrayFromMapN (&decoder, 0);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_EnterMapFromMapN (&decoder, 0);
	QCBORDecode_GetNext (&decoder, &item);
	CuAssertIntEquals (test, 0, item.label.int64);
	CuAssertIntEquals (test, 560, item.uTags[0]);
	decoded_class = item.val.string;
	QCBORDecode_GetTextStringInMapN (&decoder, 1, &decoded_vendor);
	QCBORDecode_GetTextStringInMapN (&decoder, 2, &decoded_model);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_EnterMap (&decoder, NULL);
	QCBORDecode_GetUInt64InMapN (&decoder, 0, &index);
	QCBORDecode_EnterMapFromMapN (&decoder, 1);
	QCBORDecode_EnterArrayFromMapN (&decoder, 2);
	QCBORDecode_EnterArray (&decoder, NULL);
	QCBORDecode_GetTextString (&decoder, &algorithm);
	QCBORDecode_GetByteString (&decoder, &digest);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitArray (&decoder);
	QCBORDecode_ExitMap (&decoder);
	QCBORDecode_ExitMap (&decoder);

	CuAssertIntEquals (test, sizeof (class_id), decoded_class.len);
	CuAssertIntEquals (test, 0, memcmp (class_id, decoded_class.ptr, decoded_class.len));
	CuAssertIntEquals (test, sizeof (vendor) - 1, decoded_vendor.len);
	CuAssertIntEquals (test, sizeof (model) - 1, decoded_model.len);
	CuAssertIntEquals (test, 0, index);
	CuAssertIntEquals (test, strlen ("sha-256"), algorithm.len);
	CuAssertIntEquals (test, 0, memcmp ("sha-256", algorithm.ptr, algorithm.len));
	CuAssertIntEquals (test, sizeof (sha256), digest.len);
}

static void composite_eat_concise_evidence_test_boundaries (CuTest *test)
{
	static const uint8_t class_id[] = {0x01};
	static const uint8_t vendor[] = "Vendor";
	static const uint8_t model[] = "Model";
	static const uint8_t digest[48] = {0};
	const struct tcg_concise_evidence_environment environment = {
		.has_class = true,
		.class_info = {
			.has_class_id = true,
			.class_id = {TCG_CONCISE_EVIDENCE_TAGGED_BYTES,
				{class_id, sizeof (class_id)}},
			.has_vendor = true,
			.vendor = {vendor, sizeof (vendor) - 1},
			.has_model = true,
			.model = {model, sizeof (model) - 1},
		},
	};
	struct tcg_concise_evidence_digest digest_entry = {
		{TCG_CONCISE_EVIDENCE_ALGORITHM_SHA384, {0}}, {digest, sizeof (digest)}
	};
	struct tcg_concise_evidence_measurement measurement = {
		true, {TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_UINT, {.uint_value = 0}},
		{&digest_entry, 1, false, {0}, false, {0}}
	};
	struct tcg_concise_evidence_triple triple = {environment, &measurement, 1};
	struct tcg_concise_evidence_document document = {&triple, 1};
	uint8_t encoded[256];
	uint8_t untouched[256];
	size_t required;
	size_t actual;
	enum tcg_concise_evidence_status status;

	TEST_START;
	status = tcg_concise_evidence_encode (&document, NULL, 0, &required);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_BUFFER_TOO_SMALL, status);
	CuAssertTrue (test, required > 0);
	memset (encoded, 0xa5, sizeof (encoded));
	memset (untouched, 0xa5, sizeof (untouched));
	status = tcg_concise_evidence_encode (&document, encoded, required - 1, &actual);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_BUFFER_TOO_SMALL, status);
	CuAssertIntEquals (test, required, actual);
	CuAssertIntEquals (test, 0, memcmp (encoded, untouched, sizeof (encoded)));

	digest_entry.value.length--;
	status = tcg_concise_evidence_encode (&document, encoded, sizeof (encoded), &actual);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_BAD_ARGUMENT, status);
}

static int tcg_concise_evidence_test_write_failure (void *context, const uint8_t *data,
	size_t length)
{
	(void) context;
	(void) data;
	(void) length;
	return -1;
}

static void tcg_concise_evidence_test_generic_forms (CuTest *test)
{
	static const uint8_t class_id[] = {0x10};
	static const uint8_t vendor[] = "Vendor";
	static const uint8_t model[] = "Model";
	static const uint8_t key[] = "component-0";
	static const uint8_t digest[TCG_CONCISE_EVIDENCE_SHA512_LENGTH] = {0x51};
	const struct tcg_concise_evidence_digest digest_entry = {
		{TCG_CONCISE_EVIDENCE_ALGORITHM_SHA512, {0}}, {digest, sizeof (digest)}
	};
	struct tcg_concise_evidence_measurement measurements[] = {{
		.has_key = true,
		.key = {TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_TEXT,
			{.text = {key, sizeof (key) - 1}}},
		.values = {
			.digests = &digest_entry,
			.digest_count = 1,
			.has_flags = true,
			.flags = {.has_is_immutable = true, .is_immutable = true,
				.has_is_tcb = true, .is_tcb = true},
		},
	}};
	const struct tcg_concise_evidence_environment environment = {
		.has_class = true,
		.class_info = {
			.has_class_id = true,
			.class_id = {TCG_CONCISE_EVIDENCE_TAGGED_BYTES,
				{class_id, sizeof (class_id)}},
			.has_vendor = true,
			.vendor = {vendor, sizeof (vendor) - 1},
			.has_model = true,
			.model = {model, sizeof (model) - 1},
		},
	};
	struct tcg_concise_evidence_triple triples[] = {
		{environment, measurements, 1}, {environment, measurements, 1}
	};
	struct tcg_concise_evidence_document document = {triples, 2};
	const struct composite_eat_profile_evidence_policy policy = {
		.max_triples = 2,
		.max_measurements_per_triple = 2,
		.max_digests_per_measurement = 2,
		.max_identifier_length = 32,
		.max_text_length = 64,
		.max_encoded_length = 512,
		.allowed_mkey_types = COMPOSITE_EAT_PROFILE_MKEY_UINT |
			COMPOSITE_EAT_PROFILE_MKEY_TEXT,
		.allowed_algorithms = COMPOSITE_EAT_PROFILE_ALGORITHM_SHA256 |
			COMPOSITE_EAT_PROFILE_ALGORITHM_SHA384 |
			COMPOSITE_EAT_PROFILE_ALGORITHM_SHA512,
		.require_measurement_key = true,
		.require_class = true,
		.require_class_id = true,
		.require_vendor = true,
		.require_model = true,
		.allow_flags = true,
	};
	size_t encoded_length;
	enum tcg_concise_evidence_status encode_status;
	enum composite_eat_profile_evidence_status profile_status;

	TEST_START;
	profile_status = composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_PROFILE_EVIDENCE_OK, profile_status);
	CuAssertTrue (test, encoded_length > 0);

	encode_status = tcg_concise_evidence_encode_write (&document,
		tcg_concise_evidence_test_write_failure, NULL, encoded_length, &encoded_length);
	CuAssertIntEquals (test, TCG_CONCISE_EVIDENCE_WRITE_ERROR, encode_status);

	triples[0].measurements = measurements;
	triples[1].measurements = measurements;
	measurements[0].key.value.text = (struct tcg_concise_evidence_text) {key,
		sizeof (key) - 1};
	triples[0].measurements = measurements;
	triples[0].measurement_count = 1;
	measurements[0].values.has_name = true;
	measurements[0].values.name = (struct tcg_concise_evidence_text) {model,
		sizeof (model) - 1};
	profile_status = composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length);
	CuAssertIntEquals (test, COMPOSITE_EAT_PROFILE_EVIDENCE_POLICY_ERROR, profile_status);
}


TEST_SUITE_START (tcg_concise_evidence);

TEST (composite_eat_concise_evidence_test_encode);
TEST (composite_eat_concise_evidence_test_boundaries);
TEST (tcg_concise_evidence_test_generic_forms);

TEST_SUITE_END;