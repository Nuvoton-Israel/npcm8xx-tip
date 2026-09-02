/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "tip_rng_ncl.h"
#include "tip_rom_ncl_shared_state.h"


extern ROM_NCL_SHA_FinalizeContext	pROM_NCL_SHA_FinalizeContext;
extern ROM_NCL_DRBG_Generate		pROM_NCL_DRBG_Generate;
extern ROM_NCL_DRBG_InitContext		pROM_NCL_DRBG_InitContext;
extern ROM_NCL_DRBG_Init			pROM_NCL_DRBG_Init;
extern ROM_NCL_DRBG_FinalizeContext pROM_NCL_DRBG_FinalizeContext;
extern ROM_NCL_DRBG_Instantiate		pROM_NCL_DRBG_Instantiate;
extern ROM_NCL_DRBG_Power			pROM_NCL_DRBG_Power;


int tip_rng_ncl_generate_random_buffer (const struct rng_engine *engine, size_t rand_len,
	uint8_t *buf)
{
	const struct tip_rng_ncl_engine *tip_ncl_engine = (const struct tip_rng_ncl_engine*) engine;
	const uint8_t *addData = NULL;
	uint32_t addDataLen = 0;
	NCL_STATUS_T status;

	if (tip_ncl_engine == NULL || buf == NULL || rand_len == 0) {
		return RNG_ENGINE_INVALID_ARGUMENT;
	}

	tip_ncl_engine->shared_state->lock (tip_ncl_engine->shared_state);
	status = pROM_NCL_DRBG_Generate (NULL, addData, addDataLen, buf, rand_len);
	tip_ncl_engine->shared_state->unlock (tip_ncl_engine->shared_state);
	return tip_ncl_status (status, RNG_ENGINE_RANDOM_FAILED);
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
int tip_rng_ncl_init_state (const struct tip_rng_ncl_engine *engine)
{
	if ((engine == NULL) || (engine->state == NULL)) {
		return RNG_ENGINE_INVALID_ARGUMENT;
	}

	memset (engine->state, 0, sizeof (*engine->state));

	return 0;
}

/**
 * Initialize a tip ncl engine for generating random numbers.
 *
 * @param engine The tip ncl RNG engine to initialize.
 * @param state Variable context for RNG engine.
 *
 * @return 0 if the RNG engine was initialized successfully or an error code.
 */
int tip_rng_ncl_init (struct tip_rng_ncl_engine *engine, struct tip_rng_ncl_state *state,
	struct tip_rom_ncl_shared_state *shared_state)
{
	if (engine == NULL) {
		return RNG_ENGINE_INVALID_ARGUMENT;
	}
	if (shared_state == NULL) {
		return RNG_ENGINE_INVALID_ARGUMENT;
	}

	memset (engine, 0, sizeof (struct tip_rng_ncl_engine));

	engine->base.generate_random_buffer = tip_rng_ncl_generate_random_buffer;
	engine->state = state;
	engine->shared_state = shared_state;

	return tip_rng_ncl_init_state (engine);
}

/**
 * Release the resources used by an mbed TLS RNG engine.
 *
 * @param engine The mbed TLS RNG engine to release.
 */
void tip_rng_ncl_release (const struct tip_rng_ncl_engine *engine)
{
	if (engine != NULL && engine->shared_state) {
		engine->shared_state->lock (engine->shared_state);
		pROM_NCL_DRBG_FinalizeContext (engine->state->drbg_context);
		engine->shared_state->unlock (engine->shared_state);
	}
}
