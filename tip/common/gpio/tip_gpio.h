/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/
#ifndef TIP_GPIO_H_
#define TIP_GPIO_H_

#include <stddef.h>
#include "hal.h"

#define TIP_WOL_GPIO     4
#define TIP_WOL_GPIO_AB  false

#define TIP_AUX_BMC_RESTART_GPIO     1
#define TIP_AUX_BMC_RESTART_GPIO_AB  true

int tip_init_gpio ();
int tip_set_gpio (unsigned int gpio, unsigned int a_b_option, bool b_interrupt);
void tip_gpio_handler_complete (unsigned int gpio);

#endif /* TIP_GPIO_H_ */
