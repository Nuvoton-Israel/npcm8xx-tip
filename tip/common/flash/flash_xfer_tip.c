// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include "flash/flash_master.h"
#include "flash_xfer_tip.h"
#include "hal_regs.h"
#include "platform_api.h"


static platform_mutex flash_xfer_register_lock;
static bool xfer_mutex_init = false;

/**
 * Initialize the mutex for synchronizing flash xfer.
 *
 * @return 0 if mutex was initialized sucessfully, error otherwise.
 */
int flash_xfer_tip_mutex_init ()
{
	int status;

	if (!xfer_mutex_init) {
		status = platform_mutex_init (&flash_xfer_register_lock);
		if (status != 0) {
			return status;
		}
	}

	xfer_mutex_init = true;

	return status;
}

/**
 * Determine if BMC flash xfer is in progress.
 *
 * @return true if the BMC xfer is ongoing, false otherwise.
 */
static bool flash_xfer_tip_bmc_xfer_in_progress ()
{
	return (REG_READ (SCRPAD_10_41 (20)) ? true : false);
}

/**
 * Set a flag indicating TIP flash xfer is in progress.
 *
 * @note It is expected that no BMC write is in progress when this API is called.
 *
 * @return 0 if flag was successfully set, otherwise an error.
 *
 */
int flash_xfer_tip_set_xfer_in_progress ()
{
	int status = 0;
	platform_mutex_lock (&flash_xfer_register_lock);

	if (flash_xfer_tip_bmc_xfer_in_progress ()) {
		status = FLASH_MASTER_XFER_IN_PROGRESS;
		platform_mutex_unlock (&flash_xfer_register_lock);
	}
	else {
		/* Update Sratchpad register to indicate TIP flash xfer is in progress. */
		REG_WRITE (SCRPAD_10_41 (20), 1);
	}

	return status;
}

/**
 * Clear flag indicating NO TIP flash xfer is in progress
 * and release the mutex.  This call MUST BE preceded by @fn flash_xfer_tip_set_xfer_in_progress ()
 */
void flash_xfer_tip_clear_xfer_in_progress ()
{
	/* Update Sratchpad register to indicate BMC can initiate a flash xfer. */
	REG_WRITE (SCRPAD_10_41 (20), 0);
	platform_mutex_unlock (&flash_xfer_register_lock);
}


/**
 * Release the mutex without clearing the register.
 * This is API is ONLY called from BMC task after successfully loading BMC images and allows TIP
 * tasks that needs flash access to fail.
 * This call MUST BE preceded by @fn flash_xfer_tip_set_xfer_in_progress ()
 */
void flash_xfer_tip_mutex_release ()
{
	platform_mutex_unlock (&flash_xfer_register_lock);
}
