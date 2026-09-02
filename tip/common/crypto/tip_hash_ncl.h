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
#include "tip_rom_ncl_shared_state.h"
#include "riot/reference/include/RiotSha1.h"


/**
 * Variable context for hashing with NCL hash engine.
 */
struct tip_hash_ncl_engine_state {
	union {
#ifdef HASH_ENABLE_SHA1
		RIOT_SHA1_CONTEXT sha1; 					/**< Context for SHA1 hashes. */
#endif
		uint8_t ncl_sha[NCL_SHA_CONTEXT_BYTE_SIZE]; /**< Context for NCL SHA256/384/512 hash. */
	} context;
	uint8_t active; 								/**< The active hash context. */
};

/**
 * A TIP NCL context for Hash operations.
 *
 * NCL hash engine lacks the native support on SHA1. The RIOT hash SHA1 implementation is leveraged
 * to complement NCL.
 */
struct tip_hash_ncl_engine {
	struct hash_engine base; 					/**< The base hash engine. */
	struct tip_hash_ncl_engine_state *state;	/**< Variable context for the hash engine. */
	struct tip_rom_ncl_shared_state *shared_state;	/**< Shared state for synchronization. */
};

int tip_hash_ncl_init (struct tip_hash_ncl_engine *engine, struct tip_hash_ncl_engine_state *state,
	struct tip_rom_ncl_shared_state *shared_state);
int tip_hash_ncl_init_state (const struct tip_hash_ncl_engine *engine);
void tip_hash_ncl_release (struct tip_hash_ncl_engine *engine);
int tip_hash_ncl_calculate_sha512 (const struct hash_engine *engine, const uint8_t *data,
        size_t length, uint8_t *hash, size_t hash_length);
int tip_hash_ncl_start_sha512 (const struct hash_engine *engine);


#endif /* TIP_HASH_NCL_H_ */
