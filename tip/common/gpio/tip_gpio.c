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

static unsigned int gpio_val[GPIO_TIP_NUM_OF_GPIOS];
const unsigned int GPIO_NOT_USED = 0xFF;

static void tip_gpio_handler (uint16_t val)
{
	unsigned int gpio;
	bool b_reset = false;

	/* disable GPIO interrupt in NVIC (shared by all TIP_GPIOs) */
	NVIC_EnableInt (GPIO_TIP_INTERRUPT (0), false);
	for (unsigned int i = 0 ; i < GPIO_TIP_NUM_OF_GPIOS ; i++) {
		if (GPIO_TIP_GetInterruptStatus (GPIO_TIP_GET_BIT_NUM(i)) == TRUE) {
			gpio  = GPIO_TIP_GET_BIT_NUM (gpio_val[i]);

			/* Clearing the interrupt */
			GPIO_TIP_ClearInterrupt (GPIO_TIP_GET_BIT_NUM(i));
			/* Handling of the GPIO interrupt */
			if (i == gpio) {
				GPIO_TIP_EnableInterrupt (gpio, false);

				/* GPIO interrupt is used as WOL reset or Aux BMC recovery.
				 * So handle it like a BMC reset (same flow)
				 */
				b_reset = true;
				break;
			}
		}
	}

	if (b_reset == true) {
		if (gpio == TIP_WOL_GPIO)
		  SET_REG_FIELD (TIP_SCR1, TIP_RST_INDICATION_WOL, 1);
		else if (gpio == TIP_AUX_BMC_RESTART_GPIO)
		  SET_REG_FIELD (TIP_SCR1, TIP_RST_INDICATION_FORCE_RESTART, 1);
		NVIC_BMC_reset (46);
	}

	/* restore NVIC , doesn't matter the number passed, macro refers to a fixed value */
	NVIC_ClearInt (GPIO_TIP_INTERRUPT (0));
	NVIC_EnableInt (GPIO_TIP_INTERRUPT (0), true);

	platform_printf("tip_gpio_handler b_reset = %s " NEWLINE, b_reset ? "TRUE" : "FALSE");
        return;
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
 * Init TIP GPIO default initialization
 */
int tip_init_gpio ()
{
	/* disable GPIO interrupt in NVIC (shared by all TIP_GPIOs) */
	NVIC_EnableInt(GPIO_TIP_INTERRUPT(0), false);
	NVIC_ClearInt(GPIO_TIP_INTERRUPT(0));

	/* Init all unused TIP GPIO as input */
	for (int i = 0 ; i < GPIO_TIP_NUM_OF_GPIOS ; i++) {
		GPIO_TIP_EnableInterrupt (i, false);
		GPIO_TIP_SetInterruptType(i, GPIO_TIP_INTR_NONE);
		GPIO_TIP_ClearInterrupt (i);
		gpio_val[i] = GPIO_NOT_USED;
	}

	/* Clear all GPIO interrupts */
	GPIO_TIP_ClearInterrupt (TIP_GPIO_ALL);

	platform_printf("tip_init_gpio completed " NEWLINE);
	return 0;
}

/**
 * Set TIP GPIO
 *
 * @param gpio  GPIO number 0:7
 * @param a_b_option GPIO muxing option
 * @param b_interrupt Enable the GPIO interupt.
 */
int tip_set_gpio (unsigned int gpio, unsigned int a_b_option, bool b_interrupt)
{
	if (gpio >= GPIO_TIP_NUM_OF_GPIOS) {
		return CONFIG_RESET_INVALID_ARGUMENT;
	}

	if (gpio_val[gpio] != GPIO_NOT_USED) {
		platform_printf ("WARNING: gpio %d already set previously\n", gpio);
		// TBD: print warning or override previous settings?
	}
	gpio_val[gpio] = (gpio & 0x07 ) | ((a_b_option & 0x0F) << 4);

	NVIC_EnableInt (GPIO_TIP_INTERRUPT (gpio_val[gpio]), false);

	/* Init the used GPIO with a pull up, active high , and GPIO MUX config locked */
	GPIO_TIP_Init (gpio_val[gpio], GPIO_TIP_DIR_INPUT, GPIO_TIP_PULL_UP, GPIO_TIP_OTYPE_PUSH_PULL, FALSE, TRUE);

	/* Bind the inetrrupt handler */
	if (b_interrupt == true) {
		GPIO_TIP_ConfigInterrupt (gpio_val[gpio], GPIO_TIP_INTR_FALLING, tip_gpio_handler);

		/* Clear and enable interrupt */
		GPIO_TIP_ClearInterrupt (gpio_val[gpio]);

		GPIO_TIP_EnableInterrupt (gpio_val[gpio], true);

		/* clear and enable interrupt in NVIC */
		NVIC_ClearInt (GPIO_TIP_INTERRUPT (gpio_val[gpio]));
		NVIC_EnableInt (GPIO_TIP_INTERRUPT (gpio_val[gpio]), true);

	}
	platform_printf("tip_set_gpio gpio %d, interrupt %s, option %c " NEWLINE, gpio, (b_interrupt) ? "Yes" : "No",
                        (a_b_option) ? 'B': 'A');
	return 0;
}
