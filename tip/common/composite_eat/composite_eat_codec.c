/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdbool.h>
#include <string.h>

#include "composite_eat/composite_eat_codec.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_encode.h"


#define COMPOSITE_EAT_REQUEST_KEY_VERSION 1
#define COMPOSITE_EAT_REQUEST_KEY_NONCE 2
#define COMPOSITE_EAT_REQUEST_KEY_RECORDS 3

#define COMPOSITE_EAT_RECORD_KEY_ENVIRONMENT 1
#define COMPOSITE_EAT_RECORD_KEY_DIGEST 2

#define COMPOSITE_EAT_CLAIM_NONCE 10
#define COMPOSITE_EAT_CLAIM_UEID 256
#define COMPOSITE_EAT_CLAIM_PROFILE 265
#define COMPOSITE_EAT_CLAIM_SUBMODULES 266
#define COMPOSITE_EAT_CLAIM_MEASUREMENTS 273

#define COMPOSITE_EAT_COSE_HEADER_ALGORITHM 1
#define COMPOSITE_EAT_COSE_HEADER_X5CHAIN 33
#define COMPOSITE_EAT_COSE_HEADER_X5T 34

#define COMPOSITE_EAT_REQUEST_FIELD_COUNT 3u
#define COMPOSITE_EAT_RECORD_FIELD_COUNT 2u


static bool composite_eat_codec_buffer_valid (const struct composite_eat_buffer *buffer,
	bool allow_empty)
{
	return (buffer != NULL) && ((buffer->data != NULL) || (allow_empty && (buffer->length == 0))) &&
		(allow_empty || (buffer->length != 0));
}

static bool composite_eat_codec_output_valid (uint8_t *encoded, size_t encoded_capacity,
	size_t *encoded_length)
{
	return (encoded_length != NULL) && ((encoded != NULL) || (encoded_capacity == 0));
}

static bool composite_eat_codec_utf8_valid (const uint8_t *text, size_t length)
{
	size_t i = 0;

	while (i < length) {
		if (text[i] <= 0x7f) {
			i++;
		}
		else if ((text[i] >= 0xc2) && (text[i] <= 0xdf) && ((length - i) >= 2) &&
			((text[i + 1] & 0xc0) == 0x80)) {
			i += 2;
		}
		else if ((text[i] == 0xe0) && ((length - i) >= 3) && (text[i + 1] >= 0xa0) &&
			(text[i + 1] <= 0xbf) && ((text[i + 2] & 0xc0) == 0x80)) {
			i += 3;
		}
		else if (((((text[i] >= 0xe1) && (text[i] <= 0xec)) ||
					  ((text[i] >= 0xee) && (text[i] <= 0xef))) &&
					 ((length - i) >= 3) && ((text[i + 1] & 0xc0) == 0x80) &&
					 ((text[i + 2] & 0xc0) == 0x80))) {
			i += 3;
		}
		else if ((text[i] == 0xed) && ((length - i) >= 3) && (text[i + 1] >= 0x80) &&
			(text[i + 1] <= 0x9f) && ((text[i + 2] & 0xc0) == 0x80)) {
			i += 3;
		}
		else if ((text[i] == 0xf0) && ((length - i) >= 4) && (text[i + 1] >= 0x90) &&
			(text[i + 1] <= 0xbf) && ((text[i + 2] & 0xc0) == 0x80) &&
			((text[i + 3] & 0xc0) == 0x80)) {
			i += 4;
		}
		else if ((text[i] >= 0xf1) && (text[i] <= 0xf3) && ((length - i) >= 4) &&
			((text[i + 1] & 0xc0) == 0x80) && ((text[i + 2] & 0xc0) == 0x80) &&
			((text[i + 3] & 0xc0) == 0x80)) {
			i += 4;
		}
		else if ((text[i] == 0xf4) && ((length - i) >= 4) && (text[i + 1] >= 0x80) &&
			(text[i + 1] <= 0x8f) && ((text[i + 2] & 0xc0) == 0x80) &&
			((text[i + 3] & 0xc0) == 0x80)) {
			i += 4;
		}
		else {
			return false;
		}
	}

	return true;
}

static bool composite_eat_codec_request_valid (const struct composite_eat_request *request)
{
	size_t i;
	size_t j;

	if ((request == NULL) || (request->version != COMPOSITE_EAT_REQUEST_VERSION) ||
		(request->nonce_length < COMPOSITE_EAT_MIN_NONCE_LENGTH) ||
		(request->nonce_length > COMPOSITE_EAT_MAX_NONCE_LENGTH) ||
		(request->record_count > COMPOSITE_EAT_MAX_DEVICE_RECORDS)) {
		return false;
	}

	for (i = 0; i < request->record_count; ++i) {
		if ((request->records[i].environment_length == 0) ||
			(request->records[i].environment_length > COMPOSITE_EAT_MAX_ENVIRONMENT_LENGTH) ||
			(memchr (request->records[i].environment, '\0',
				 request->records[i].environment_length) != NULL) ||
			!composite_eat_codec_utf8_valid (request->records[i].environment,
				request->records[i].environment_length)) {
			return false;
		}

		for (j = 0; j < i; ++j) {
			if ((request->records[i].environment_length ==
					request->records[j].environment_length) &&
				(memcmp (request->records[i].environment, request->records[j].environment,
					 request->records[i].environment_length) == 0)) {
				return false;
			}
		}
	}

	return true;
}

static enum composite_eat_codec_status composite_eat_codec_get_next (QCBORDecodeContext *decoder,
	QCBORItem *item)
{
	if ((QCBORDecode_GetNext (decoder, item) != QCBOR_SUCCESS) ||
		(item->uTags[0] != CBOR_TAG_INVALID16)) {
		return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
	}

	return COMPOSITE_EAT_CODEC_OK;
}

static bool composite_eat_codec_integer_label (const QCBORItem *item, int64_t expected)
{
	return (item->uLabelType == QCBOR_TYPE_INT64) && (item->label.int64 == expected);
}

static enum composite_eat_codec_status
composite_eat_codec_decode_digest (QCBORDecodeContext *decoder, const QCBORItem *container,
	struct composite_eat_device_record *record)
{
	QCBORItem item;
	enum composite_eat_codec_status status;

	if ((container->uDataType != QCBOR_TYPE_ARRAY) || (container->val.uCount != 2) ||
		(container->uNestingLevel != 3)) {
		return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
	}

	status = composite_eat_codec_get_next (decoder, &item);
	if ((status != COMPOSITE_EAT_CODEC_OK) || (item.uNestingLevel != 4) ||
		(item.uLabelType != QCBOR_TYPE_NONE) || (item.uDataType != QCBOR_TYPE_INT64) ||
		(item.val.int64 != COMPOSITE_EAT_COSE_ALGORITHM_SHA384)) {
		return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
	}

	status = composite_eat_codec_get_next (decoder, &item);
	if ((status != COMPOSITE_EAT_CODEC_OK) || (item.uNestingLevel != 4) ||
		(item.uLabelType != QCBOR_TYPE_NONE) || (item.uDataType != QCBOR_TYPE_BYTE_STRING) ||
		(item.val.string.len != COMPOSITE_EAT_SHA384_LENGTH)) {
		return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
	}

	memcpy (record->digest, item.val.string.ptr, sizeof (record->digest));
	return COMPOSITE_EAT_CODEC_OK;
}

static enum composite_eat_codec_status
composite_eat_codec_decode_record (QCBORDecodeContext *decoder,
	struct composite_eat_request *request, size_t record_index)
{
	struct composite_eat_device_record *record = &request->records[record_index];
	QCBORItem item;
	bool environment_seen = false;
	bool digest_seen = false;
	size_t i;
	size_t field;
	enum composite_eat_codec_status status;

	status = composite_eat_codec_get_next (decoder, &item);
	if ((status != COMPOSITE_EAT_CODEC_OK) || (item.uNestingLevel != 2) ||
		(item.uLabelType != QCBOR_TYPE_NONE) || (item.uDataType != QCBOR_TYPE_MAP) ||
		(item.val.uCount != COMPOSITE_EAT_RECORD_FIELD_COUNT)) {
		return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
	}

	for (field = 0; field < COMPOSITE_EAT_RECORD_FIELD_COUNT; ++field) {
		status = composite_eat_codec_get_next (decoder, &item);
		if ((status != COMPOSITE_EAT_CODEC_OK) || (item.uNestingLevel != 3)) {
			return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
		}

		if (composite_eat_codec_integer_label (&item, COMPOSITE_EAT_RECORD_KEY_ENVIRONMENT)) {
			if (environment_seen || (item.uDataType != QCBOR_TYPE_TEXT_STRING) ||
				(item.val.string.len == 0) ||
				(item.val.string.len > COMPOSITE_EAT_MAX_ENVIRONMENT_LENGTH) ||
				(memchr (item.val.string.ptr, '\0', item.val.string.len) != NULL) ||
				!composite_eat_codec_utf8_valid (item.val.string.ptr, item.val.string.len)) {
				return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
			}

			record->environment_length = item.val.string.len;
			memcpy (record->environment, item.val.string.ptr, record->environment_length);
			environment_seen = true;
		}
		else if (composite_eat_codec_integer_label (&item, COMPOSITE_EAT_RECORD_KEY_DIGEST)) {
			if (digest_seen) {
				return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
			}

			status = composite_eat_codec_decode_digest (decoder, &item, record);
			if (status != COMPOSITE_EAT_CODEC_OK) {
				return status;
			}
			digest_seen = true;
		}
		else {
			return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
		}
	}

	if (!environment_seen || !digest_seen) {
		return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
	}

	for (i = 0; i < record_index; ++i) {
		if ((record->environment_length == request->records[i].environment_length) &&
			(memcmp (record->environment, request->records[i].environment,
				 record->environment_length) == 0)) {
			return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
		}
	}

	return COMPOSITE_EAT_CODEC_OK;
}

static enum composite_eat_codec_status
composite_eat_codec_decode_records (QCBORDecodeContext *decoder, const QCBORItem *container,
	struct composite_eat_request *request)
{
	size_t i;
	enum composite_eat_codec_status status;

	if ((container->uDataType != QCBOR_TYPE_ARRAY) || (container->uNestingLevel != 1) ||
		(container->val.uCount == QCBOR_COUNT_INDICATES_INDEFINITE_LENGTH)) {
		return COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
	}
	if (container->val.uCount > COMPOSITE_EAT_MAX_DEVICE_RECORDS) {
		return COMPOSITE_EAT_CODEC_TOO_MANY_RECORDS;
	}

	request->record_count = container->val.uCount;
	for (i = 0; i < request->record_count; ++i) {
		status = composite_eat_codec_decode_record (decoder, request, i);
		if (status != COMPOSITE_EAT_CODEC_OK) {
			return status;
		}
	}

	return COMPOSITE_EAT_CODEC_OK;
}

enum composite_eat_codec_status composite_eat_codec_decode_request (const uint8_t *encoded,
	size_t encoded_length, struct composite_eat_request *request)
{
	QCBORDecodeContext decoder;
	QCBORItem item;
	bool version_seen = false;
	bool nonce_seen = false;
	bool records_seen = false;
	size_t field;
	enum composite_eat_codec_status status = COMPOSITE_EAT_CODEC_OK;

	if ((encoded == NULL) || (encoded_length == 0) || (request == NULL)) {
		return COMPOSITE_EAT_CODEC_BAD_ARGUMENT;
	}

	memset (request, 0, sizeof (*request));
	QCBORDecode_Init (&decoder, (UsefulBufC){encoded, encoded_length}, QCBOR_DECODE_MODE_NORMAL);

	status = composite_eat_codec_get_next (&decoder, &item);
	if ((status != COMPOSITE_EAT_CODEC_OK) || (item.uNestingLevel != 0) ||
		(item.uLabelType != QCBOR_TYPE_NONE) || (item.uDataType != QCBOR_TYPE_MAP) ||
		(item.val.uCount != COMPOSITE_EAT_REQUEST_FIELD_COUNT)) {
		status = COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
		goto error;
	}

	for (field = 0; field < COMPOSITE_EAT_REQUEST_FIELD_COUNT; ++field) {
		status = composite_eat_codec_get_next (&decoder, &item);
		if ((status != COMPOSITE_EAT_CODEC_OK) || (item.uNestingLevel != 1)) {
			status = COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
			goto error;
		}

		if (composite_eat_codec_integer_label (&item, COMPOSITE_EAT_REQUEST_KEY_VERSION)) {
			if (version_seen || (item.uDataType != QCBOR_TYPE_INT64)) {
				status = COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
				goto error;
			}
			if (item.val.int64 != COMPOSITE_EAT_REQUEST_VERSION) {
				status = COMPOSITE_EAT_CODEC_BAD_VERSION;
				goto error;
			}
			request->version = (uint32_t) item.val.int64;
			version_seen = true;
		}
		else if (composite_eat_codec_integer_label (&item, COMPOSITE_EAT_REQUEST_KEY_NONCE)) {
			if (nonce_seen || (item.uDataType != QCBOR_TYPE_BYTE_STRING) ||
				(item.val.string.len < COMPOSITE_EAT_MIN_NONCE_LENGTH) ||
				(item.val.string.len > COMPOSITE_EAT_MAX_NONCE_LENGTH)) {
				status = COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
				goto error;
			}
			request->nonce_length = item.val.string.len;
			memcpy (request->nonce, item.val.string.ptr, request->nonce_length);
			nonce_seen = true;
		}
		else if (composite_eat_codec_integer_label (&item, COMPOSITE_EAT_REQUEST_KEY_RECORDS)) {
			if (records_seen) {
				status = COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
				goto error;
			}
			status = composite_eat_codec_decode_records (&decoder, &item, request);
			if (status != COMPOSITE_EAT_CODEC_OK) {
				goto error;
			}
			records_seen = true;
		}
		else {
			status = COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
			goto error;
		}
	}

	if (!version_seen || !nonce_seen || !records_seen ||
		(QCBORDecode_Finish (&decoder) != QCBOR_SUCCESS)) {
		status = COMPOSITE_EAT_CODEC_MALFORMED_REQUEST;
		goto error;
	}

	return COMPOSITE_EAT_CODEC_OK;

error:
	memset (request, 0, sizeof (*request));
	return status;
}

static void composite_eat_codec_add_request (QCBOREncodeContext *encoder,
	const struct composite_eat_request *request)
{
	size_t i;

	QCBOREncode_OpenMap (encoder);
	QCBOREncode_AddInt64ToMapN (encoder, COMPOSITE_EAT_REQUEST_KEY_VERSION, request->version);
	QCBOREncode_AddBytesToMapN (encoder, COMPOSITE_EAT_REQUEST_KEY_NONCE,
		(UsefulBufC){request->nonce, request->nonce_length});
	QCBOREncode_OpenArrayInMapN (encoder, COMPOSITE_EAT_REQUEST_KEY_RECORDS);
	for (i = 0; i < request->record_count; ++i) {
		QCBOREncode_OpenMap (encoder);
		QCBOREncode_AddTextToMapN (encoder, COMPOSITE_EAT_RECORD_KEY_ENVIRONMENT,
			(UsefulBufC){request->records[i].environment, request->records[i].environment_length});
		QCBOREncode_OpenArrayInMapN (encoder, COMPOSITE_EAT_RECORD_KEY_DIGEST);
		QCBOREncode_AddInt64 (encoder, COMPOSITE_EAT_COSE_ALGORITHM_SHA384);
		QCBOREncode_AddBytes (encoder,
			(UsefulBufC){request->records[i].digest, sizeof (request->records[i].digest)});
		QCBOREncode_CloseArray (encoder);
		QCBOREncode_CloseMap (encoder);
	}
	QCBOREncode_CloseArray (encoder);
	QCBOREncode_CloseMap (encoder);
}

static void composite_eat_codec_add_payload (QCBOREncodeContext *encoder,
	const struct composite_eat_request *request, const struct composite_eat_claims *claims)
{
	size_t i;
	size_t j;
	uint8_t order[COMPOSITE_EAT_MAX_DEVICE_RECORDS];
	char environment[COMPOSITE_EAT_MAX_ENVIRONMENT_LENGTH + 1];

	for (i = 0; i < request->record_count; ++i) {
		order[i] = (uint8_t) i;
		for (j = i; j > 0; --j) {
			const struct composite_eat_device_record *left = &request->records[order[j - 1]];
			const struct composite_eat_device_record *right = &request->records[order[j]];
			int comparison;

			if (left->environment_length != right->environment_length) {
				comparison = (left->environment_length < right->environment_length) ? -1 : 1;
			}
			else {
				comparison =
					memcmp (left->environment, right->environment, left->environment_length);
			}
			if (comparison <= 0) {
				break;
			}

			order[j] = order[j - 1];
			order[j - 1] = (uint8_t) i;
		}
	}

	QCBOREncode_OpenMap (encoder);
	QCBOREncode_AddBytesToMapN (encoder, COMPOSITE_EAT_CLAIM_NONCE,
		(UsefulBufC){request->nonce, request->nonce_length});
	QCBOREncode_AddBytesToMapN (encoder, COMPOSITE_EAT_CLAIM_UEID,
		(UsefulBufC){claims->ueid.data, claims->ueid.length});
	QCBOREncode_AddTextToMapN (encoder, COMPOSITE_EAT_CLAIM_PROFILE,
		(UsefulBufC){claims->profile.data, claims->profile.length});
	QCBOREncode_OpenMapInMapN (encoder, COMPOSITE_EAT_CLAIM_SUBMODULES);
	for (i = 0; i < request->record_count; ++i) {
		const struct composite_eat_device_record *record = &request->records[order[i]];

		memcpy (environment, record->environment, record->environment_length);
		environment[record->environment_length] = '\0';
		QCBOREncode_OpenArrayInMap (encoder, environment);
		QCBOREncode_AddInt64 (encoder, COMPOSITE_EAT_COSE_ALGORITHM_SHA384);
		QCBOREncode_AddBytes (encoder, (UsefulBufC){record->digest, sizeof (record->digest)});
		QCBOREncode_CloseArray (encoder);
	}
	QCBOREncode_CloseMap (encoder);
	QCBOREncode_OpenArrayInMapN (encoder, COMPOSITE_EAT_CLAIM_MEASUREMENTS);
	QCBOREncode_OpenArray (encoder);
	QCBOREncode_AddUInt64 (encoder, claims->measurement_content_format);
	QCBOREncode_AddBytes (encoder,
		(UsefulBufC){claims->measurements.data, claims->measurements.length});
	QCBOREncode_CloseArray (encoder);
	QCBOREncode_CloseArray (encoder);
	QCBOREncode_CloseMap (encoder);
}

static void composite_eat_codec_add_protected_headers (QCBOREncodeContext *encoder,
	const uint8_t certificate_thumbprint[COMPOSITE_EAT_SHA384_LENGTH])
{
	QCBOREncode_OpenMap (encoder);
	QCBOREncode_AddInt64ToMapN (encoder, COMPOSITE_EAT_COSE_HEADER_ALGORITHM,
		COMPOSITE_EAT_COSE_ALGORITHM_ES384);
	QCBOREncode_OpenArrayInMapN (encoder, COMPOSITE_EAT_COSE_HEADER_X5T);
	QCBOREncode_AddInt64 (encoder, COMPOSITE_EAT_COSE_ALGORITHM_SHA384);
	QCBOREncode_AddBytes (encoder,
		(UsefulBufC){certificate_thumbprint, COMPOSITE_EAT_SHA384_LENGTH});
	QCBOREncode_CloseArray (encoder);
	QCBOREncode_CloseMap (encoder);
}

static void composite_eat_codec_add_cose_sign1 (QCBOREncodeContext *encoder,
	const struct composite_eat_cose_sign1 *cose)
{
	size_t i;

	QCBOREncode_AddTag (encoder, CBOR_TAG_COSE_SIGN1);
	QCBOREncode_OpenArray (encoder);
	QCBOREncode_AddBytes (encoder,
		(UsefulBufC){cose->protected_headers.data, cose->protected_headers.length});
	QCBOREncode_OpenMap (encoder);
	if (cose->certificate_count == 1) {
		QCBOREncode_AddBytesToMapN (encoder, COMPOSITE_EAT_COSE_HEADER_X5CHAIN,
			(UsefulBufC){cose->certificates[0].data, cose->certificates[0].length});
	}
	else {
		QCBOREncode_OpenArrayInMapN (encoder, COMPOSITE_EAT_COSE_HEADER_X5CHAIN);
		for (i = 0; i < cose->certificate_count; ++i) {
			QCBOREncode_AddBytes (encoder,
				(UsefulBufC){cose->certificates[i].data, cose->certificates[i].length});
		}
		QCBOREncode_CloseArray (encoder);
	}
	QCBOREncode_CloseMap (encoder);
	QCBOREncode_AddBytes (encoder, (UsefulBufC){cose->payload.data, cose->payload.length});
	QCBOREncode_AddBytes (encoder, (UsefulBufC){cose->signature.data, cose->signature.length});
	QCBOREncode_CloseArray (encoder);
}

static enum composite_eat_codec_status composite_eat_codec_finish_size (QCBOREncodeContext *encoder,
	size_t *encoded_length)
{
	if (QCBOREncode_FinishGetSize (encoder, encoded_length) != QCBOR_SUCCESS) {
		*encoded_length = 0;
		return COMPOSITE_EAT_CODEC_ENCODING_ERROR;
	}

	return COMPOSITE_EAT_CODEC_OK;
}

static enum composite_eat_codec_status
composite_eat_codec_finish_output (QCBOREncodeContext *encoder, size_t expected_length)
{
	UsefulBufC encoded;

	if ((QCBOREncode_Finish (encoder, &encoded) != QCBOR_SUCCESS) ||
		(encoded.len != expected_length)) {
		return COMPOSITE_EAT_CODEC_ENCODING_ERROR;
	}

	return COMPOSITE_EAT_CODEC_OK;
}

enum composite_eat_codec_status
composite_eat_codec_encode_request (const struct composite_eat_request *request, uint8_t *encoded,
	size_t encoded_capacity, size_t *encoded_length)
{
	QCBOREncodeContext encoder;
	enum composite_eat_codec_status status;

	if (!composite_eat_codec_output_valid (encoded, encoded_capacity, encoded_length) ||
		!composite_eat_codec_request_valid (request)) {
		return COMPOSITE_EAT_CODEC_BAD_ARGUMENT;
	}

	*encoded_length = 0;
	QCBOREncode_Init (&encoder, SizeCalculateUsefulBuf);
	composite_eat_codec_add_request (&encoder, request);
	status = composite_eat_codec_finish_size (&encoder, encoded_length);
	if (status != COMPOSITE_EAT_CODEC_OK) {
		return status;
	}
	if (encoded_capacity < *encoded_length) {
		return COMPOSITE_EAT_CODEC_BUFFER_TOO_SMALL;
	}

	QCBOREncode_Init (&encoder, (UsefulBuf){encoded, encoded_capacity});
	composite_eat_codec_add_request (&encoder, request);
	return composite_eat_codec_finish_output (&encoder, *encoded_length);
}

enum composite_eat_codec_status
composite_eat_codec_encode_payload (const struct composite_eat_request *request,
	const struct composite_eat_claims *claims, uint8_t *encoded, size_t encoded_capacity,
	size_t *encoded_length)
{
	QCBOREncodeContext encoder;
	enum composite_eat_codec_status status;

	if (!composite_eat_codec_output_valid (encoded, encoded_capacity, encoded_length) ||
		!composite_eat_codec_request_valid (request) || (claims == NULL) ||
		!composite_eat_codec_buffer_valid (&claims->ueid, false) ||
		!composite_eat_codec_buffer_valid (&claims->profile, false) ||
		!composite_eat_codec_buffer_valid (&claims->measurements, false) ||
		!composite_eat_codec_utf8_valid (claims->profile.data, claims->profile.length)) {
		return COMPOSITE_EAT_CODEC_BAD_ARGUMENT;
	}

	*encoded_length = 0;
	QCBOREncode_Init (&encoder, SizeCalculateUsefulBuf);
	composite_eat_codec_add_payload (&encoder, request, claims);
	status = composite_eat_codec_finish_size (&encoder, encoded_length);
	if (status != COMPOSITE_EAT_CODEC_OK) {
		return status;
	}
	if (encoded_capacity < *encoded_length) {
		return COMPOSITE_EAT_CODEC_BUFFER_TOO_SMALL;
	}

	QCBOREncode_Init (&encoder, (UsefulBuf){encoded, encoded_capacity});
	composite_eat_codec_add_payload (&encoder, request, claims);
	return composite_eat_codec_finish_output (&encoder, *encoded_length);
}

enum composite_eat_codec_status
composite_eat_codec_encode_protected_headers (const uint8_t certificate_thumbprint
												  [COMPOSITE_EAT_SHA384_LENGTH],
	uint8_t *encoded, size_t encoded_capacity, size_t *encoded_length)
{
	QCBOREncodeContext encoder;
	enum composite_eat_codec_status status;

	if ((certificate_thumbprint == NULL) ||
		!composite_eat_codec_output_valid (encoded, encoded_capacity, encoded_length)) {
		return COMPOSITE_EAT_CODEC_BAD_ARGUMENT;
	}

	*encoded_length = 0;
	QCBOREncode_Init (&encoder, SizeCalculateUsefulBuf);
	composite_eat_codec_add_protected_headers (&encoder, certificate_thumbprint);
	status = composite_eat_codec_finish_size (&encoder, encoded_length);
	if (status != COMPOSITE_EAT_CODEC_OK) {
		return status;
	}
	if (encoded_capacity < *encoded_length) {
		return COMPOSITE_EAT_CODEC_BUFFER_TOO_SMALL;
	}

	QCBOREncode_Init (&encoder, (UsefulBuf){encoded, encoded_capacity});
	composite_eat_codec_add_protected_headers (&encoder, certificate_thumbprint);
	return composite_eat_codec_finish_output (&encoder, *encoded_length);
}

enum composite_eat_codec_status
composite_eat_codec_encode_cose_sign1 (const struct composite_eat_cose_sign1 *cose,
	uint8_t *encoded, size_t encoded_capacity, size_t *encoded_length)
{
	QCBOREncodeContext encoder;
	enum composite_eat_codec_status status;
	size_t i;

	if (!composite_eat_codec_output_valid (encoded, encoded_capacity, encoded_length) ||
		(cose == NULL) || !composite_eat_codec_buffer_valid (&cose->protected_headers, false) ||
		!composite_eat_codec_buffer_valid (&cose->payload, false) ||
		!composite_eat_codec_buffer_valid (&cose->signature, false) ||
		(cose->signature.length != COMPOSITE_EAT_ES384_SIGNATURE_LENGTH) ||
		(cose->certificate_count == 0) ||
		(cose->certificate_count > COMPOSITE_EAT_MAX_CERTIFICATES)) {
		return COMPOSITE_EAT_CODEC_BAD_ARGUMENT;
	}

	for (i = 0; i < cose->certificate_count; ++i) {
		if (!composite_eat_codec_buffer_valid (&cose->certificates[i], false)) {
			return COMPOSITE_EAT_CODEC_BAD_ARGUMENT;
		}
	}

	*encoded_length = 0;
	QCBOREncode_Init (&encoder, SizeCalculateUsefulBuf);
	composite_eat_codec_add_cose_sign1 (&encoder, cose);
	status = composite_eat_codec_finish_size (&encoder, encoded_length);
	if (status != COMPOSITE_EAT_CODEC_OK) {
		return status;
	}
	if (encoded_capacity < *encoded_length) {
		return COMPOSITE_EAT_CODEC_BUFFER_TOO_SMALL;
	}

	QCBOREncode_Init (&encoder, (UsefulBuf){encoded, encoded_capacity});
	composite_eat_codec_add_cose_sign1 (&encoder, cose);
	return composite_eat_codec_finish_output (&encoder, *encoded_length);
}