// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_AES_GCM_NCL_H_
#define TIP_AES_GCM_NCL_H_

#include "crypto/aes_gcm.h"
#include "tip_security.h"
#include "tip_rom_ncl_shared_state.h"


/**
 * Variable context for NCL AES-GCM operations.
 */
struct tip_aes_gcm_ncl_state {
	uint8_t context[NCL_AES_CONTEXT_BYTE_SIZE]; 	/**< Context for AES-GCM operations. */
	NCL_AES_KEY_SIZE_T key_size;					/**< the AES key size */
};

/**
 * An TIP NCL context for AES-GCM operations.
 */
struct tip_aes_gcm_ncl_engine {
	struct aes_gcm_engine base;						/**< The base AES engine. */
	struct tip_aes_gcm_ncl_state *state;			/**< Variable context for AES-GCM operations. */
	struct tip_rom_ncl_shared_state *shared_state;		/**< Shared state for synchronization. */
};

int tip_aes_gcm_ncl_init (struct tip_aes_gcm_ncl_engine *engine,
	struct tip_aes_gcm_ncl_state *state, struct tip_rom_ncl_shared_state *shared_state);
int tip_aes_gcm_ncl_init_state (const struct tip_aes_gcm_ncl_engine *engine);
void tip_aes_gcm_ncl_release (const struct tip_aes_gcm_ncl_engine *engine);


#endif /* TIP_AES_GCM_NCL_H_ */
