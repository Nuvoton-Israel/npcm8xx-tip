/*
 *  Nuvoton Technology Corporation confidential
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *<<<----------------------------------------------------------------------
 * File Contents:
 *   tip_rng_ncl.c
 *            This file contains API of security routines for ROM code RNG engine
 *  Project:
 *            Arbel
 */

#include <stdint.h>
#include <stddef.h>
#include "tip_rng_ncl.h"


extern ROM_NCL_SHA_Power			pROM_NCL_SHA_Power;
extern ROM_NCL_SHA_InitContext		pROM_NCL_SHA_InitContext;
extern ROM_NCL_SHA_FinalizeContext	pROM_NCL_SHA_FinalizeContext;
extern ROM_NCL_DRBG_Generate		pROM_NCL_DRBG_Generate;
extern ROM_NCL_DRBG_InitContext		pROM_NCL_DRBG_InitContext;
extern ROM_NCL_DRBG_Init			pROM_NCL_DRBG_Init;
extern ROM_NCL_DRBG_FinalizeContext pROM_NCL_DRBG_FinalizeContext;
extern ROM_NCL_DRBG_Instantiate		pROM_NCL_DRBG_Instantiate;
extern ROM_NCL_DRBG_Power			pROM_NCL_DRBG_Power;


static int tip_rng_ncl_generate_random_buffer (struct rng_engine *engine, size_t rand_len,
	uint8_t *buf)
{
	struct tip_rng_ncl_engine *tip_ncl_engine = (struct tip_rng_ncl_engine *) engine;
	const uint8_t *addData = NULL;
	uint32_t addDataLen = 0;
	NCL_STATUS_T status;

	if (tip_ncl_engine == NULL || buf == NULL || rand_len == 0) {
		return RNG_ENGINE_INVALID_ARGUMENT;
	}

	status = pROM_NCL_DRBG_Generate (NULL, addData, addDataLen, buf, rand_len);
	return tip_ncl_status (status, RNG_ENGINE_RANDOM_FAILED);
}

/**
 * Initialize a tip ncl engine for generating random numbers.
 *
 * @param engine The tip ncl RNG engine to initialize.
 *
 * @return 0 if the RNG engine was initialized successfully or an error code.
 */
int tip_rng_ncl_init (struct tip_rng_ncl_engine *engine)
{
	if (engine == NULL) {
		return RNG_ENGINE_INVALID_ARGUMENT;
	}
	NCL_STATUS_RET_CHECK (pROM_NCL_DRBG_Power (engine->drbg_context, TRUE));
	NCL_STATUS_RET_CHECK (pROM_NCL_DRBG_InitContext (engine->drbg_context));
	NCL_STATUS_RET_CHECK (pROM_NCL_DRBG_Init (engine->drbg_context, FALSE));

	engine->base.generate_random_buffer = tip_rng_ncl_generate_random_buffer;
	return 0;
}

/**
 * Release the resources used by an mbed TLS RNG engine.
 *
 * @param engine The mbed TLS RNG engine to release.
 */
void tip_rng_ncl_release (struct tip_rng_ncl_engine *engine)
{
	if (engine != NULL) {
		pROM_NCL_DRBG_FinalizeContext (engine->drbg_context);
	}
}
