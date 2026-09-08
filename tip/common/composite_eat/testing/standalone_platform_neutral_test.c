/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "composite_eat/generation_request.h"
#include "composite_eat/main_token_builder.h"
#include "composite_eat/profile_evidence.h"
#include "composite_eat/tcg_concise_evidence.h"


#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf (stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
			return 1; \
		} \
	} while (0)

struct test_output {
	uint8_t *data;
	size_t capacity;
	size_t length;
};

struct test_evidence_source {
	const uint8_t *data;
	size_t length;
};

static int test_write (void *context, const uint8_t *data, size_t length)
{
	struct test_output *output = context;

	if ((output == NULL) || (output->length > output->capacity) ||
		((output->capacity - output->length) < length)) {
		return -1;
	}
	memcpy (&output->data[output->length], data, length);
	output->length += length;
	return 0;
}

static int test_write_failure (void *context, const uint8_t *data, size_t length)
{
	(void) context;
	(void) data;
	(void) length;
	return -1;
}

static int test_write_evidence (void *context, composite_eat_main_token_write write,
	void *write_context)
{
	const struct test_evidence_source *source = context;

	return write (write_context, source->data, source->length);
}

static struct tcg_concise_evidence_environment test_environment (const uint8_t *class_id,
	size_t class_id_length, const uint8_t *model, size_t model_length)
{
	static const uint8_t vendor[] = "Example Vendor";
	struct tcg_concise_evidence_environment environment = {
		.has_class = true,
		.class_info = {
			.has_class_id = true,
			.class_id = {TCG_CONCISE_EVIDENCE_TAGGED_BYTES,
				{class_id, class_id_length}},
			.has_vendor = true,
			.vendor = {vendor, sizeof (vendor) - 1},
			.has_model = true,
			.model = {model, model_length},
		},
	};

	return environment;
}

static int test_legacy_vector (void)
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
	static const uint8_t class_id[] = {1, 2, 3, 4};
	static const uint8_t model[] = "Example Model";
	uint8_t sha256[TCG_CONCISE_EVIDENCE_SHA256_LENGTH];
	uint8_t sha384[TCG_CONCISE_EVIDENCE_SHA384_LENGTH];
	struct tcg_concise_evidence_digest digests[2];
	struct tcg_concise_evidence_measurement measurements[2] = {0};
	struct tcg_concise_evidence_triple triple = {0};
	struct tcg_concise_evidence_document document = {&triple, 1};
	uint8_t encoded[sizeof (expected)];
	size_t encoded_length;

	memset (sha256, 0x25, sizeof (sha256));
	memset (sha384, 0x38, sizeof (sha384));
	digests[0] = (struct tcg_concise_evidence_digest) {
		{TCG_CONCISE_EVIDENCE_ALGORITHM_SHA256, {0}}, {sha256, sizeof (sha256)}};
	digests[1] = (struct tcg_concise_evidence_digest) {
		{TCG_CONCISE_EVIDENCE_ALGORITHM_SHA384, {0}}, {sha384, sizeof (sha384)}};
	measurements[0].has_key = true;
	measurements[0].key.type = TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_UINT;
	measurements[0].key.value.uint_value = 0;
	measurements[0].values.digests = &digests[0];
	measurements[0].values.digest_count = 1;
	measurements[1].has_key = true;
	measurements[1].key.type = TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_UINT;
	measurements[1].key.value.uint_value = 1;
	measurements[1].values.digests = &digests[1];
	measurements[1].values.digest_count = 1;
	triple.environment = test_environment (class_id, sizeof (class_id), model,
		sizeof (model) - 1);
	triple.measurements = measurements;
	triple.measurement_count = 2;

	CHECK (tcg_concise_evidence_encode (&document, encoded, sizeof (encoded),
		&encoded_length) == TCG_CONCISE_EVIDENCE_OK);
	CHECK (encoded_length == sizeof (expected));
	CHECK (memcmp (encoded, expected, sizeof (expected)) == 0);
	return 0;
}

static int test_generic_document (void)
{
	static const uint8_t rot_class[] = "generic-rot";
	static const uint8_t rom_class[] = "generic-rom";
	static const uint8_t rot_model[] = "Generic RoT";
	static const uint8_t rom_model[] = "Generic ROM";
	static const uint8_t component_key[] = "component-0";
	uint8_t sha256[TCG_CONCISE_EVIDENCE_SHA256_LENGTH] = {0};
	uint8_t sha512[TCG_CONCISE_EVIDENCE_SHA512_LENGTH] = {0};
	struct tcg_concise_evidence_digest digests[] = {
		{{TCG_CONCISE_EVIDENCE_ALGORITHM_SHA256, {0}}, {sha256, sizeof (sha256)}},
		{{TCG_CONCISE_EVIDENCE_ALGORITHM_SHA512, {0}}, {sha512, sizeof (sha512)}},
	};
	struct tcg_concise_evidence_measurement measurements[2] = {0};
	struct tcg_concise_evidence_triple triples[2] = {0};
	struct tcg_concise_evidence_document document = {triples, 2};
	const struct composite_eat_profile_evidence_policy policy = {
		.max_triples = 2,
		.max_measurements_per_triple = 8,
		.max_digests_per_measurement = 2,
		.max_identifier_length = 64,
		.max_text_length = 64,
		.max_encoded_length = 1024,
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
	uint8_t encoded[1024];
	size_t encoded_length;
	const char *output_path;
	FILE *output;

	measurements[0].has_key = true;
	measurements[0].key.type = TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_UINT;
	measurements[0].key.value.uint_value = 0;
	measurements[0].values.digests = &digests[0];
	measurements[0].values.digest_count = 1;
	measurements[1].has_key = true;
	measurements[1].key.type = TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_TEXT;
	measurements[1].key.value.text =
		(struct tcg_concise_evidence_text) {component_key, sizeof (component_key) - 1};
	measurements[1].values.digests = &digests[1];
	measurements[1].values.digest_count = 1;
	measurements[1].values.has_flags = true;
	measurements[1].values.flags.has_is_integrity_protected = true;
	measurements[1].values.flags.is_integrity_protected = true;
	measurements[1].values.flags.has_is_immutable = true;
	measurements[1].values.flags.is_immutable = true;
	measurements[1].values.flags.has_is_tcb = true;
	measurements[1].values.flags.is_tcb = true;
	triples[0].environment = test_environment (rot_class, sizeof (rot_class) - 1,
		rot_model, sizeof (rot_model) - 1);
	triples[0].measurements = &measurements[0];
	triples[0].measurement_count = 1;
	triples[1].environment = test_environment (rom_class, sizeof (rom_class) - 1,
		rom_model, sizeof (rom_model) - 1);
	triples[1].measurements = &measurements[1];
	triples[1].measurement_count = 1;

	CHECK (composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length) == COMPOSITE_EAT_PROFILE_EVIDENCE_OK);
	CHECK (tcg_concise_evidence_encode (&document, encoded, sizeof (encoded),
		&encoded_length) == TCG_CONCISE_EVIDENCE_OK);
	output_path = getenv ("TCG_CE_OUTPUT");
	if (output_path != NULL) {
		output = fopen (output_path, "wb");
		CHECK (output != NULL);
		if (fwrite (encoded, 1, encoded_length, output) != encoded_length) {
			fclose (output);
			return 1;
		}
		CHECK (fclose (output) == 0);
	}
	CHECK (tcg_concise_evidence_encode_write (&document, test_write_failure, NULL,
		encoded_length, &encoded_length) == TCG_CONCISE_EVIDENCE_WRITE_ERROR);

	document.evidence_triple_count = policy.max_triples + 1;
	CHECK (composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length) == COMPOSITE_EAT_PROFILE_EVIDENCE_LIMIT_EXCEEDED);
	document.evidence_triple_count = 2;
	triples[0].measurement_count = policy.max_measurements_per_triple + 1;
	CHECK (composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length) == COMPOSITE_EAT_PROFILE_EVIDENCE_LIMIT_EXCEEDED);
	triples[0].measurement_count = 1;
	measurements[0].values.digest_count = policy.max_digests_per_measurement + 1;
	CHECK (composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length) == COMPOSITE_EAT_PROFILE_EVIDENCE_LIMIT_EXCEEDED);
	measurements[0].values.digest_count = 1;

	triples[0].measurements = measurements;
	triples[0].measurement_count = 2;
	measurements[0].key.value.uint_value = 5;
	measurements[1].key.type = TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_UINT;
	measurements[1].key.value.uint_value = 3;
	CHECK (composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length) == COMPOSITE_EAT_PROFILE_EVIDENCE_OK);
	measurements[1].key.value.uint_value = 5;
	CHECK (composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length) == COMPOSITE_EAT_PROFILE_EVIDENCE_POLICY_ERROR);
	measurements[1].key.type = TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_TEXT;
	measurements[1].key.value.text =
		(struct tcg_concise_evidence_text) {component_key, sizeof (component_key) - 1};
	digests[1].value.length--;
	CHECK (composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length) == COMPOSITE_EAT_PROFILE_EVIDENCE_SCHEMA_ERROR);
	return 0;
}

static int test_generic_identifier_and_algorithm_forms (void)
{
	static const uint8_t class_id[] = {0x01};
	static const uint8_t model[] = "Generic Model";
	static const uint8_t ueid[] = {0x01, 1, 2, 3, 4, 5, 6};
	static const uint8_t group[] = {0x47};
	static const uint8_t uuid[16] = {0x37};
	static const uint8_t oid[] = {0x2b, 0x06, 0x01};
	static const uint8_t algorithm_name[] = "vendor-hash";
	static const uint8_t digest_value[] = {1, 2, 3};
	struct tcg_concise_evidence_digest digests[2] = {
		{{TCG_CONCISE_EVIDENCE_ALGORITHM_INTEGER, {.integer = -16}},
			{digest_value, sizeof (digest_value)}},
		{{TCG_CONCISE_EVIDENCE_ALGORITHM_TEXT,
			{.text = {algorithm_name, sizeof (algorithm_name) - 1}}},
			{digest_value, sizeof (digest_value)}},
	};
	struct tcg_concise_evidence_measurement measurement = {
		.has_key = true,
		.key = {TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_UUID,
			{.bytes = {uuid, sizeof (uuid)}}},
		.values = {.digests = digests, .digest_count = 2},
	};
	struct tcg_concise_evidence_triple triple = {
		.environment = {
			.has_class = true,
			.class_info = {
				.has_class_id = true,
				.class_id = {TCG_CONCISE_EVIDENCE_TAGGED_BYTES,
					{class_id, sizeof (class_id)}},
				.has_model = true,
				.model = {model, sizeof (model) - 1},
			},
			.has_instance = true,
			.instance = {TCG_CONCISE_EVIDENCE_TAGGED_UEID, {ueid, sizeof (ueid)}},
			.has_group = true,
			.group = {TCG_CONCISE_EVIDENCE_TAGGED_BYTES, {group, sizeof (group)}},
		},
		.measurements = &measurement,
		.measurement_count = 1,
	};
	struct tcg_concise_evidence_document document = {&triple, 1};
	const struct composite_eat_profile_evidence_policy policy = {
		.max_triples = 1,
		.max_measurements_per_triple = 1,
		.max_digests_per_measurement = 2,
		.max_identifier_length = 32,
		.max_text_length = 8,
		.max_encoded_length = 512,
		.allowed_mkey_types = COMPOSITE_EAT_PROFILE_MKEY_UUID |
			COMPOSITE_EAT_PROFILE_MKEY_OID,
		.allowed_algorithms = COMPOSITE_EAT_PROFILE_ALGORITHM_INTEGER |
			COMPOSITE_EAT_PROFILE_ALGORITHM_TEXT,
		.require_measurement_key = true,
		.require_class = true,
		.require_class_id = true,
	};
	size_t encoded_length;

	CHECK (tcg_concise_evidence_get_size (&document, &encoded_length) ==
		TCG_CONCISE_EVIDENCE_OK);
	CHECK (encoded_length > 0);
	measurement.key.type = TCG_CONCISE_EVIDENCE_MEASURED_ELEMENT_OID;
	measurement.key.value.bytes = (struct tcg_concise_evidence_bytes) {oid, sizeof (oid)};
	CHECK (tcg_concise_evidence_get_size (&document, &encoded_length) ==
		TCG_CONCISE_EVIDENCE_OK);
	CHECK (composite_eat_profile_validate_local_evidence (&document, &policy,
		&encoded_length) == COMPOSITE_EAT_PROFILE_EVIDENCE_POLICY_ERROR);

	triple.environment.instance.type = TCG_CONCISE_EVIDENCE_TAGGED_OID;
	triple.environment.instance.value =
		(struct tcg_concise_evidence_bytes) {oid, sizeof (oid)};
	CHECK (tcg_concise_evidence_validate (&document) ==
		TCG_CONCISE_EVIDENCE_BAD_ARGUMENT);
	return 0;
}

static int test_opaque_local_evidence (void)
{
	static const uint8_t ueid[] = {1, 1, 2, 3, 4, 5, 6};
	static const uint8_t profile[] = "https://example.org/profile";
	static const uint8_t evidence1[] = {0xa1, 0x00, 0x80};
	static const uint8_t evidence2[] = {0x80};
	static const uint8_t certificate[] = {0x30, 0x00};
	struct composite_eat_local_evidence local_evidence[] = {
		{.content_format = COMPOSITE_EAT_TCG_CONCISE_EVIDENCE_FORMAT,
			.encoded = {evidence1, sizeof (evidence1)}},
		{.content_format = 295, .encoded = {evidence2, sizeof (evidence2)}},
	};
	static const struct composite_eat_der_certificate certificates[] = {
		{certificate, sizeof (certificate)}
	};
	struct composite_eat_generation_request request = {0};
	struct composite_eat_evidence_snapshot evidence = {
		{ueid, sizeof (ueid)}, {profile, sizeof (profile) - 1}, local_evidence, 2
	};
	struct composite_eat_attestation_identity identity = {certificates, 1};
	struct composite_eat_main_token_workspace workspace;
	struct composite_eat_main_token_prepared prepared;
	uint8_t nonce[COMPOSITE_EAT_NONCE_LENGTH] = {0};
	uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH] = {0};
	uint8_t thumbprint[COMPOSITE_EAT_SHA384_LENGTH] = {0};
	uint8_t signature[COMPOSITE_EAT_ES384_SIGNATURE_LENGTH] = {0};
	uint8_t signing[1024];
	uint8_t response[1024];
	uint8_t streamed_response[1024];
	struct test_output signing_output = {signing, sizeof (signing), 0};
	struct test_evidence_source source = {evidence1, sizeof (evidence1)};
	size_t response_length;
	size_t streamed_response_length;

	request.version = COMPOSITE_EAT_GENERATION_REQUEST_VERSION;
	request.nonce = nonce;
	request.nonce_length = sizeof (nonce);
	request.records[0].environment = (const uint8_t *) "env.0";
	request.records[0].environment_length = sizeof ("env.0") - 1;
	request.records[0].digest = digest;
	request.records[0].digest_length = sizeof (digest);
	request.record_count = 1;

	CHECK (composite_eat_main_token_prepare (&request, &evidence, &identity, thumbprint,
		&workspace, &prepared) == COMPOSITE_EAT_MAIN_TOKEN_OK);
	CHECK (composite_eat_main_token_write_signing_input (&prepared, test_write,
		&signing_output) == COMPOSITE_EAT_MAIN_TOKEN_OK);
	CHECK (composite_eat_main_token_finish (&prepared, signature, response, sizeof (response),
		&response_length) == COMPOSITE_EAT_MAIN_TOKEN_OK);
	CHECK (response_length > 0);

	local_evidence[0].encoded = (struct composite_eat_buffer) {0};
	local_evidence[0].encoded_length = source.length;
	local_evidence[0].write = test_write_evidence;
	local_evidence[0].write_context = &source;
	signing_output.length = 0;
	CHECK (composite_eat_main_token_prepare (&request, &evidence, &identity, thumbprint,
		&workspace, &prepared) == COMPOSITE_EAT_MAIN_TOKEN_OK);
	CHECK (composite_eat_main_token_write_signing_input (&prepared, test_write,
		&signing_output) == COMPOSITE_EAT_MAIN_TOKEN_OK);
	CHECK (composite_eat_main_token_finish (&prepared, signature, streamed_response,
		sizeof (streamed_response), &streamed_response_length) ==
		COMPOSITE_EAT_MAIN_TOKEN_OK);
	CHECK (streamed_response_length == response_length);
	CHECK (memcmp (streamed_response, response, response_length) == 0);

	local_evidence[0].encoded_length++;
	signing_output.length = 0;
	CHECK (composite_eat_main_token_prepare (&request, &evidence, &identity, thumbprint,
		&workspace, &prepared) == COMPOSITE_EAT_MAIN_TOKEN_OK);
	CHECK (composite_eat_main_token_write_signing_input (&prepared, test_write,
		&signing_output) == COMPOSITE_EAT_MAIN_TOKEN_WRITE_ERROR);

	local_evidence[0].encoded.data = evidence1;
	local_evidence[0].encoded.length = 0;
	CHECK (composite_eat_main_token_prepare (&request, &evidence, &identity, thumbprint,
		&workspace, &prepared) == COMPOSITE_EAT_MAIN_TOKEN_BAD_ARGUMENT);
	return 0;
}

int main (void)
{
	CHECK (test_legacy_vector () == 0);
	CHECK (test_generic_document () == 0);
	CHECK (test_generic_identifier_and_algorithm_forms () == 0);
	CHECK (test_opaque_local_evidence () == 0);
	return 0;
}
