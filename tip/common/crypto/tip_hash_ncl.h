/*
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2023 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 *<<<-------------------------------------------------------
 * File Contents:
 *   tip_hash_ncl.h
 *            This file contains the definition of aes engine for ncl
 *  Project:  Arbel
 */

#ifndef TIP_HASH_NCL_H_
#define TIP_HASH_NCL_H_

#include "tip_security.h"

/**
 * A TIP NCL context for Hash operations.
 */
struct tip_hash_ncl_engine {
	struct hash_engine base; /**< The base hash engine. */
	uint8_t context[NCL_SHA_CONTEXT_BYTE_SIZE];
};

int tip_hash_ncl_init (struct tip_hash_ncl_engine *engine);
void tip_hash_ncl_release (struct tip_hash_ncl_engine *engine);


#endif /* TIP_HASH_NCL_H_ */
