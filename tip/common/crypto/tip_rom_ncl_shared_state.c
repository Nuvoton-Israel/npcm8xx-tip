/*
 *  Copyright (c) Nuvoton Technology Corporation. All rights reserved.
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT license.
 */

#include "tip_rom_ncl_shared_state.h"
#include "common/unused.h"


/* --- FreeRTOS implementation --- */

static void tip_rom_ncl_shared_state_freertos_lock (struct tip_rom_ncl_shared_state *state)
{
	struct tip_rom_ncl_shared_state_freertos *s =
		(struct tip_rom_ncl_shared_state_freertos *) state;

	platform_mutex_lock (&s->mutex);
}

static void tip_rom_ncl_shared_state_freertos_unlock (struct tip_rom_ncl_shared_state *state)
{
	struct tip_rom_ncl_shared_state_freertos *s =
		(struct tip_rom_ncl_shared_state_freertos *) state;

	platform_mutex_unlock (&s->mutex);
}

/**
 * Initialize a FreeRTOS-backed shared state for NCL engine serialization.
 *
 * @param state The shared state instance to initialize.
 *
 * @return 0 on success or an error code.
 */
int tip_rom_ncl_shared_state_freertos_init (struct tip_rom_ncl_shared_state_freertos *state)
{
	int status;

	if (state == NULL) {
		return -1;
	}

	status = platform_mutex_init (&state->mutex);
	if (status != 0) {
		return status;
	}

	state->base.lock = tip_rom_ncl_shared_state_freertos_lock;
	state->base.unlock = tip_rom_ncl_shared_state_freertos_unlock;

	return 0;
}

/**
 * Release resources for a FreeRTOS-backed shared state.
 *
 * @param state The shared state instance to release.
 */
void tip_rom_ncl_shared_state_freertos_release (struct tip_rom_ncl_shared_state_freertos *state)
{
	if (state != NULL) {
		platform_mutex_free (&state->mutex);
	}
}


/* --- Boot (bare-metal) implementation --- */

static void tip_rom_ncl_shared_state_boot_lock (struct tip_rom_ncl_shared_state *state)
{
	UNUSED (state);
}

static void tip_rom_ncl_shared_state_boot_unlock (struct tip_rom_ncl_shared_state *state)
{
	UNUSED (state);
}

/**
 * Initialize a no-op shared state for bare-metal (L0) NCL engine use.
 *
 * @param state The shared state instance to initialize.
 *
 * @return 0 on success or an error code.
 */
int tip_rom_ncl_shared_state_boot_init (struct tip_rom_ncl_shared_state_boot *state)
{
	if (state == NULL) {
		return -1;
	}

	state->base.lock = tip_rom_ncl_shared_state_boot_lock;
	state->base.unlock = tip_rom_ncl_shared_state_boot_unlock;

	return 0;
}
