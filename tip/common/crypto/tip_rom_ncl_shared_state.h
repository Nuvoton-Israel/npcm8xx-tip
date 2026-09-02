/*
 *  Copyright (c) Nuvoton Technology Corporation. All rights reserved.
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT license.
 */

#ifndef TIP_ROM_NCL_SHARED_STATE_H_
#define TIP_ROM_NCL_SHARED_STATE_H_

#include "platform_api.h"

/**
 * Base interface for cross-engine synchronization of ROM NCL API calls.
 * Implementations provide lock/unlock to serialize hardware access.
 * A single initialized instance must be passed to all NCL-based engines.
 */
struct tip_rom_ncl_shared_state {
	void (*lock) (struct tip_rom_ncl_shared_state *state);		/**< Acquire the lock. */
	void (*unlock) (struct tip_rom_ncl_shared_state *state);	/**< Release the lock. */
};

/**
 * FreeRTOS implementation: uses a platform_mutex for thread-safe serialization.
 * Use in L1 (multithreaded) context.
 */
struct tip_rom_ncl_shared_state_freertos {
	struct tip_rom_ncl_shared_state base;	/**< Must be first. */
	platform_mutex mutex;					/**< Mutex instance. */
};

/**
 * Boot (bare-metal) implementation: no-op lock/unlock, for single-threaded L0 context.
 */
struct tip_rom_ncl_shared_state_boot {
	struct tip_rom_ncl_shared_state base;	/**< Must be first. */
};

int tip_rom_ncl_shared_state_freertos_init (struct tip_rom_ncl_shared_state_freertos *state);
void tip_rom_ncl_shared_state_freertos_release (struct tip_rom_ncl_shared_state_freertos *state);
int tip_rom_ncl_shared_state_boot_init (struct tip_rom_ncl_shared_state_boot *state);

/**
 * Variant of NCL_STATUS_RET_CHECK that unlocks shared_state before returning on error.
 * @param func         The NCL function call to check.
 * @param shared_state Non-NULL pointer to tip_rom_ncl_shared_state.
 */
#define NCL_STATUS_RET_CHECK_MUTEX(func, shared_state)  \
{                                                       \
	NCL_STATUS_T ret;                                   \
	if ((ret = func) != NCL_STATUS_OK) {                \
		(shared_state)->unlock (shared_state);          \
		return ret;                                     \
	}                                                   \
}


#endif /* TIP_ROM_NCL_SHARED_STATE_H_ */
