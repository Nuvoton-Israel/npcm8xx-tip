/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/
#ifndef TIP_RNG_NCL_H_
#define TIP_RNG_NCL_H_

#include "tip_security.h"
#include "crypto/rng.h"
#include "tip_rom_ncl_shared_state.h"

struct tip_rng_ncl_state {
	uint8_t drbg_context[NCL_DRBG_CONTEXT_BYTE_SIZE]; /**< Buffer for the DRBG context used by the engine. */
};

/**
 * A Tip NCL context for RNG operations.
 */
struct tip_rng_ncl_engine {
	struct rng_engine base;							/**< The base RNG engine. */
	struct tip_rng_ncl_state *state;				/**< Variable context for RNG engine. */
	struct tip_rom_ncl_shared_state *shared_state;		/**< Shared state for synchronization. */
};

int tip_rng_ncl_init (struct tip_rng_ncl_engine *engine, struct tip_rng_ncl_state *state,
	struct tip_rom_ncl_shared_state *shared_state);
int tip_rng_ncl_init_state (const struct tip_rng_ncl_engine *engine);
void tip_rng_ncl_release (const struct tip_rng_ncl_engine *engine);


#endif // TIP_RNG_NCL_H_
