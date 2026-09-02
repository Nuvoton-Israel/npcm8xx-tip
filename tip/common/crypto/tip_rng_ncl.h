/*
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2023 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 *<<<-------------------------------------------------------
 * File Contents:
 *   tip_rng_ncl.h
 *            This file contains the definition of aes engine for ncl
 *  Project:  Arbel
 */


#ifndef TIP_RNG_NCL_H_
#define TIP_RNG_NCL_H_

#include "tip_security.h"
#include "crypto/rng.h"

/**
 * A Tip NCL context for RNG operations.
 */
struct tip_rng_ncl_engine {
	struct rng_engine base;							  /**< The base RNG engine. */
	uint8_t drbg_context[NCL_DRBG_CONTEXT_BYTE_SIZE]; /**< Buffer for the DRBG context used by the engine. */
};

int	 tip_rng_ncl_init (struct tip_rng_ncl_engine *engine);
void tip_rng_ncl_release (struct tip_rng_ncl_engine *engine);


#endif // TIP_RNG_NCL_H_
