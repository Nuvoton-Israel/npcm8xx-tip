/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2024 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *    tip_gpio.h
 *            This file contains the TIP gpio handling
 * Project:
 *            Arbel
 *------------------------------------------------------------------------*/
#ifndef TIP_GPIO_H_
#define TIP_GPIO_H_

#include <stddef.h>
#include "hal.h"

#define TIP_WOL_GPIO     4
#define TIP_WOL_GPIO_AB  false

int tip_init_gpio (unsigned int gpio, unsigned int a_b_option, bool b_interrupt);
void tip_gpio_handler_complete (unsigned int gpio);

#endif /* TIP_GPIO_H_ */
