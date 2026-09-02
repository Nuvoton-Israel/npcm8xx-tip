/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef FLASH_XFER_TIP_H_
#define FLASH_XFER_TIP_H_

#include <stdbool.h>


int flash_xfer_tip_mutex_init ();
void flash_xfer_tip_mutex_release ();

int flash_xfer_tip_set_xfer_in_progress ();
void flash_xfer_tip_clear_xfer_in_progress ();


#endif /* FLASH_XFER_TIP_H_ */
