/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2024 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *   tip_gpio.c
 *            This file contains the TIP gpio handling
 *  Project:  Arbel
 *------------------------------------------------------------------------*/
#include <string.h>
#include "tip_gpio.h"
#include "tip_boot.h"
#include "tip_rom_utils.h"
#include "platform_io.h"
#include "platform_api.h"
#include "hal_regs.h"
#include "cmd_interface/config_reset.h"
#include "tip_reset.h"

static unsigned int gpio_val;

static void tip_gpio_handler (uint16_t val)
{
	unsigned int gpio = GPIO_TIP_GET_BIT_NUM (gpio_val);

	/* disable GPIO interrupt in NVIC (shared by all TIP_GPIOs) */
	NVIC_EnableInt (GPIO_TIP_INTERRUPT (0), false);
	for (unsigned int i = 0 ; i < GPIO_TIP_NUM_OF_GPIOS ; i++) {
		if (GPIO_TIP_GetInterruptStatus (GPIO_TIP_GET_BIT_NUM(i)) == TRUE) {
			platform_printf_dbg ("GPIO%d int val = %d" NEWLINE, i, GPIO_TIP_Read (i));

			/* Handling of the GPIO interrupt */
			if (i == gpio) {
				GPIO_TIP_EnableInterrupt (i, false);
				/* GPIO interrupt is used as WOL reset.
				 * So handle it like a BMC reset (same flow)
				 */
				NVIC_BMC_reset (46);
			}
			else {
				/* Ignore interrupts from other TP GPIOs */
				GPIO_TIP_ClearInterrupt (i);
			}
		}
	}

	/* restore NVIC */
	NVIC_ClearInt (GPIO_TIP_INTERRUPT (i));
	NVIC_EnableInt (GPIO_TIP_INTERRUPT (0), true);
}

/**
 * TIP GPIO complete handling.
 *
 * @param gpio  GPIO number 0:7
 */
void tip_gpio_handler_complete (unsigned int gpio)
{
	GPIO_TIP_ClearInterrupt (gpio);
	GPIO_TIP_EnableInterrupt (gpio, true);
}

/**
 * Init TIP Wakeup GPIO
 *
 * @param gpio  GPIO number 0:7
 * @param a_b_option GPIO muxing option
 * @param b_interrupt Enable the GPIO interupt.
 */
int tip_init_gpio (unsigned int gpio, unsigned int a_b_option, bool b_interrupt)
{
	gpio_val = (gpio & 0x07 ) | (a_b_option & 0x0F << 4);

	if (gpio >= GPIO_TIP_NUM_OF_GPIOS) {
		return CONFIG_RESET_INVALID_ARGUMENT;
	}

	NVIC_EnableInt (GPIO_TIP_INTERRUPT (gpio_val), false);
	
	/* Init all unused TIP GPIO as input */
	for (int i = 0 ; i < GPIO_TIP_NUM_OF_GPIOS ; i++) {
		GPIO_TIP_EnableInterrupt (i, false);
		GPIO_TIP_SetInterruptType(i, GPIO_TIP_INTR_NONE);
		GPIO_TIP_ClearInterrupt (i);
	}

	/* Init the used GPIO with a pull up, active low */
	GPIO_TIP_Init (gpio_val, GPIO_TIP_DIR_INPUT, GPIO_TIP_PULL_UP, GPIO_TIP_OTYPE_PUSH_PULL, FALSE);

	/* Bind the inetrrupt handler */
	if (b_interrupt == true) {
		GPIO_TIP_ConfigInterrupt (gpio_val, GPIO_TIP_INTR_FALLING, tip_gpio_handler);

		/* Clear all GPIO interrupts */
		GPIO_TIP_ClearInterrupt (TIP_GPIO_ALL);
		GPIO_TIP_EnableInterrupt (gpio_val, true);

		/* clear and enable interrupt in NVIC */
		NVIC_ClearInt (GPIO_TIP_INTERRUPT (gpio_val));
		NVIC_EnableInt (GPIO_TIP_INTERRUPT (gpio_val), true);
	}

	platform_printf ("Init wakeup TIP_GPIO%d %c" NEWLINE, gpio, (a_b_option) ? 'B': 'A');

	return 0;
}
