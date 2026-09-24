/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "asn1/ecc_der_util.h"
#include "status/rot_status.h"
#include "composite_eat/tip_main_token_generator.h"


#ifndef COMPOSITE_EAT_TIP_MAIN_TOKEN_NO_DEFAULT_OPS
static const struct composite_eat_tip_main_token_ops composite_eat_tip_main_token_default_ops = {
	.get_keys = riot_key_manager_get_riot_keys,
	.release_keys = riot_key_manager_release_riot_keys,
	.get_intermediate_ca = riot_key_manager_get_intermediate_ca,
	.get_root_ca = riot_key_manager_get_root_ca,
};
#endif

static void composite_eat_tip_main_token_clear_workspace (
	struct composite_eat_tip_main_token_workspace *workspace)
{
	volatile uint8_t *cursor = (volatile uint8_t *) workspace;
	size_t length = sizeof (*workspace);

	while (length-- > 0) {
		*cursor++ = 0;
	}
}

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_generator_init (
	struct composite_eat_tip_main_token_generator *generator, const struct ecc_engine *ecc,
	const struct hash_engine *hash, const struct riot_key_manager *riot,
	const struct composite_eat_tip_evidence_adapter *evidence,
	const struct composite_eat_buffer *profile,
	struct composite_eat_tip_main_token_workspace *workspace)
{
#ifdef COMPOSITE_EAT_TIP_MAIN_TOKEN_NO_DEFAULT_OPS
	(void) generator;
	(void) ecc;
	(void) hash;
	(void) riot;
	(void) evidence;
	(void) profile;
	(void) workspace;
	return COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_ARGUMENT;
#else
	return composite_eat_tip_main_token_generator_init_with_ops (generator, ecc, hash, riot,
		evidence, profile, workspace, &composite_eat_tip_main_token_default_ops);
#endif
}

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_generator_init_with_ops (
	struct composite_eat_tip_main_token_generator *generator, const struct ecc_engine *ecc,
	const struct hash_engine *hash, const struct riot_key_manager *riot,
	const struct composite_eat_tip_evidence_adapter *evidence,
	const struct composite_eat_buffer *profile,
	struct composite_eat_tip_main_token_workspace *workspace,
	const struct composite_eat_tip_main_token_ops *ops)
{
	if ((generator == NULL) || (ecc == NULL) || (ecc->init_key_pair == NULL) ||
		(ecc->sign == NULL) || (ecc->release_key_pair == NULL) || (hash == NULL) ||
		(hash->calculate_sha384 == NULL) || (hash->start_sha384 == NULL) ||
		(hash->update == NULL) || (hash->finish == NULL) || (hash->cancel == NULL) ||
		(riot == NULL) || (evidence == NULL) || (profile == NULL) ||
		(profile->data == NULL) || (profile->length == 0) ||
		(profile->length > COMPOSITE_EAT_MAX_PROFILE_LENGTH) || (workspace == NULL) ||
		(ops == NULL) || (ops->get_keys == NULL) || (ops->release_keys == NULL) ||
		(ops->get_intermediate_ca == NULL) || (ops->get_root_ca == NULL)) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_ARGUMENT;
	}

	generator->ecc = ecc;
	generator->hash = hash;
	generator->riot = riot;
	generator->evidence = evidence;
	generator->profile = *profile;
	generator->workspace = workspace;
	generator->ops = ops;
	composite_eat_tip_main_token_clear_workspace (workspace);
	return COMPOSITE_EAT_TIP_MAIN_TOKEN_OK;
}

static bool composite_eat_tip_main_token_add_certificate (
	struct composite_eat_tip_main_token_workspace *workspace, size_t *count,
	const uint8_t *certificate, size_t length)
{
	if ((certificate == NULL) || (length == 0) ||
		(length > COMPOSITE_EAT_MAX_CERTIFICATE_LENGTH) ||
		(*count >= COMPOSITE_EAT_MAX_CERTIFICATES)) {
		return false;
	}

	workspace->certificates[*count].data = certificate;
	workspace->certificates[*count].length = length;
	(*count)++;
	return true;
}

static enum composite_eat_tip_main_token_status composite_eat_tip_main_token_identity (
	const struct composite_eat_tip_main_token_generator *generator, const struct riot_keys *keys,
	struct composite_eat_attestation_identity *identity,
	struct composite_eat_evidence_snapshot *snapshot)
{
	struct composite_eat_tip_main_token_workspace *workspace = generator->workspace;
	const struct der_cert *certificate;
	size_t certificate_count = 0;

	if ((keys == NULL) || (keys->alias_key == NULL) || (keys->alias_key_length == 0) ||
		!composite_eat_tip_main_token_add_certificate (workspace, &certificate_count,
			keys->alias_cert, keys->alias_cert_length) ||
		!composite_eat_tip_main_token_add_certificate (workspace, &certificate_count,
			keys->devid_cert, keys->devid_cert_length)) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_IDENTITY_ERROR;
	}

	certificate = generator->ops->get_intermediate_ca (generator->riot);
	if ((certificate != NULL) && (certificate->cert != NULL) && (certificate->length != 0) &&
		!composite_eat_tip_main_token_add_certificate (workspace, &certificate_count,
			certificate->cert, certificate->length)) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_IDENTITY_ERROR;
	}
	certificate = generator->ops->get_root_ca (generator->riot);
	if ((certificate != NULL) && (certificate->cert != NULL) && (certificate->length != 0) &&
		!composite_eat_tip_main_token_add_certificate (workspace, &certificate_count,
			certificate->cert, certificate->length)) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_IDENTITY_ERROR;
	}

	if ((generator->hash->calculate_sha384 (generator->hash, keys->devid_cert,
		keys->devid_cert_length, workspace->signature_digest,
		sizeof (workspace->signature_digest)) != 0) ||
		(generator->hash->calculate_sha384 (generator->hash, keys->alias_cert,
			keys->alias_cert_length, workspace->leaf_thumbprint,
			sizeof (workspace->leaf_thumbprint)) != 0)) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_HASH_ERROR;
	}

	/* Nuvoton UEID type 1: a truncated SHA-384 commitment to the Device ID certificate. */
	workspace->ueid[0] = 0x01;
	memcpy (&workspace->ueid[1], workspace->signature_digest,
		sizeof (workspace->ueid) - 1);
	identity->certificates = workspace->certificates;
	identity->certificate_count = certificate_count;
	snapshot->ueid = (struct composite_eat_buffer){workspace->ueid, sizeof (workspace->ueid)};
	snapshot->profile = generator->profile;
	return COMPOSITE_EAT_TIP_MAIN_TOKEN_OK;
}

static int composite_eat_tip_main_token_hash_write (void *context, const uint8_t *data,
	size_t length)
{
	const struct hash_engine *hash = context;

	return hash->update (hash, data, length);
}

static enum composite_eat_tip_main_token_status composite_eat_tip_main_token_hash (
	const struct composite_eat_tip_main_token_generator *generator)
{
	struct composite_eat_tip_main_token_workspace *workspace = generator->workspace;
	enum composite_eat_main_token_status token_status;

	if (generator->hash->start_sha384 (generator->hash) != 0) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_HASH_ERROR;
	}
	token_status = composite_eat_main_token_write_signing_input (&workspace->prepared,
		composite_eat_tip_main_token_hash_write, (void *) generator->hash);
	if (token_status != COMPOSITE_EAT_MAIN_TOKEN_OK) {
		generator->hash->cancel (generator->hash);
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_HASH_ERROR;
	}
	if (generator->hash->finish (generator->hash, workspace->signature_digest,
		sizeof (workspace->signature_digest)) != 0) {
		generator->hash->cancel (generator->hash);
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_HASH_ERROR;
	}

	return COMPOSITE_EAT_TIP_MAIN_TOKEN_OK;
}

static enum composite_eat_tip_main_token_status composite_eat_tip_main_token_sign (
	const struct composite_eat_tip_main_token_generator *generator, const struct riot_keys *keys)
{
	struct composite_eat_tip_main_token_workspace *workspace = generator->workspace;
	struct ecc_private_key private_key;
	uint8_t der_signature[ECC_DER_ECDSA_MAX_LENGTH];
	int der_length;

	if (generator->ecc->init_key_pair (generator->ecc, keys->alias_key, keys->alias_key_length,
		&private_key, NULL) != 0) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_SIGN_ERROR;
	}

	der_length = generator->ecc->sign (generator->ecc, &private_key,
		workspace->signature_digest, sizeof (workspace->signature_digest), NULL,
		der_signature, sizeof (der_signature));
	generator->ecc->release_key_pair (generator->ecc, &private_key, NULL);
	if (ROT_IS_ERROR (der_length) ||
		(ecc_der_decode_ecdsa_signature (der_signature, (size_t) der_length,
			workspace->signature,
			&workspace->signature[COMPOSITE_EAT_SHA384_LENGTH],
			COMPOSITE_EAT_SHA384_LENGTH) != 0)) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_SIGN_ERROR;
	}

	return COMPOSITE_EAT_TIP_MAIN_TOKEN_OK;
}

struct composite_eat_tip_local_evidence_writer {
	const struct tcg_concise_evidence_document *document;
	size_t capacity;
};

static int composite_eat_tip_main_token_write_local_evidence (void *context,
	composite_eat_main_token_write write, void *write_context)
{
	const struct composite_eat_tip_local_evidence_writer *evidence = context;
	size_t encoded_length;

	return (tcg_concise_evidence_encode_write (evidence->document, write, write_context,
		evidence->capacity, &encoded_length) == TCG_CONCISE_EVIDENCE_OK) ? 0 : -1;
}

static enum composite_eat_tip_main_token_status composite_eat_tip_main_token_generate_internal (
	const struct composite_eat_tip_main_token_generator *generator, const uint8_t *request,
	size_t request_length, uint8_t *response, composite_eat_main_token_write write,
	void *context, size_t response_capacity, size_t *response_length)
{
	struct composite_eat_tip_main_token_workspace *workspace;
	struct composite_eat_attestation_identity identity;
	struct composite_eat_evidence_snapshot snapshot;
	struct composite_eat_tip_local_evidence concise_evidence;
	struct composite_eat_tip_local_evidence_writer evidence_writer;
	struct composite_eat_local_evidence local_evidence;
	const struct riot_keys *keys = NULL;
	enum composite_eat_generation_request_status request_status;
	enum composite_eat_main_token_status token_status;
	enum composite_eat_tip_main_token_status status;

	if (response_length != NULL) {
		*response_length = 0;
	}
	if ((generator == NULL) || (generator->workspace == NULL) || (request == NULL) ||
		(request_length == 0) || (response_length == NULL) ||
		((write == NULL) && (response == NULL) && (response_capacity != 0))) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_ARGUMENT;
	}

	workspace = generator->workspace;
	if (workspace->active) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_BUSY;
	}
	workspace->active = true;

	request_status = composite_eat_generation_request_decode (request, request_length,
		&workspace->request);
	if (request_status != COMPOSITE_EAT_GENERATION_REQUEST_OK) {
		if (request_status == COMPOSITE_EAT_GENERATION_REQUEST_BAD_VERSION) {
			status = COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_VERSION;
		}
		else if (request_status == COMPOSITE_EAT_GENERATION_REQUEST_TOO_MANY_RECORDS) {
			status = COMPOSITE_EAT_TIP_MAIN_TOKEN_TOO_MANY_RECORDS;
		}
		else {
			status = COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_REQUEST;
		}
		goto cleanup;
	}
	composite_eat_tip_evidence_clear (&workspace->evidence);
	composite_eat_main_token_clear (&workspace->token, &workspace->prepared);
	memset (workspace->certificates, 0, sizeof (workspace->certificates));
	memset (workspace->ueid, 0, sizeof (workspace->ueid));
	memset (workspace->leaf_thumbprint, 0, sizeof (workspace->leaf_thumbprint));
	memset (workspace->signature_digest, 0, sizeof (workspace->signature_digest));
	memset (workspace->signature, 0, sizeof (workspace->signature));

	keys = generator->ops->get_keys (generator->riot);
	memset (&identity, 0, sizeof (identity));
	memset (&snapshot, 0, sizeof (snapshot));
	memset (&local_evidence, 0, sizeof (local_evidence));
	status = composite_eat_tip_main_token_identity (generator, keys, &identity, &snapshot);
	if (status != COMPOSITE_EAT_TIP_MAIN_TOKEN_OK) {
		goto cleanup;
	}

	if (composite_eat_tip_evidence_capture (generator->evidence, &workspace->evidence,
		&concise_evidence) != COMPOSITE_EAT_TIP_EVIDENCE_OK) {
		status = COMPOSITE_EAT_TIP_MAIN_TOKEN_EVIDENCE_ERROR;
		goto cleanup;
	}
	local_evidence.content_format = COMPOSITE_EAT_TCG_CONCISE_EVIDENCE_FORMAT;
	local_evidence.encoded_length = concise_evidence.encoded_length;
	local_evidence.write = composite_eat_tip_main_token_write_local_evidence;
	evidence_writer.document = concise_evidence.document;
	evidence_writer.capacity = concise_evidence.encoded_length;
	local_evidence.write_context = &evidence_writer;
	snapshot.local_evidence = &local_evidence;
	snapshot.local_evidence_count = 1;
	token_status = composite_eat_main_token_prepare (&workspace->request, &snapshot, &identity,
		workspace->leaf_thumbprint, &workspace->token, &workspace->prepared);
	if (token_status != COMPOSITE_EAT_MAIN_TOKEN_OK) {
		status = COMPOSITE_EAT_TIP_MAIN_TOKEN_ENCODING_ERROR;
		goto cleanup;
	}
	if (workspace->prepared.response_length > COMPOSITE_EAT_TIP_MAX_RESPONSE_LENGTH) {
		*response_length = workspace->prepared.response_length;
		status = COMPOSITE_EAT_TIP_MAIN_TOKEN_RESPONSE_TOO_LARGE;
		goto cleanup;
	}
	if (workspace->prepared.response_length > response_capacity) {
		*response_length = workspace->prepared.response_length;
		status = COMPOSITE_EAT_TIP_MAIN_TOKEN_BUFFER_TOO_SMALL;
		goto cleanup;
	}

	status = composite_eat_tip_main_token_hash (generator);
	if (status != COMPOSITE_EAT_TIP_MAIN_TOKEN_OK) {
		goto cleanup;
	}
	status = composite_eat_tip_main_token_sign (generator, keys);
	if (status != COMPOSITE_EAT_TIP_MAIN_TOKEN_OK) {
		goto cleanup;
	}

	if (write != NULL) {
		token_status = composite_eat_main_token_finish_write (&workspace->prepared,
			workspace->signature, write, context, response_capacity, response_length);
	}
	else {
		token_status = composite_eat_main_token_finish (&workspace->prepared,
			workspace->signature, response, response_capacity, response_length);
	}
	if (token_status == COMPOSITE_EAT_MAIN_TOKEN_BUFFER_TOO_SMALL) {
		status = COMPOSITE_EAT_TIP_MAIN_TOKEN_BUFFER_TOO_SMALL;
	}
	else if (token_status == COMPOSITE_EAT_MAIN_TOKEN_WRITE_ERROR) {
		status = COMPOSITE_EAT_TIP_MAIN_TOKEN_CANCELED;
	}
	else if (token_status != COMPOSITE_EAT_MAIN_TOKEN_OK) {
		status = COMPOSITE_EAT_TIP_MAIN_TOKEN_ENCODING_ERROR;
		*response_length = 0;
	}
	else {
		status = COMPOSITE_EAT_TIP_MAIN_TOKEN_OK;
	}

cleanup:
	if (keys != NULL) {
		generator->ops->release_keys (generator->riot, keys);
	}
	composite_eat_tip_main_token_clear_workspace (workspace);
	return status;
}

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_generate (
	const struct composite_eat_tip_main_token_generator *generator, const uint8_t *request,
	size_t request_length, uint8_t *response, size_t response_capacity,
	size_t *response_length)
{
	return composite_eat_tip_main_token_generate_internal (generator, request, request_length,
		response, NULL, NULL, response_capacity, response_length);
}

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_generate_write (
	const struct composite_eat_tip_main_token_generator *generator, const uint8_t *request,
	size_t request_length, composite_eat_main_token_write write, void *context,
	size_t response_capacity, size_t *response_length)
{
	if (write == NULL) {
		if (response_length != NULL) {
			*response_length = 0;
		}
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_ARGUMENT;
	}

	return composite_eat_tip_main_token_generate_internal (generator, request, request_length,
		NULL, write, context, response_capacity, response_length);
}

enum composite_eat_tip_main_token_status composite_eat_tip_main_token_snapshot_request (
	const struct composite_eat_tip_main_token_generator *generator, const uint8_t *request,
	size_t request_length, const uint8_t **snapshot)
{
	if (snapshot != NULL) {
		*snapshot = NULL;
	}
	if ((generator == NULL) || (generator->workspace == NULL) || (request == NULL) ||
		(request_length == 0) ||
		(request_length > sizeof (generator->workspace->request_snapshot)) ||
		(snapshot == NULL) || generator->workspace->active) {
		return COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_ARGUMENT;
	}

	memcpy (generator->workspace->request_snapshot, request, request_length);
	*snapshot = generator->workspace->request_snapshot;
	return COMPOSITE_EAT_TIP_MAIN_TOKEN_OK;
}