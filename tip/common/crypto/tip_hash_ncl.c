/*
 *  Nuvoton Technology Corporation confidential
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *<<<----------------------------------------------------------------------
 * File Contents:
 *   tip_hash_ncl.c
 *            This file contains API of security routines for ROM code HASH engine
 *  Project:
 *            Arbel
 */

#include <stdlib.h>
#include <string.h>
#include "tip_hash_ncl.h"

extern ROM_NCL_SHA_Clear		   pROM_NCL_SHA_Clear;
extern ROM_NCL_SHA_Power		   pROM_NCL_SHA_Power;
extern ROM_NCL_SHA_InitContext	   pROM_NCL_SHA_InitContext;
extern ROM_NCL_SHA_Init			   pROM_NCL_SHA_Init;
extern ROM_NCL_SHA_FinalizeContext pROM_NCL_SHA_FinalizeContext;
extern ROM_NCL_SHA_Reset		   pROM_NCL_SHA_Reset;
extern ROM_NCL_SHA_Update		   pROM_NCL_SHA_Update;
extern ROM_NCL_SHA_Calc			   pROM_NCL_SHA_Calc;
extern ROM_NCL_SHA_Finish		   pROM_NCL_SHA_Finish;
extern ROM_NCL_SHA_Start		   pROM_NCL_SHA_Start;


/**
 * Free the active hash context.
 *
 * @param engine The hash engine whose context should be freed.
 */
static void tip_hash_ncl_free_context (struct tip_hash_ncl_engine *engine)
{
	if (engine != NULL) {
		switch (engine->active) {
#ifdef HASH_ENABLE_SHA1
			case HASH_ACTIVE_SHA1:
				memset (&engine->context.sha1, 0, sizeof (RIOT_SHA1_CONTEXT));
				break;
#endif

			case HASH_ACTIVE_SHA256:
#if defined HASH_ENABLE_SHA384 || defined HASH_ENABLE_SHA512
			case HASH_ACTIVE_SHA384:
			case HASH_ACTIVE_SHA512:
#endif
				pROM_NCL_SHA_Clear (engine->context.ncl_sha);
				break;
		}

		engine->active = HASH_ACTIVE_NONE;
	}
}

#ifdef HASH_ENABLE_SHA1
static int tip_hash_ncl_calculate_sha1 (struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0)) || (hash == NULL)
		|| ((data != NULL) && (length == 0))) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	if (hash_length < SHA1_HASH_LENGTH) {
		return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
	}
	if (data != NULL) {
		/* leverage RIOT HASH SHA1 implementation */
		RIOT_SHA1_Block (data, length, hash);
	}

	return 0;
}

static int tip_hash_ncl_start_sha1 (struct hash_engine *engine)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;

	if (tip_ncl == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	/* leverage RIOT HASH SHA1 implementation */
	RIOT_SHA1_Init (&tip_ncl->context.sha1);

	tip_ncl->active = HASH_ACTIVE_SHA1;

	return 0;
}
#endif

static int tip_hash_ncl_calculate_sha256 (struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;
	NCL_STATUS_T status;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0)) || (hash == NULL)
		|| ((data != NULL) && (length == 0))) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	if (hash_length < SHA256_HASH_LENGTH) {
		return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
	}

	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_Reset (tip_ncl->context.ncl_sha));
	status = pROM_NCL_SHA_Calc (tip_ncl->context.ncl_sha, NCL_SHA_TYPE_2_256, data, length, hash);

	return tip_ncl_status (status, HASH_ENGINE_SHA256_FAILED);
}

static int tip_hash_ncl_start_sha256 (struct hash_engine *engine)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;
	NCL_STATUS_T status;

	if (tip_ncl == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_Reset (tip_ncl->context.ncl_sha));
	status = pROM_NCL_SHA_Start (tip_ncl->context.ncl_sha, NCL_SHA_TYPE_2_256);
	if (status != NCL_STATUS_OK) {
		return HASH_ENGINE_START_SHA256_FAILED;
	}

	tip_ncl->active = HASH_ACTIVE_SHA256;
	return 0;
}

#ifdef HASH_ENABLE_SHA384
static int tip_hash_ncl_calculate_sha384 (struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;
	NCL_STATUS_T status;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0)) || (hash == NULL)) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	if (hash_length < SHA384_HASH_LENGTH) {
		return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
	}

	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_Reset (tip_ncl->context.ncl_sha));
	status = pROM_NCL_SHA_Calc (tip_ncl->context.ncl_sha, NCL_SHA_TYPE_2_384, data, length, hash);

	return tip_ncl_status (status, HASH_ENGINE_SHA384_FAILED);
}

static int tip_hash_ncl_start_sha384 (struct hash_engine *engine)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;
	NCL_STATUS_T status;

	if (tip_ncl == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_Reset (tip_ncl->context.ncl_sha));
	status = pROM_NCL_SHA_Start (tip_ncl->context.ncl_sha, NCL_SHA_TYPE_2_384);
	if (status != NCL_STATUS_OK) {
		return HASH_ENGINE_START_SHA384_FAILED;
	}

	tip_ncl->active = HASH_ACTIVE_SHA384;
	return 0;
}
#endif

#ifdef HASH_ENABLE_SHA512
static int tip_hash_ncl_calculate_sha512 (struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;
	NCL_STATUS_T status;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0)) || (hash == NULL)
		|| ((data != NULL) && (length == 0))) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	if (hash_length < SHA512_HASH_LENGTH) {
		return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
	}

	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_Reset (tip_ncl->context.ncl_sha));
	status = pROM_NCL_SHA_Calc (tip_ncl->context.ncl_sha, NCL_SHA_TYPE_2_512, data, length, hash);

	return tip_ncl_status (status, HASH_ENGINE_SHA512_FAILED);
}

static int tip_hash_ncl_start_sha512 (struct hash_engine *engine)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;
	NCL_STATUS_T status;

	if (tip_ncl == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_Reset (tip_ncl->context.ncl_sha));
	status = pROM_NCL_SHA_Start (tip_ncl->context.ncl_sha, NCL_SHA_TYPE_2_512);
	if (status != NCL_STATUS_OK) {
		return HASH_ENGINE_START_SHA512_FAILED;
	}

	tip_ncl->active = HASH_ACTIVE_SHA512;
	return 0;
}
#endif

static int tip_hash_ncl_update (struct hash_engine *engine, const uint8_t *data, size_t length)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;
	NCL_STATUS_T status = NCL_STATUS_OK;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0)) 
		|| ((data != NULL) && (length == 0))) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	switch (tip_ncl->active) {
#ifdef HASH_ENABLE_SHA1
		case HASH_ACTIVE_SHA1:
			if (data != NULL) 
				RIOT_SHA1_Update (&tip_ncl->context.sha1, data, length);
			break;
#endif

		case HASH_ACTIVE_SHA256:
#if defined HASH_ENABLE_SHA384 || defined HASH_ENABLE_SHA512
		case HASH_ACTIVE_SHA384:
		case HASH_ACTIVE_SHA512:
#endif
			status = pROM_NCL_SHA_Update (tip_ncl->context.ncl_sha, data, length);
			break;

		default:
			return HASH_ENGINE_NO_ACTIVE_HASH;
	}

	return tip_ncl_status (status, HASH_ENGINE_UPDATE_FAILED);
}

static int tip_hash_ncl_finish (struct hash_engine *engine, uint8_t *hash, size_t hash_length)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;
	NCL_STATUS_T status = NCL_STATUS_OK;

	if ((tip_ncl == NULL) || (hash == NULL)) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	switch (tip_ncl->active) {
#ifdef HASH_ENABLE_SHA1
		case HASH_ACTIVE_SHA1:
			if (hash_length < SHA1_HASH_LENGTH) {
				return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
			}
			RIOT_SHA1_Final (&tip_ncl->context.sha1, hash);
			break;
#endif

		case HASH_ACTIVE_SHA256:
			if (hash_length < SHA256_HASH_LENGTH) {
				return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
			}
			status = pROM_NCL_SHA_Finish (tip_ncl->context.ncl_sha, hash);
			break;

#ifdef HASH_ENABLE_SHA384
		case HASH_ACTIVE_SHA384:
			if (hash_length < SHA384_HASH_LENGTH) {
				return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
			}
			status = pROM_NCL_SHA_Finish (tip_ncl->context.ncl_sha, hash);
			break;
#endif

#ifdef HASH_ENABLE_SHA512
		case HASH_ACTIVE_SHA512:
			if (hash_length < SHA512_HASH_LENGTH) {
				return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
			}
			status = pROM_NCL_SHA_Finish (tip_ncl->context.ncl_sha, hash);
			break;
#endif
		default:
			return HASH_ENGINE_NO_ACTIVE_HASH;
	}

	if (status != NCL_STATUS_OK) {
		return HASH_ENGINE_FINISH_FAILED;
	}

	tip_hash_ncl_free_context (tip_ncl);
	return 0;
}

static void tip_hash_ncl_cancel (struct hash_engine *engine)
{
	struct tip_hash_ncl_engine *tip_ncl = (struct tip_hash_ncl_engine *) engine;
	if (tip_ncl) {
		tip_hash_ncl_free_context (tip_ncl);
	}
}

/**
 * Initialize NCL hash engine.
 *
 * @param engine The hash engine to initialize.
 *
 * @return 0 if the hash engine was successfully initialized or an error code.
 */
int tip_hash_ncl_init (struct tip_hash_ncl_engine *engine)
{
	if (engine == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	memset (engine, 0, sizeof (struct tip_hash_ncl_engine));
	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_Power (engine->context.ncl_sha, TRUE));

	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_InitContext (engine->context.ncl_sha));

	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_Init (engine->context.ncl_sha));

	NCL_STATUS_RET_CHECK (pROM_NCL_SHA_Reset (engine->context.ncl_sha));

#ifdef HASH_ENABLE_SHA1
	engine->base.calculate_sha1 = tip_hash_ncl_calculate_sha1;
	engine->base.start_sha1 = tip_hash_ncl_start_sha1;
#endif
	engine->base.calculate_sha256 = tip_hash_ncl_calculate_sha256;
	engine->base.start_sha256 = tip_hash_ncl_start_sha256;
#ifdef HASH_ENABLE_SHA384
	engine->base.calculate_sha384 = tip_hash_ncl_calculate_sha384;
	engine->base.start_sha384 = tip_hash_ncl_start_sha384;
#endif
#ifdef HASH_ENABLE_SHA512
	engine->base.calculate_sha512 = tip_hash_ncl_calculate_sha512;
	engine->base.start_sha512 = tip_hash_ncl_start_sha512;
#endif
	engine->base.update = tip_hash_ncl_update;
	engine->base.finish = tip_hash_ncl_finish;
	engine->base.cancel = tip_hash_ncl_cancel;

	engine->active = HASH_ACTIVE_NONE;

	return 0;
}

/**
 * Release the resources used by NCL hash engine.
 *
 * @param engine The hash engine to release.
 */
void tip_hash_ncl_release (struct tip_hash_ncl_engine *engine)
{
	if (engine != NULL) {
		tip_hash_ncl_free_context (engine);
	}
}
