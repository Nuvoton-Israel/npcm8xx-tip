/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/
#ifndef TIP_RSA_NCL_H_
#define TIP_RSA_NCL_H_

#include "crypto/rsa.h"
#include "tip_security.h"
#include "tip_rom_ncl_shared_state.h"

/**
 * A TIP NCL context for RSA encryption.
 */
struct tip_rsa_ncl_engine {
	struct rsa_engine base;						/**< The base RSA engine. */
	uint8_t context[NCL_RSA_CONTEXT_BYTE_SIZE]; /**< Buffer for the RSA context used by the engine. */
	struct tip_rom_ncl_shared_state *shared_state;	/**< Shared state for synchronization. */
};

int tip_rsa_ncl_init (struct tip_rsa_ncl_engine *engine, struct tip_rom_ncl_shared_state *shared_state);
void tip_rsa_ncl_release (struct tip_rsa_ncl_engine *engine);


#endif /* TIP_RSA_NCL_H_ */
