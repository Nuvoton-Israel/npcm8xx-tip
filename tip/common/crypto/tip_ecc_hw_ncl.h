/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/
#ifndef TIP_ECC_HW_NCL_H_
#define TIP_ECC_HW_NCL_H_

#include "platform_api.h"
#include "tip_security.h"
#include "crypto/ecc_hw.h"
#include "tip_rom_ncl_shared_state.h"

/**
 * Variable context for NCL ECC operations.
 */
struct tip_ecc_hw_ncl_state {
	uint8_t context[NCL_ECC_CONTEXT_BYTE_SIZE];		/**< Buffer for the ECC context used by the engine. */
};

/**
 * A TIP NCL context for ECC operations.
 */
struct tip_ecc_hw_ncl_engine {
	struct ecc_hw base;								/**< The base ECC HW instance. */
	struct tip_ecc_hw_ncl_state *state;				/**< The variable context for ecc engine. */
	struct tip_rom_ncl_shared_state *shared_state;		/**< Shared state for synchronization. */
};


int tip_ecc_hw_ncl_init (struct tip_ecc_hw_ncl_engine *engine, struct tip_ecc_hw_ncl_state *state,
	struct tip_rom_ncl_shared_state *shared_state);
int tip_ecc_hw_ncl_init_state (const struct tip_ecc_hw_ncl_engine *engine);
void tip_ecc_hw_ncl_release (struct tip_ecc_hw_ncl_engine *engine);


#endif /* TIP_ECC_HW_NCL_H_ */
