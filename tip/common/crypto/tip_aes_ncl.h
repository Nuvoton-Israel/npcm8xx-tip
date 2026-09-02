/*
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2023 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 *<<<-------------------------------------------------------
 * File Contents:
 *   tip_aes_ncl.h
 *            This file contains the definition of aes engine for ncl
 *  Project:  Arbel
 */

#ifndef TIP_AES_NCL_H_
#define TIP_AES_NCL_H_

#include "crypto/aes.h"
#include "tip_security.h"

/**
 * An TIP NCL context for AES operations.
 */
struct tip_aes_ncl_engine {
	struct aes_engine base;						/**< The base AES engine. */
	uint8_t context[NCL_AES_CONTEXT_BYTE_SIZE]; /**< Context for AES-GCM operations. */
	NCL_AES_KEY_SIZE_T key_size;				/**< the AES key size */
	NCL_AES_MODE_T aes_mode;					/**< the type of AES mode (GCM\CCM) */
	uint8_t key_ind;							/**< the AES relevant key index */
	int (*set_mode) (struct tip_aes_ncl_engine *engine, NCL_AES_MODE_T mode);
	int (*select_key) (struct tip_aes_ncl_engine *engine, uint8_t key_num);
};

int tip_aes_ncl_init (struct tip_aes_ncl_engine *engine);
void tip_aes_ncl_release (struct tip_aes_ncl_engine *engine);


#endif /* TIP_AES_NCL_H_ */
