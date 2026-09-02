/*
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2023 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 *<<<-------------------------------------------------------
 * File Contents:
 *   tip_ecc_hw_ncl.h
 *            This file contains the definition of ecc hw engine for ncl
 *  Project:  Arbel
 */

#ifndef TIP_ECC_HW_NCL_H_
#define TIP_ECC_HW_NCL_H_

#include "tip_security.h"
#include "crypto/ecc_hw.h"

/**
 * A TIP NCL context for ECC operations.
 */
struct tip_ecc_hw_ncl_engine {
	struct ecc_hw base;								/**< The base ECC HW instance. */
	uint8_t context[NCL_ECC_CONTEXT_BYTE_SIZE];		/**< Buffer for the ECC context used by the engine. */
};

int tip_ecc_hw_ncl_init (struct tip_ecc_hw_ncl_engine *engine);
void tip_ecc_hw_ncl_release (struct tip_ecc_hw_ncl_engine *engine);


#endif /* TIP_ECC_HW_NCL_H_ */