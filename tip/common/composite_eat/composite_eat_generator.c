/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdbool.h>
#include <string.h>

#include "asn1/ecc_der_util.h"
#include "status/rot_status.h"
#include "composite_eat/composite_eat_generator.h"
#include "qcbor/qcbor_encode.h"


#define COMPOSITE_EAT_PROFILE "tag:microsoft.com,2026:platform-composite-attestation-v1"
#define COMPOSITE_EAT_CONCISE_EVIDENCE_FORMAT 10571u
#define COMPOSITE_EAT_EVIDENCE_CLASS "tip-platform-state"
#define COMPOSITE_EAT_EVIDENCE_VENDOR "Microsoft/Nuvoton"
#define COMPOSITE_EAT_EVIDENCE_MODEL "M1120 BMC"
#define COMPOSITE_EAT_NI_SHA256 1
#define COMPOSITE_EAT_NI_SHA384 7

struct composite_eat_measurement {
	uint64_t key;
	uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH];
	size_t digest_length;
	int64_t algorithm;
};

#ifndef COMPOSITE_EAT_GENERATOR_NO_DEFAULT_ADAPTER
static const struct composite_eat_attestation_ops composite_eat_default_attestation = {
	.get_keys = riot_key_manager_get_riot_keys,
	.release_keys = riot_key_manager_release_riot_keys,
	.get_intermediate_ca = riot_key_manager_get_intermediate_ca,
	.get_root_ca = riot_key_manager_get_root_ca,
	.get_pcr_count = pcr_store_get_num_pcrs,
	.get_pcr_digest_length = pcr_store_get_pcr_digest_length,
	.compute_pcr = pcr_store_compute_pcr,
};
#endif

static size_t composite_eat_bstr_header (size_t length, uint8_t header[5])
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
	if (length < 0x10000) {
		header[0] = 0x59;
		header[1] = (uint8_t) (length >> 8);
		header[2] = (uint8_t) length;
		return 3;
	}

	header[0] = 0x5a;
	header[1] = (uint8_t) (length >> 24);
	header[2] = (uint8_t) (length >> 16);
	header[3] = (uint8_t) (length >> 8);
	header[4] = (uint8_t) length;
	return 5;
}

static int composite_eat_hash_sig_structure (const struct hash_engine *hash,
	const uint8_t *protected_headers, size_t protected_length, const uint8_t *payload,
	size_t payload_length, uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH])
{
	static const uint8_t prefix[] = {0x84, 0x6a, 'S', 'i', 'g', 'n', 'a', 't', 'u', 'r', 'e', '1'};
	static const uint8_t empty_external_aad = 0x40;
	uint8_t protected_bstr[5];
	uint8_t payload_bstr[5];
	size_t protected_bstr_length;
	size_t payload_bstr_length;
	int status;
	bool started = false;

	protected_bstr_length = composite_eat_bstr_header (protected_length, protected_bstr);
	payload_bstr_length = composite_eat_bstr_header (payload_length, payload_bstr);
	status = hash->start_sha384 (hash);
	if (status == 0) {
		started = true;
		status = hash->update (hash, prefix, sizeof (prefix));
	}
	if (status == 0) {
		status = hash->update (hash, protected_bstr, protected_bstr_length);
	}
	if (status == 0) {
		status = hash->update (hash, protected_headers, protected_length);
	}
	if (status == 0) {
		status = hash->update (hash, &empty_external_aad, sizeof (empty_external_aad));
	}
	if (status == 0) {
		status = hash->update (hash, payload_bstr, payload_bstr_length);
	}
	if (status == 0) {
		status = hash->update (hash, payload, payload_length);
	}
	if ((status != 0) && started) {
		hash->cancel (hash);
		return status;
	}
	if (status != 0) {
		return status;
	}

	status = hash->finish (hash, digest, COMPOSITE_EAT_SHA384_LENGTH);
	if (status != 0) {
		hash->cancel (hash);
	}
	return status;
}

static enum composite_eat_generator_status
composite_eat_get_measurements (const struct composite_eat_generator *generator,
	struct composite_eat_measurement measurements[COMPOSITE_EAT_MAX_PCR_DIGESTS],
	size_t *measurement_count)
{
	int count;
	int pcr;

	count = generator->attestation->get_pcr_count (generator->pcr_store);
	if ((count <= 0) || (count > (int) COMPOSITE_EAT_MAX_PCR_DIGESTS)) {
		return COMPOSITE_EAT_GENERATOR_ATTESTATION_ERROR;
	}

	for (pcr = 0; pcr < count; ++pcr) {
		int digest_length =
			generator->attestation->get_pcr_digest_length (generator->pcr_store, (uint8_t) pcr);

		if ((digest_length != SHA256_HASH_LENGTH) &&
			(digest_length != COMPOSITE_EAT_SHA384_LENGTH)) {
			return COMPOSITE_EAT_GENERATOR_ATTESTATION_ERROR;
		}
		if (generator->attestation->compute_pcr (generator->pcr_store, generator->hash,
				(uint8_t) pcr, measurements[pcr].digest, (size_t) digest_length) != digest_length) {
			return COMPOSITE_EAT_GENERATOR_ATTESTATION_ERROR;
		}
		measurements[pcr].key = (uint64_t) pcr;
		measurements[pcr].digest_length = (size_t) digest_length;
		measurements[pcr].algorithm = (digest_length == SHA256_HASH_LENGTH) ?
			COMPOSITE_EAT_NI_SHA256 :
			COMPOSITE_EAT_NI_SHA384;
	}

	*measurement_count = (size_t) count;
	return COMPOSITE_EAT_GENERATOR_OK;
}

static enum composite_eat_generator_status
composite_eat_encode_evidence (struct composite_eat_generator_workspace *workspace,
	const struct composite_eat_measurement *measurements, size_t measurement_count,
	size_t *evidence_length)
{
	QCBOREncodeContext encoder;
	UsefulBufC output;
	size_t i;

	QCBOREncode_Init (&encoder, (UsefulBuf){workspace->evidence, sizeof (workspace->evidence)});
	QCBOREncode_OpenMap (&encoder);
	QCBOREncode_OpenMapInMapN (&encoder, 0);
	QCBOREncode_OpenArrayInMapN (&encoder, 0);
	QCBOREncode_OpenArray (&encoder);
	QCBOREncode_OpenMap (&encoder);
	QCBOREncode_OpenMapInMapN (&encoder, 0);
	QCBOREncode_AddInt64 (&encoder, 0);
	QCBOREncode_AddTag (&encoder, 560);
	QCBOREncode_AddBytes (&encoder, UsefulBuf_FROM_SZ_LITERAL (COMPOSITE_EAT_EVIDENCE_CLASS));
	QCBOREncode_AddTextToMapN (&encoder, 1,
		UsefulBuf_FROM_SZ_LITERAL (COMPOSITE_EAT_EVIDENCE_VENDOR));
	QCBOREncode_AddTextToMapN (&encoder, 2,
		UsefulBuf_FROM_SZ_LITERAL (COMPOSITE_EAT_EVIDENCE_MODEL));
	QCBOREncode_CloseMap (&encoder);
	QCBOREncode_CloseMap (&encoder);
	QCBOREncode_OpenArray (&encoder);
	for (i = 0; i < measurement_count; ++i) {
		QCBOREncode_OpenMap (&encoder);
		QCBOREncode_AddUInt64ToMapN (&encoder, 0, measurements[i].key);
		QCBOREncode_OpenMapInMapN (&encoder, 1);
		QCBOREncode_OpenArrayInMapN (&encoder, 2);
		QCBOREncode_OpenArray (&encoder);
		QCBOREncode_AddInt64 (&encoder, measurements[i].algorithm);
		QCBOREncode_AddBytes (&encoder,
			(UsefulBufC){measurements[i].digest, measurements[i].digest_length});
		QCBOREncode_CloseArray (&encoder);
		QCBOREncode_CloseArray (&encoder);
		QCBOREncode_CloseMap (&encoder);
		QCBOREncode_CloseMap (&encoder);
	}
	QCBOREncode_CloseArray (&encoder);
	QCBOREncode_CloseArray (&encoder);
	QCBOREncode_CloseArray (&encoder);
	QCBOREncode_CloseMap (&encoder);
	QCBOREncode_CloseMap (&encoder);

	if (QCBOREncode_Finish (&encoder, &output) != QCBOR_SUCCESS) {
		return COMPOSITE_EAT_GENERATOR_ENCODING_ERROR;
	}
	*evidence_length = output.len;
	return COMPOSITE_EAT_GENERATOR_OK;
}

static enum composite_eat_generator_status
composite_eat_sign (const struct composite_eat_generator *generator, const struct riot_keys *keys,
	const uint8_t digest[COMPOSITE_EAT_SHA384_LENGTH],
	uint8_t signature[COMPOSITE_EAT_ES384_SIGNATURE_LENGTH])
{
	struct ecc_private_key private_key;
	uint8_t der_signature[ECC_DER_ECDSA_MAX_LENGTH];
	int der_length;
	int status;

	status = generator->ecc->init_key_pair (generator->ecc, keys->alias_key, keys->alias_key_length,
		&private_key, NULL);
	if (status != 0) {
		return COMPOSITE_EAT_GENERATOR_SIGN_ERROR;
	}

	der_length = generator->ecc->sign (generator->ecc, &private_key, digest,
		COMPOSITE_EAT_SHA384_LENGTH, NULL, der_signature, sizeof (der_signature));
	generator->ecc->release_key_pair (generator->ecc, &private_key, NULL);
	if (ROT_IS_ERROR (der_length) ||
		(ecc_der_decode_ecdsa_signature (der_signature, (size_t) der_length, signature,
			 &signature[COMPOSITE_EAT_SHA384_LENGTH], COMPOSITE_EAT_SHA384_LENGTH) != 0)) {
		return COMPOSITE_EAT_GENERATOR_SIGN_ERROR;
	}

	return COMPOSITE_EAT_GENERATOR_OK;
}

enum composite_eat_generator_status
composite_eat_generator_init (struct composite_eat_generator *generator,
	const struct ecc_engine *ecc, const struct hash_engine *hash,
	const struct riot_key_manager *riot, struct pcr_store *pcr_store,
	struct composite_eat_generator_workspace *workspace)
{
#ifdef COMPOSITE_EAT_GENERATOR_NO_DEFAULT_ADAPTER
	(void) generator;
	(void) ecc;
	(void) hash;
	(void) riot;
	(void) pcr_store;
	(void) workspace;
	return COMPOSITE_EAT_GENERATOR_BAD_ARGUMENT;
#else
	return composite_eat_generator_init_with_attestation (generator, ecc, hash, riot, pcr_store,
		workspace, &composite_eat_default_attestation);
#endif
}

enum composite_eat_generator_status
composite_eat_generator_init_with_attestation (struct composite_eat_generator *generator,
	const struct ecc_engine *ecc, const struct hash_engine *hash,
	const struct riot_key_manager *riot, struct pcr_store *pcr_store,
	struct composite_eat_generator_workspace *workspace,
	const struct composite_eat_attestation_ops *attestation)
{
	if ((generator == NULL) || (ecc == NULL) || (ecc->init_key_pair == NULL) ||
		(ecc->sign == NULL) || (ecc->release_key_pair == NULL) || (hash == NULL) ||
		(hash->calculate_sha384 == NULL) || (hash->start_sha384 == NULL) ||
		(hash->update == NULL) || (hash->finish == NULL) || (hash->cancel == NULL) ||
		(riot == NULL) || (pcr_store == NULL) || (workspace == NULL) || (attestation == NULL) ||
		(attestation->get_keys == NULL) || (attestation->release_keys == NULL) ||
		(attestation->get_intermediate_ca == NULL) || (attestation->get_root_ca == NULL) ||
		(attestation->get_pcr_count == NULL) || (attestation->get_pcr_digest_length == NULL) ||
		(attestation->compute_pcr == NULL)) {
		return COMPOSITE_EAT_GENERATOR_BAD_ARGUMENT;
	}

	generator->ecc = ecc;
	generator->hash = hash;
	generator->riot = riot;
	generator->pcr_store = pcr_store;
	generator->workspace = workspace;
	generator->attestation = attestation;
	memset (workspace, 0, sizeof (*workspace));
	return COMPOSITE_EAT_GENERATOR_OK;
}

enum composite_eat_generator_status
composite_eat_generate (const struct composite_eat_generator *generator, const uint8_t *request,
	size_t request_length, uint8_t *response, size_t response_capacity, size_t *response_length)
{
	struct composite_eat_generator_workspace *workspace;
	const struct riot_keys *keys;
	const struct der_cert *certificate;
	struct composite_eat_measurement measurements[COMPOSITE_EAT_MAX_PCR_DIGESTS];
	struct composite_eat_claims claims;
	struct composite_eat_cose_sign1 cose;
	uint8_t device_digest[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t certificate_digest[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t signature_digest[COMPOSITE_EAT_SHA384_LENGTH];
	uint8_t signature[COMPOSITE_EAT_ES384_SIGNATURE_LENGTH];
	uint8_t ueid[17];
	size_t measurement_count;
	size_t evidence_length;
	size_t payload_length;
	size_t protected_length;
	enum composite_eat_codec_status codec_status;
	enum composite_eat_generator_status status;

	if (response_length != NULL) {
		*response_length = 0;
	}
	if ((generator == NULL) || (generator->ecc == NULL) || (generator->hash == NULL) ||
		(generator->riot == NULL) || (generator->workspace == NULL) || (request == NULL) ||
		(request_length == 0) || (response_length == NULL) ||
		((response == NULL) && (response_capacity != 0))) {
		return COMPOSITE_EAT_GENERATOR_BAD_ARGUMENT;
	}

	workspace = generator->workspace;
	if (workspace->active) {
		return COMPOSITE_EAT_GENERATOR_BUSY;
	}
	workspace->active = true;
	codec_status =
		composite_eat_codec_decode_request (request, request_length, &workspace->request);
	if (codec_status != COMPOSITE_EAT_CODEC_OK) {
		workspace->active = false;
		if (codec_status == COMPOSITE_EAT_CODEC_BAD_VERSION) {
			return COMPOSITE_EAT_GENERATOR_BAD_VERSION;
		}
		if (codec_status == COMPOSITE_EAT_CODEC_TOO_MANY_RECORDS) {
			return COMPOSITE_EAT_GENERATOR_TOO_MANY_RECORDS;
		}
		return COMPOSITE_EAT_GENERATOR_BAD_REQUEST;
	}

	keys = generator->attestation->get_keys (generator->riot);
	if ((keys == NULL) || (keys->alias_key == NULL) || (keys->alias_cert == NULL) ||
		(keys->devid_cert == NULL) || (keys->alias_key_length == 0) ||
		(keys->alias_cert_length == 0) || (keys->devid_cert_length == 0)) {
		if (keys != NULL) {
			generator->attestation->release_keys (generator->riot, keys);
		}
		workspace->active = false;
		return COMPOSITE_EAT_GENERATOR_ATTESTATION_ERROR;
	}

	if ((generator->hash->calculate_sha384 (generator->hash, keys->devid_cert,
			 keys->devid_cert_length, device_digest, sizeof (device_digest)) != 0) ||
		(generator->hash->calculate_sha384 (generator->hash, keys->alias_cert,
			 keys->alias_cert_length, certificate_digest, sizeof (certificate_digest)) != 0)) {
		status = COMPOSITE_EAT_GENERATOR_HASH_ERROR;
		goto release_keys;
	}

	ueid[0] = 0x01;
	memcpy (&ueid[1], device_digest, sizeof (ueid) - 1);
	status = composite_eat_get_measurements (generator, measurements, &measurement_count);
	if (status != COMPOSITE_EAT_GENERATOR_OK) {
		goto release_keys;
	}
	status = composite_eat_encode_evidence (workspace, measurements, measurement_count,
		&evidence_length);
	if (status != COMPOSITE_EAT_GENERATOR_OK) {
		goto release_keys;
	}

	claims.ueid = (struct composite_eat_buffer){ueid, sizeof (ueid)};
	claims.profile = (struct composite_eat_buffer){(const uint8_t *) COMPOSITE_EAT_PROFILE,
		sizeof (COMPOSITE_EAT_PROFILE) - 1};
	claims.measurement_content_format = COMPOSITE_EAT_CONCISE_EVIDENCE_FORMAT;
	claims.measurements = (struct composite_eat_buffer){workspace->evidence, evidence_length};
	codec_status = composite_eat_codec_encode_payload (&workspace->request, &claims,
		workspace->payload, sizeof (workspace->payload), &payload_length);
	if (codec_status != COMPOSITE_EAT_CODEC_OK) {
		status = COMPOSITE_EAT_GENERATOR_ENCODING_ERROR;
		goto release_keys;
	}
	codec_status = composite_eat_codec_encode_protected_headers (certificate_digest,
		workspace->protected_headers, sizeof (workspace->protected_headers), &protected_length);
	if (codec_status != COMPOSITE_EAT_CODEC_OK) {
		status = COMPOSITE_EAT_GENERATOR_ENCODING_ERROR;
		goto release_keys;
	}
	if (composite_eat_hash_sig_structure (generator->hash, workspace->protected_headers,
			protected_length, workspace->payload, payload_length, signature_digest) != 0) {
		status = COMPOSITE_EAT_GENERATOR_HASH_ERROR;
		goto release_keys;
	}
	status = composite_eat_sign (generator, keys, signature_digest, signature);
	if (status != COMPOSITE_EAT_GENERATOR_OK) {
		goto release_keys;
	}

	memset (&cose, 0, sizeof (cose));
	cose.protected_headers =
		(struct composite_eat_buffer){workspace->protected_headers, protected_length};
	cose.payload = (struct composite_eat_buffer){workspace->payload, payload_length};
	cose.signature = (struct composite_eat_buffer){signature, sizeof (signature)};
	cose.certificates[cose.certificate_count++] =
		(struct composite_eat_buffer){keys->alias_cert, keys->alias_cert_length};
	cose.certificates[cose.certificate_count++] =
		(struct composite_eat_buffer){keys->devid_cert, keys->devid_cert_length};
	certificate = generator->attestation->get_intermediate_ca (generator->riot);
	if ((certificate != NULL) && (certificate->cert != NULL) && (certificate->length != 0)) {
		cose.certificates[cose.certificate_count++] =
			(struct composite_eat_buffer){certificate->cert, certificate->length};
	}
	certificate = generator->attestation->get_root_ca (generator->riot);
	if ((certificate != NULL) && (certificate->cert != NULL) && (certificate->length != 0)) {
		cose.certificates[cose.certificate_count++] =
			(struct composite_eat_buffer){certificate->cert, certificate->length};
	}

	codec_status =
		composite_eat_codec_encode_cose_sign1 (&cose, response, response_capacity, response_length);
	if (codec_status == COMPOSITE_EAT_CODEC_BUFFER_TOO_SMALL) {
		status = COMPOSITE_EAT_GENERATOR_BUFFER_TOO_SMALL;
	}
	else if (codec_status != COMPOSITE_EAT_CODEC_OK) {
		status = COMPOSITE_EAT_GENERATOR_ENCODING_ERROR;
		*response_length = 0;
	}
	else {
		status = COMPOSITE_EAT_GENERATOR_OK;
	}

release_keys:
	generator->attestation->release_keys (generator->riot, keys);
	workspace->active = false;
	return status;
}

enum composite_eat_generator_status
composite_eat_generator_snapshot_request (const struct composite_eat_generator *generator,
	const uint8_t *request, size_t request_length, const uint8_t **snapshot)
{
	if ((snapshot != NULL)) {
		*snapshot = NULL;
	}
	if ((generator == NULL) || (generator->workspace == NULL) || (request == NULL) ||
		(request_length == 0) || (request_length > sizeof (generator->workspace->payload)) ||
		(snapshot == NULL) || generator->workspace->active) {
		return COMPOSITE_EAT_GENERATOR_BAD_ARGUMENT;
	}

	memcpy (generator->workspace->payload, request, request_length);
	*snapshot = generator->workspace->payload;
	return COMPOSITE_EAT_GENERATOR_OK;
}