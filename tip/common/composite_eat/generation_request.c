/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <stdbool.h>
#include <string.h>

#include "composite_eat/generation_request.h"
#include "qcbor/qcbor_decode.h"


#define COMPOSITE_EAT_REQUEST_KEY_VERSION 1
#define COMPOSITE_EAT_REQUEST_KEY_NONCE 2
#define COMPOSITE_EAT_REQUEST_KEY_RECORDS 3

#define COMPOSITE_EAT_RECORD_KEY_ENVIRONMENT 1
#define COMPOSITE_EAT_RECORD_KEY_DIGEST 2

#define COMPOSITE_EAT_REQUEST_FIELD_COUNT 3u
#define COMPOSITE_EAT_RECORD_FIELD_COUNT 2u


static bool composite_eat_generation_request_environment_valid (const uint8_t *environment,
	size_t length)
{
	size_t i;

	if ((environment == NULL) || (length < 5) ||
		(length > COMPOSITE_EAT_MAX_ENVIRONMENT_LENGTH) ||
		(memcmp (environment, "env.", 4) != 0) || (environment[length - 1] == '.')) {
		return false;
	}

	for (i = 4; i < length; ++i) {
		if (environment[i] == '.') {
			if (environment[i - 1] == '.') {
				return false;
			}
		}
		else if (!(((environment[i] >= 'a') && (environment[i] <= 'z')) ||
			((environment[i] >= '0') && (environment[i] <= '9')) ||
			(environment[i] == '-'))) {
			return false;
		}
	}

	return true;
}

enum composite_eat_generation_request_status composite_eat_generation_request_validate (
	const struct composite_eat_generation_request *request)
{
	size_t i;
	size_t previous;

	if ((request == NULL) ||
		(request->version != COMPOSITE_EAT_GENERATION_REQUEST_VERSION) ||
		(request->nonce == NULL) ||
		(request->nonce_length != COMPOSITE_EAT_NONCE_LENGTH) ||
		(request->record_count == 0) ||
		(request->record_count > COMPOSITE_EAT_MAX_DEVICE_RECORDS)) {
		return COMPOSITE_EAT_GENERATION_REQUEST_BAD_ARGUMENT;
	}

	for (i = 0; i < request->record_count; ++i) {
		if ((request->records[i].digest == NULL) ||
			(request->records[i].digest_length != COMPOSITE_EAT_SHA384_LENGTH) ||
			!composite_eat_generation_request_environment_valid (
			request->records[i].environment, request->records[i].environment_length)) {
			return COMPOSITE_EAT_GENERATION_REQUEST_BAD_ARGUMENT;
		}

		for (previous = 0; previous < i; ++previous) {
			if ((request->records[i].environment_length ==
					request->records[previous].environment_length) &&
				(memcmp (request->records[i].environment,
					request->records[previous].environment,
					request->records[i].environment_length) == 0)) {
				return COMPOSITE_EAT_GENERATION_REQUEST_BAD_ARGUMENT;
			}
		}
	}

	return COMPOSITE_EAT_GENERATION_REQUEST_OK;
}

static enum composite_eat_generation_request_status composite_eat_generation_request_get_next (
	QCBORDecodeContext *decoder, QCBORItem *item)
{
	if ((QCBORDecode_GetNext (decoder, item) != QCBOR_SUCCESS) ||
		(item->uTags[0] != CBOR_TAG_INVALID16)) {
		return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
	}

	return COMPOSITE_EAT_GENERATION_REQUEST_OK;
}

static bool composite_eat_generation_request_integer_label (const QCBORItem *item, int64_t expected)
{
	return (item->uLabelType == QCBOR_TYPE_INT64) && (item->label.int64 == expected);
}

static enum composite_eat_generation_request_status composite_eat_generation_request_decode_digest (
	QCBORDecodeContext *decoder, const QCBORItem *container,
	struct composite_eat_device_record *record)
{
	QCBORItem item;
	enum composite_eat_generation_request_status status;

	if ((container->uDataType != QCBOR_TYPE_ARRAY) || (container->val.uCount != 2) ||
		(container->uNestingLevel != 3)) {
		return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
	}

	status = composite_eat_generation_request_get_next (decoder, &item);
	if ((status != COMPOSITE_EAT_GENERATION_REQUEST_OK) || (item.uNestingLevel != 4) ||
		(item.uLabelType != QCBOR_TYPE_NONE) || (item.uDataType != QCBOR_TYPE_INT64) ||
		(item.val.int64 != COMPOSITE_EAT_COSE_ALGORITHM_SHA384)) {
		return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
	}

	status = composite_eat_generation_request_get_next (decoder, &item);
	if ((status != COMPOSITE_EAT_GENERATION_REQUEST_OK) || (item.uNestingLevel != 4) ||
		(item.uLabelType != QCBOR_TYPE_NONE) || (item.uDataType != QCBOR_TYPE_BYTE_STRING) ||
		(item.val.string.len != COMPOSITE_EAT_SHA384_LENGTH)) {
		return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
	}

	record->digest = item.val.string.ptr;
	record->digest_length = item.val.string.len;
	return COMPOSITE_EAT_GENERATION_REQUEST_OK;
}

static enum composite_eat_generation_request_status composite_eat_generation_request_decode_record (
	QCBORDecodeContext *decoder, struct composite_eat_generation_request *request,
	size_t record_index)
{
	struct composite_eat_device_record *record = &request->records[record_index];
	QCBORItem item;
	bool environment_seen = false;
	bool digest_seen = false;
	size_t i;
	size_t field;
	enum composite_eat_generation_request_status status;

	status = composite_eat_generation_request_get_next (decoder, &item);
	if ((status != COMPOSITE_EAT_GENERATION_REQUEST_OK) || (item.uNestingLevel != 2) ||
		(item.uLabelType != QCBOR_TYPE_NONE) || (item.uDataType != QCBOR_TYPE_MAP) ||
		(item.val.uCount != COMPOSITE_EAT_RECORD_FIELD_COUNT)) {
		return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
	}

	for (field = 0; field < COMPOSITE_EAT_RECORD_FIELD_COUNT; ++field) {
		status = composite_eat_generation_request_get_next (decoder, &item);
		if ((status != COMPOSITE_EAT_GENERATION_REQUEST_OK) || (item.uNestingLevel != 3)) {
			return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
		}

		if (composite_eat_generation_request_integer_label (&item,
			COMPOSITE_EAT_RECORD_KEY_ENVIRONMENT)) {
			if (environment_seen || (item.uDataType != QCBOR_TYPE_TEXT_STRING) ||
				!composite_eat_generation_request_environment_valid (item.val.string.ptr,
					item.val.string.len)) {
				return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
			}

			record->environment = item.val.string.ptr;
			record->environment_length = item.val.string.len;
			environment_seen = true;
		}
		else if (composite_eat_generation_request_integer_label (&item,
			COMPOSITE_EAT_RECORD_KEY_DIGEST)) {
			if (digest_seen) {
				return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
			}

			status = composite_eat_generation_request_decode_digest (decoder, &item, record);
			if (status != COMPOSITE_EAT_GENERATION_REQUEST_OK) {
				return status;
			}
			digest_seen = true;
		}
		else {
			return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
		}
	}

	if (!environment_seen || !digest_seen) {
		return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
	}

	for (i = 0; i < record_index; ++i) {
		if ((record->environment_length == request->records[i].environment_length) &&
			(memcmp (record->environment, request->records[i].environment,
				record->environment_length) == 0)) {
			return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
		}
	}

	return COMPOSITE_EAT_GENERATION_REQUEST_OK;
}

static enum composite_eat_generation_request_status composite_eat_generation_request_decode_records (
	QCBORDecodeContext *decoder, const QCBORItem *container,
	struct composite_eat_generation_request *request)
{
	size_t i;
	enum composite_eat_generation_request_status status;

	if ((container->uDataType != QCBOR_TYPE_ARRAY) || (container->uNestingLevel != 1) ||
		(container->val.uCount == QCBOR_COUNT_INDICATES_INDEFINITE_LENGTH)) {
		return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
	}
	if (container->val.uCount == 0) {
		return COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
	}
	if (container->val.uCount > COMPOSITE_EAT_MAX_DEVICE_RECORDS) {
		return COMPOSITE_EAT_GENERATION_REQUEST_TOO_MANY_RECORDS;
	}

	request->record_count = container->val.uCount;
	for (i = 0; i < request->record_count; ++i) {
		status = composite_eat_generation_request_decode_record (decoder, request, i);
		if (status != COMPOSITE_EAT_GENERATION_REQUEST_OK) {
			return status;
		}
	}

	return COMPOSITE_EAT_GENERATION_REQUEST_OK;
}

enum composite_eat_generation_request_status composite_eat_generation_request_decode (
	const uint8_t *encoded, size_t encoded_length, struct composite_eat_generation_request *request)
{
	QCBORDecodeContext decoder;
	QCBORItem item;
	bool version_seen = false;
	bool nonce_seen = false;
	bool records_seen = false;
	size_t field;
	enum composite_eat_generation_request_status status = COMPOSITE_EAT_GENERATION_REQUEST_OK;

	if ((encoded == NULL) || (encoded_length == 0) || (request == NULL)) {
		return COMPOSITE_EAT_GENERATION_REQUEST_BAD_ARGUMENT;
	}

	memset (request, 0, sizeof (*request));
	QCBORDecode_Init (&decoder, (UsefulBufC){encoded, encoded_length}, QCBOR_DECODE_MODE_NORMAL);

	status = composite_eat_generation_request_get_next (&decoder, &item);
	if ((status != COMPOSITE_EAT_GENERATION_REQUEST_OK) || (item.uNestingLevel != 0) ||
		(item.uLabelType != QCBOR_TYPE_NONE) || (item.uDataType != QCBOR_TYPE_MAP) ||
		(item.val.uCount != COMPOSITE_EAT_REQUEST_FIELD_COUNT)) {
		status = COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
		goto error;
	}

	for (field = 0; field < COMPOSITE_EAT_REQUEST_FIELD_COUNT; ++field) {
		status = composite_eat_generation_request_get_next (&decoder, &item);
		if ((status != COMPOSITE_EAT_GENERATION_REQUEST_OK) || (item.uNestingLevel != 1)) {
			status = COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
			goto error;
		}

		if (composite_eat_generation_request_integer_label (&item,
			COMPOSITE_EAT_REQUEST_KEY_VERSION)) {
			if (version_seen || (item.uDataType != QCBOR_TYPE_INT64)) {
				status = COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
				goto error;
			}
			if (item.val.int64 != COMPOSITE_EAT_GENERATION_REQUEST_VERSION) {
				status = COMPOSITE_EAT_GENERATION_REQUEST_BAD_VERSION;
				goto error;
			}
			request->version = (uint32_t) item.val.int64;
			version_seen = true;
		}
		else if (composite_eat_generation_request_integer_label (&item,
			COMPOSITE_EAT_REQUEST_KEY_NONCE)) {
			if (nonce_seen || (item.uDataType != QCBOR_TYPE_BYTE_STRING) ||
				(item.val.string.len != COMPOSITE_EAT_NONCE_LENGTH)) {
				status = COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
				goto error;
			}
			request->nonce = item.val.string.ptr;
			request->nonce_length = item.val.string.len;
			nonce_seen = true;
		}
		else if (composite_eat_generation_request_integer_label (&item,
			COMPOSITE_EAT_REQUEST_KEY_RECORDS)) {
			if (records_seen) {
				status = COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
				goto error;
			}
			status = composite_eat_generation_request_decode_records (&decoder, &item, request);
			if (status != COMPOSITE_EAT_GENERATION_REQUEST_OK) {
				goto error;
			}
			records_seen = true;
		}
		else {
			status = COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
			goto error;
		}
	}

	if (!version_seen || !nonce_seen || !records_seen ||
		(QCBORDecode_Finish (&decoder) != QCBOR_SUCCESS) ||
		(composite_eat_generation_request_validate (request) !=
			COMPOSITE_EAT_GENERATION_REQUEST_OK)) {
		status = COMPOSITE_EAT_GENERATION_REQUEST_MALFORMED;
		goto error;
	}

	return COMPOSITE_EAT_GENERATION_REQUEST_OK;

error:
	memset (request, 0, sizeof (*request));
	return status;
}