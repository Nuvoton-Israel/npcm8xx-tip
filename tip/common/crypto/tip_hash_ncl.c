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
extern ROM_NCL_SHA_Finish		   pROM_NCL_SHA_Finish;
extern ROM_NCL_SHA_Start		   pROM_NCL_SHA_Start;
extern ROM_SEC_SHA_CALC            pROM_SEC_SHA_Calc;

/**
 * Free the active hash context.
 *
 * @param engine The hash engine whose context should be freed.
 */
static void tip_hash_ncl_free_context (const struct tip_hash_ncl_engine *engine)
{
	if (engine != NULL) {
		switch (engine->state->active) {
#ifdef HASH_ENABLE_SHA1
			case HASH_ACTIVE_SHA1:
				memset (&engine->state->context.sha1, 0, sizeof (RIOT_SHA1_CONTEXT));
				break;
#endif

			case HASH_ACTIVE_SHA256:
#if defined HASH_ENABLE_SHA384 || defined HASH_ENABLE_SHA512
			case HASH_ACTIVE_SHA384:
			case HASH_ACTIVE_SHA512:
#endif
				pROM_NCL_SHA_Clear (engine->state->context.ncl_sha);
				break;
		}

		engine->state->active = HASH_ACTIVE_NONE;
	}
}

#ifdef HASH_ENABLE_SHA1
int tip_hash_ncl_calculate_sha1 (const struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0)) || (hash == NULL)
		|| ((data != NULL) && (length == 0))) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->state->active != HASH_ACTIVE_NONE) {
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

int tip_hash_ncl_start_sha1 (const struct hash_engine *engine)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;

	if (tip_ncl == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->state->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	/* leverage RIOT HASH SHA1 implementation */
	RIOT_SHA1_Init (&tip_ncl->state->context.sha1);

	tip_ncl->state->active = HASH_ACTIVE_SHA1;

	return 0;
}
#endif

int tip_hash_ncl_calculate_sha256 (const struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;
	NCL_STATUS_T status;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0)) || (hash == NULL)
		|| ((data != NULL) && (length == 0))) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->state->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	if (hash_length < SHA256_HASH_LENGTH) {
		return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
	}

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_Reset (tip_ncl->state->context.ncl_sha),
		tip_ncl->shared_state);
	status = pROM_SEC_SHA_Calc (tip_ncl->state->context.ncl_sha, NCL_SHA_TYPE_2_256, data, length,
		hash);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

	return tip_ncl_status (status, HASH_ENGINE_SHA256_FAILED);
}

int tip_hash_ncl_start_sha256 (const struct hash_engine *engine)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;
	NCL_STATUS_T status;

	if (tip_ncl == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_Reset (tip_ncl->state->context.ncl_sha),
		tip_ncl->shared_state);
	status = pROM_NCL_SHA_Start (tip_ncl->state->context.ncl_sha, NCL_SHA_TYPE_2_256);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);
	if (status != NCL_STATUS_OK) {
		return HASH_ENGINE_START_SHA256_FAILED;
	}

	tip_ncl->state->active = HASH_ACTIVE_SHA256;
	return 0;
}

#ifdef HASH_ENABLE_SHA384
int tip_hash_ncl_calculate_sha384 (const struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;
	NCL_STATUS_T status;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0)) || (hash == NULL)) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->state->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	if (hash_length < SHA384_HASH_LENGTH) {
		return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
	}

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_Reset (tip_ncl->state->context.ncl_sha),
		tip_ncl->shared_state);
	status = pROM_SEC_SHA_Calc (tip_ncl->state->context.ncl_sha, NCL_SHA_TYPE_2_384, data, length,
		hash);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

	return tip_ncl_status (status, HASH_ENGINE_SHA384_FAILED);
}

int tip_hash_ncl_start_sha384 (const struct hash_engine *engine)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;
	NCL_STATUS_T status;

	if (tip_ncl == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->state->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_Reset (tip_ncl->state->context.ncl_sha),
		tip_ncl->shared_state);
	status = pROM_NCL_SHA_Start (tip_ncl->state->context.ncl_sha, NCL_SHA_TYPE_2_384);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);
	if (status != NCL_STATUS_OK) {
		return HASH_ENGINE_START_SHA384_FAILED;
	}

	tip_ncl->state->active = HASH_ACTIVE_SHA384;
	return 0;
}
#endif

int tip_hash_ncl_calculate_sha512 (const struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine *) engine;
	NCL_STATUS_T status;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0)) || (hash == NULL)
		|| ((data != NULL) && (length == 0))) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->state->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	if (hash_length < SHA512_HASH_LENGTH) {
		return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
	}

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_Reset (tip_ncl->state->context.ncl_sha),
		tip_ncl->shared_state);
	status = pROM_SEC_SHA_Calc (tip_ncl->state->context.ncl_sha, NCL_SHA_TYPE_2_512, data, length,
		hash);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

	return tip_ncl_status (status, HASH_ENGINE_SHA512_FAILED);
}

int tip_hash_ncl_start_sha512 (const struct hash_engine *engine)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;
	NCL_STATUS_T status;

	if (tip_ncl == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	if (tip_ncl->state->active != HASH_ACTIVE_NONE) {
		return HASH_ENGINE_HASH_IN_PROGRESS;
	}

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_Reset (tip_ncl->state->context.ncl_sha),
		tip_ncl->shared_state);
	status = pROM_NCL_SHA_Start (tip_ncl->state->context.ncl_sha, NCL_SHA_TYPE_2_512);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);
	if (status != NCL_STATUS_OK) {
		return HASH_ENGINE_START_SHA512_FAILED;
	}

	tip_ncl->state->active = HASH_ACTIVE_SHA512;
	return 0;
}

enum hash_type tip_hash_ncl_get_active_algorithm (const struct hash_engine *engine)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;

	if (tip_ncl == NULL) {
		return HASH_TYPE_INVALID;
	}

	return hash_get_type_from_active (tip_ncl->state->active);
}

int tip_hash_ncl_update (const struct hash_engine *engine, const uint8_t *data,
	size_t length)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;
	NCL_STATUS_T status = NCL_STATUS_OK;

	if ((tip_ncl == NULL) || ((data == NULL) && (length != 0))
		|| ((data != NULL) && (length == 0))) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	switch (tip_ncl->state->active) {
#ifdef HASH_ENABLE_SHA1
		case HASH_ACTIVE_SHA1:
			if (data != NULL)
				RIOT_SHA1_Update (&tip_ncl->state->context.sha1, data, length);
			break;
#endif

		case HASH_ACTIVE_SHA256:
#if defined HASH_ENABLE_SHA384 || defined HASH_ENABLE_SHA512
		case HASH_ACTIVE_SHA384:
		case HASH_ACTIVE_SHA512:
#endif
			tip_ncl->shared_state->lock (tip_ncl->shared_state);
			status = pROM_NCL_SHA_Update (tip_ncl->state->context.ncl_sha, data, length);
			tip_ncl->shared_state->unlock (tip_ncl->shared_state);
			break;

		default:
			return HASH_ENGINE_NO_ACTIVE_HASH;
	}

	return tip_ncl_status (status, HASH_ENGINE_UPDATE_FAILED);
}

int tip_hash_ncl_finish (const struct hash_engine *engine, uint8_t *hash, size_t hash_length)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;
	NCL_STATUS_T status = NCL_STATUS_OK;

	if ((tip_ncl == NULL) || (hash == NULL)) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	tip_ncl->shared_state->lock (tip_ncl->shared_state);

	switch (tip_ncl->state->active) {
#ifdef HASH_ENABLE_SHA1
		case HASH_ACTIVE_SHA1:
			if (hash_length < SHA1_HASH_LENGTH) {
				tip_ncl->shared_state->unlock (tip_ncl->shared_state);
				return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
			}
			RIOT_SHA1_Final (&tip_ncl->state->context.sha1, hash);
			break;
#endif

		case HASH_ACTIVE_SHA256:
			if (hash_length < SHA256_HASH_LENGTH) {
				tip_ncl->shared_state->unlock (tip_ncl->shared_state);
				return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
			}
			status = pROM_NCL_SHA_Finish (tip_ncl->state->context.ncl_sha, hash);
			break;

#ifdef HASH_ENABLE_SHA384
		case HASH_ACTIVE_SHA384:
			if (hash_length < SHA384_HASH_LENGTH) {
				tip_ncl->shared_state->unlock (tip_ncl->shared_state);
				return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
			}
			status = pROM_NCL_SHA_Finish (tip_ncl->state->context.ncl_sha, hash);
			break;
#endif

#ifdef HASH_ENABLE_SHA512
		case HASH_ACTIVE_SHA512:
			if (hash_length < SHA512_HASH_LENGTH) {
				tip_ncl->shared_state->unlock (tip_ncl->shared_state);
				return HASH_ENGINE_HASH_BUFFER_TOO_SMALL;
			}
			status = pROM_NCL_SHA_Finish (tip_ncl->state->context.ncl_sha, hash);
			break;
#endif
		default:
			tip_ncl->shared_state->unlock (tip_ncl->shared_state);
			return HASH_ENGINE_NO_ACTIVE_HASH;
	}

	if (status != NCL_STATUS_OK) {
		tip_ncl->shared_state->unlock (tip_ncl->shared_state);
		return HASH_ENGINE_FINISH_FAILED;
	}

	tip_hash_ncl_free_context (tip_ncl);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);
	return 0;
}

void tip_hash_ncl_cancel (const struct hash_engine *engine)
{
	const struct tip_hash_ncl_engine *tip_ncl = (const struct tip_hash_ncl_engine*) engine;
	if (tip_ncl) {
		tip_ncl->shared_state->lock (tip_ncl->shared_state);
		tip_hash_ncl_free_context (tip_ncl);
		tip_ncl->shared_state->unlock (tip_ncl->shared_state);
	}
}

/**
 * Initialize only the variable state of an NCL hash engine.  The rest of the instance is
 * assumed to already have been initialized.
 *
 * This would generally be used with a statically initialized instance.
 *
 * @param engine The hash engine that contains the state to initialize.
 *
 * @return 0 if the state was successfully initialized or an error code.
 */
int tip_hash_ncl_init_state (const struct tip_hash_ncl_engine *engine)
{
	if ((engine == NULL) || (engine->state == NULL)) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	memset (engine->state, 0, sizeof (*engine->state));

	engine->shared_state->lock (engine->shared_state);

	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_Power (engine->state->context.ncl_sha, TRUE),
		engine->shared_state);

	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_InitContext (engine->state->context.ncl_sha),
		engine->shared_state);

	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_Init (engine->state->context.ncl_sha),
		engine->shared_state);

	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_SHA_Reset (engine->state->context.ncl_sha),
		engine->shared_state);

	engine->shared_state->unlock (engine->shared_state);

	engine->state->active = HASH_ACTIVE_NONE;

	return 0;
}

/**
 * Initialize NCL hash engine.
 *
 * @param engine The hash engine to initialize.
 * @param state The variable context for the hash engine.  This must be uninitialized.
 *
 * @return 0 if the hash engine was successfully initialized or an error code.
 */
int tip_hash_ncl_init (struct tip_hash_ncl_engine *engine, struct tip_hash_ncl_engine_state *state,
	struct tip_rom_ncl_shared_state *shared_state)
{
	if (engine == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}
	if (shared_state == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	memset (engine, 0, sizeof (struct tip_hash_ncl_engine));

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

	engine->base.get_active_algorithm = tip_hash_ncl_get_active_algorithm;
	engine->base.update = tip_hash_ncl_update;
	engine->base.finish = tip_hash_ncl_finish;
	engine->base.cancel = tip_hash_ncl_cancel;

	engine->state = state;
	engine->shared_state = shared_state;

	return tip_hash_ncl_init_state (engine);
}

/**
 * Release the resources used by NCL hash engine.
 *
 * @param engine The hash engine to release.
 */
void tip_hash_ncl_release (struct tip_hash_ncl_engine *engine)
{
	if (engine != NULL && engine->shared_state) {
		engine->shared_state->lock (engine->shared_state);
		tip_hash_ncl_free_context (engine);
		engine->shared_state->unlock (engine->shared_state);
	}
}
