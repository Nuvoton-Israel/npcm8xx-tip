// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef FLASH_XFER_TIP_H_
#define FLASH_XFER_TIP_H_

#include <stdbool.h>


int flash_xfer_tip_mutex_init ();
void flash_xfer_tip_mutex_release ();

int flash_xfer_tip_set_xfer_in_progress ();
void flash_xfer_tip_clear_xfer_in_progress ();


#endif /* FLASH_XFER_TIP_H_ */
