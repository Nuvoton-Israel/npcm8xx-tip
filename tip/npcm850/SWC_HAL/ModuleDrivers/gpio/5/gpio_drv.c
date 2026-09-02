/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2010-2024 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   gpio_drv.c                                                                                            */
/*            This file contains General Purpose I/O (GPIO) driver implementation                          */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_DRV()

#include "gpio_drv.h"
#include "gpio_regs.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                  LOCAL FUNCTIONS FORWARD DECLARATIONS                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#if defined (GPIO_EVENT_SUPPORT) || (defined (MIWU_MODULE_TYPE) && defined (GPIO_MIWU_INTERRUPT_SUPPORT))
static GPIO_HANDLER_T GPIO_callback[GPIO_NUM_OF_PORTS] = {0};
BOOLEAN GPIO_GetInterruptStatus (UINT gpio_a_b);
GPIO_EVENT_T GPIO_GetEventType(UINT GPIO);
#endif
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 MACROS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define  GPIO_GET_VAL_INIT(ind)  \
	UINT a_or_b   = GPIO_GET_PORT_NUM(ind);  \
	UINT gpio = GPIO_GET_BIT_NUM (ind);  \
	DEFS_STATUS_COND_CHECK_ACTION(gpio < GPIO_NUM_OF_GPIOS, while(1));    \
	DEFS_STATUS_COND_CHECK_ACTION(a_or_b < GPIO_NUM_OF_PORTS, while(1));

#define  GPIO_GET_VAL(ind)  \
	UINT a_or_b   = GPIO_GET_PORT_NUM(ind);  \
	UINT gpio = GPIO_GET_BIT_NUM (ind);  \
	DEFS_STATUS_COND_CHECK_ACTION(gpio < GPIO_NUM_OF_GPIOS, while(1));    \
	DEFS_STATUS_COND_CHECK_ACTION(a_or_b < GPIO_NUM_OF_PORTS, while(1));


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_EnableInput                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to configure                                                   */
/*                  enable    - TRUE: Enable the relevant pin I/O buffer                                   */
/*                              FALSE: Disable the relevant pin I/O buffer.                                */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Enable or Disable the I/O buffer of the relevant pin (related to the GPIO)             */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_EnableInput (UINT gpio_a_b, BOOLEAN enable)
{
	// No IO Buffer required (no MIWU)
	HAL_PRINT("Warning - Buffer type is not supported\n");
}

#if defined (GPIO_EVENT_SUPPORT) || (defined (MIWU_MODULE_TYPE) && defined (GPIO_MIWU_INTERRUPT_SUPPORT))
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_ClearInterrupt                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to clear                                                       */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Clears a pending event of a given GPIO input                                           */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_ClearInterrupt (UINT gpio_a_b)
{
	GPIO_GET_VAL(gpio_a_b);

	SET_REG_FIELD (GPIOIST, GPIOIST_GPST(gpio), 1);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_InstallHandler                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to configure                                                   */
/*                  handler   - gpio handler procedure to be installed.                                    */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Install a handler function to the relevant GPIO.                                       */
/*                  The interrupt itself is not enabled and not configured                                 */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_InstallHandler (UINT gpio_a_b, GPIO_HANDLER_T handler)
{
	/*-----------------------------------------------------------------------------------------------------*/
	/* Configure GPIO Event according to arguments                                                         */
	/*-----------------------------------------------------------------------------------------------------*/

	/*-----------------------------------------------------------------------------------------------------*/
	/* Check for Illegal Cases                                                                             */
	/*-----------------------------------------------------------------------------------------------------*/
	GPIO_GET_VAL(gpio_a_b);

	/*-----------------------------------------------------------------------------------------*/
	/* Setting event handler                                                                   */
	/*-----------------------------------------------------------------------------------------*/
	if (handler != NULL)
	{
		GPIO_callback[gpio]   = handler;
		/*-------------------------------------------------------------------------------------*/
		/* Registering event handler to AIC                                                    */
		/*-------------------------------------------------------------------------------------*/
		INTERRUPT_REGISTER_AND_ENABLE(GPIO_INTERRUPT_PROVIDER, GPIO_INTERRUPT(gpio), handler,
							GPIO_INTERRUPT_POLARITY, GPIO_INTERRUPT_PRIORITY);
	}
	else
	{
		GPIO_callback[gpio]    = NULL;
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_SetInterruptType                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to configure                                                   */
/*                  intType   - Interrupt type, on of the following:                                       */
/*                                GPIO_INTR_RISING, GPIO_INTR_FALLING, GPIO_INTR_HIGH,                     */
/*                                GPIO_INTR_LOW, GPIO_INTR_ANY_EDGE, GPIO_INTR_NONE                        */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Configure the interrupt type for the specific GPIO                                     */
/*                  The interrupt itself is not enabled                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_SetInterruptType (UINT gpio_a_b, UINT intType)
{
	/*-----------------------------------------------------------------------------------------------------*/
	/* Error checking                                                                                      */
	/*-----------------------------------------------------------------------------------------------------*/
	GPIO_GET_VAL(gpio_a_b);
	if (intType == GPIO_INTR_NONE)
	{
		// Clear Event and polarity of this pin (assuming gpio is idle)
		SET_REG_FIELD(GPIOILG, GPIOILG_GPBE(gpio), 0);
		SET_REG_FIELD(GPIOILG, GPIOILG_GPLG(gpio), 1); /* edge */
		SET_REG_FIELD(GPIOIEP, GPIOIEP_GPPL(gpio), 0); /* rising */
	}
	else
	{
		if ((intType == GPIO_INTR_HIGH) || (intType ==  GPIO_INTR_LOW))
		{
			SET_REG_FIELD(GPIOILG, GPIOILG_GPLG(gpio), 0); // select Level (High/low)
			SET_REG_FIELD(GPIOILG, GPIOILG_GPBE(gpio), 0); // clear both edges setting
			SET_REG_FIELD(GPIOIEP, GPIOIEP_GPPL(gpio), (intType == GPIO_INTR_HIGH) ? 1 : 0);
		}

		else  // edge event
		{
			SET_REG_FIELD(GPIOILG, GPIOILG_GPLG(gpio), 1); // select edge (rise/fall/both)
			if (intType == GPIO_INTR_ANY_EDGE)
			{
				SET_REG_FIELD(GPIOILG, GPIOILG_GPBE(gpio), 1);
				SET_REG_FIELD(GPIOIEP, GPIOIEP_GPPL(gpio), 0);
			}
			else
			{
				SET_REG_FIELD(GPIOILG, GPIOILG_GPBE(gpio), 0);
				SET_REG_FIELD(GPIOIEP, GPIOIEP_GPPL(gpio), (intType == GPIO_INTR_RISING) ? 1 : 0);
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_ConfigInterrupt                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to configure                                                   */
/*                  intType   - Interrupt type, one of the following:                                      */
/*                              GPIO_INTR_RISING, GPIO_INTR_FALLING, GPIO_INTR_ANY_EDGE                    */
/*                              GPIO_INTR_LOW, GPIO_INTR_HIGH                                              */
/*                  handler   - gpio handler procedure to be installed.                                    */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Configure the interrupt type for the specific GPIO                                     */
/*                  Install the handler and enable the relevant interrupt.                                 */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_ConfigInterrupt (UINT gpio_a_b, UINT intType, GPIO_HANDLER_T handler)
{
	GPIO_EnableInterrupt(gpio_a_b, false);
	GPIO_ClearInterrupt(gpio_a_b);
	GPIO_SetInterruptType(gpio_a_b, intType);
	GPIO_InstallHandler(gpio_a_b, handler); /* clear, set and enable NVIC interrupt */
	GPIO_EnableInterrupt(gpio_a_b, true);
	/* Note: interrupt is now cleared and enabled, both in GPIO module and in NVIC */
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_EnableInterrupt                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio     - gpio number in hex e.g 0x84 = GPIO84                                        */
/*                  enable   - TRUE to enable the gpio interrupt ; FALSE to disable.                       */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Description:     This routine enables/disables a given gpio interrupt.                                  */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_EnableInterrupt (UINT gpio_a_b , BOOLEAN enable)
{
	GPIO_GET_VAL(gpio_a_b);
	SET_REG_FIELD(GPIOIEP, GPIOIEP_GPIE(gpio), enable);
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_Init                                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gp -                                                                                   */
/*                  gpioDir -                                                                              */
/*                  outType -                                                                              */
/*                  pullDir -                                                                              */
/*                  vddDriven -                                                                            */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs init of a single tip gpio.                                       */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_Init (
	UINT            gp,
	GPIO_DIR_T      gpioDir,
	GPIO_PULL_T     pullDir,
	GPIO_OTYPE_T    outType,
	BOOLEAN         vddDriven
)
{
	/*-----------------------------------------------------------------------------------------------------*/
	/* Error checking                                                                                      */
	/*-----------------------------------------------------------------------------------------------------*/
	UINT a_or_b   = GPIO_GET_PORT_NUM(gp);
	UINT gpio = GPIO_GET_BIT_NUM (gp);
	DEFS_STATUS_COND_CHECK_ACTION(gpio < GPIO_NUM_OF_GPIOS, ASSERT(1));
	DEFS_STATUS_COND_CHECK_ACTION(a_or_b < GPIO_NUM_OF_PORTS, ASSERT(1));

	/*-----------------------------------------------------------------------------------------------------*/
	/* Disable interrupt while gpio is being initiatited                                                   */
	/*-----------------------------------------------------------------------------------------------------*/
	GPIO_EnableInterrupt (gpio, false);

	/*-----------------------------------------------------------------------------------------------------*/
	/* Mux GPIO                                                                                            */
	/*-----------------------------------------------------------------------------------------------------*/
	GPIO_MUX(gpio, a_or_b);

	/*-----------------------------------------------------------------------------------------------------*/
	/* Configure Pull                                                                                      */
	/*-----------------------------------------------------------------------------------------------------*/
	GPIO_ConfigPull(gpio, pullDir);

#if defined GPIO_CAPABILITY_VDD_DRIVEN
	/*-----------------------------------------------------------------------------------------------------*/
	/* Config if VDD driven                                                                                */
	/*-----------------------------------------------------------------------------------------------------*/
	GPIO_ConfigVddDriven(gpio, vddDriven);
#endif

	/*-----------------------------------------------------------------------------------------------------*/
	/* Configure direction                                                                                 */
	/*-----------------------------------------------------------------------------------------------------*/
	GPIO_ConfigDir(gpio, gpioDir);

	/*-----------------------------------------------------------------------------------------------------*/
	/* By default: no interrupt is enabled                                                                 */
	/*-----------------------------------------------------------------------------------------------------*/
	GPIO_SetInterruptType(gpio, GPIO_INTR_NONE);
	GPIO_ClearInterrupt (gpio);
	
#if defined (GPIO_EVENT_SUPPORT) || (defined (MIWU_MODULE_TYPE) && defined (GPIO_MIWU_INTERRUPT_SUPPORT))	
	GPIO_callback[gpio] = NULL;
#endif
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_ConfigDir                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                  gpioDir - GPIO pin direction [INPUT(0) or OUTPUT(1)].                                  */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the direction (INPUT or OUTPUT) of a given GPIO pin.           */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_ConfigDir (
	UINT        gpio_a_b,
	GPIO_DIR_T  gpioDir
)
{
	GPIO_GET_VAL(gpio_a_b);

	/*-----------------------------------------------------------------------------------------------------*/
	/* Configure direction                                                                                 */
	/*-----------------------------------------------------------------------------------------------------*/
	// GPIO_DIR_OUTPUT == 1 GPIO_DIR_INPUT == 0;
	SET_REG_FIELD(GPIODOD, GPIODOD_GPOE(gpio), gpioDir);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_ConfigPull                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio     - GPIO number.                                                                */
/*                  pullDir  - Pull selection [GPIO_PULL_NONE (0) or GPIO_PULL_UP(1) or GPIO_PULL_DOWN(2)] */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables/disables the pull up feature of the given GPIO pin and configures */
/*                  it to pull-up or pull-down (when applicable).                                          */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_ConfigPull (
	UINT        gpio_a_b,
	GPIO_PULL_T pullDir
)
{
	/*-----------------------------------------------------------------------------------------------------*/
	/* Change Pull-Up/Down Enable and Selection according to arguments                                     */
	/*-----------------------------------------------------------------------------------------------------*/
	GPIO_GET_VAL(gpio_a_b);

	/*-----------------------------------------------------------------------------------------------------*/
	/*  Select PU or PD                                                                                    */
	/*-----------------------------------------------------------------------------------------------------*/
	if (pullDir == GPIO_PULL_NONE)
	{
		SET_REG_FIELD(GPIOPUD, GPIOPUD_GPPU(gpio), 0);
		SET_REG_FIELD(GPIOPUD, GPIOPUD_GPPD(gpio), 0);
	}
	else if (pullDir == GPIO_PULL_DOWN)
	{
		SET_REG_FIELD(GPIOPUD, GPIOPUD_GPPU(gpio), 0);
		SET_REG_FIELD(GPIOPUD, GPIOPUD_GPPD(gpio), 1);
	}
	else  // (pullDir == GPIO_PULL_UP)
	{
		SET_REG_FIELD(GPIOPUD, GPIOPUD_GPPU(gpio), 1);
		SET_REG_FIELD(GPIOPUD, GPIOPUD_GPPD(gpio), 0);
	}
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_GetDir                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                                                                                                         */
/* Returns:         GPIO pin direction                                                                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the GPIO pin direction (INPUT or OUTPUT)                          */
/*---------------------------------------------------------------------------------------------------------*/
GPIO_DIR_T GPIO_GetDir (UINT gpio_a_b)
{

	GPIO_GET_VAL(gpio_a_b);

	/*-----------------------------------------------------------------------------------------------------*/
	/* Configure the direction of the GPIOx pins according to arguments                                    */
	/*-----------------------------------------------------------------------------------------------------*/
	//GPIO_DIR_OUTPUT == 1 GPIO_DIR_INPUT == 0;
	return (GPIO_DIR_T)(READ_REG_FIELD(GPIODOD, GPIODOD_GPDO(gpio)));
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_GetPullUp                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    -    The number of the GPIO queried                                            */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine is a getter for the pullup on a given GPIO                                */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN GPIO_GetPullUp (UINT gpio_a_b)
{
	GPIO_GET_VAL(gpio_a_b);
	return (BOOLEAN)(READ_REG_FIELD(GPIOPUD, GPIOPUD_GPPU(gpio)));
}

#if defined GPIO_CAPABILITY_VDD_DRIVEN
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_ConfigVddDriven                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO pin number.                                                           */
/*                  vddDriven - TRUE to enable GPIO by VDD Present Control; FALSE otherwise.               */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures a given GPIO pin to be VDD-driven.                             */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_ConfigVddDriven (
	UINT      gpio_a_b,
	BOOLEAN  vddDriven
)
{
	GPIO_GET_VAL(gpio_a_b);
	SET_REG_FIELD(GPIOVDD, GPIOVDD_GPVD(gpio), vddDriven);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_GetVddDriven                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    -    The number of the GPIO queried                                            */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine is a getter for the vdd Driven state of a given GPIO                      */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN GPIO_GetVddDriven (UINT gpio_a_b)
{
	GPIO_GET_VAL(gpio_a_b);
	return (BOOLEAN)READ_REG_FIELD(GPIOVDD, GPIOVDD_GPVD(gpio));
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_Read                                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio - GPIO pin number.                                                                */
/*                                                                                                         */
/* Returns:         Read value (1 or 0).                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reads data from a given GPIO pin.                                         */
/*---------------------------------------------------------------------------------------------------------*/
UINT GPIO_Read (UINT gpio_a_b)
{
	GPIO_GET_VAL(gpio_a_b);

	/*-----------------------------------------------------------------------------------------------------*/
	/* Read GPIO pin value according to arguments                                                          */
	/*-----------------------------------------------------------------------------------------------------*/
	return (READ_REG_FIELD(GPIODIN, GPIODIN_GPI(gpio)));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_Write                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio  - GPIO pin number.                                                               */
/*                  value - Value to be written (0 or 1).                                                  */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine writes data to a given GPIO port.                                         */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_Write (
	UINT   gpio_a_b,
	UINT   value
)
{
	GPIO_GET_VAL(gpio_a_b);
	/*-----------------------------------------------------------------------------------------------------*/
	/* Set the output values of the GPIO pin according to arguments                                        */
	/*-----------------------------------------------------------------------------------------------------*/
	SET_REG_FIELD(GPIODOD, GPIODOD_GPDO(gpio), value);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_ReadOutput                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio - GPIO pin number.                                                                */
/*                                                                                                         */
/* Returns:         Read value (1 or 0).                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reads data from output buffer of the given GPIO pin.                      */
/*---------------------------------------------------------------------------------------------------------*/
UINT GPIO_ReadOutput (UINT gpio_a_b)
{
	GPIO_GET_VAL(gpio_a_b);
	return (READ_REG_FIELD(GPIODOD, GPIODOD_GPDO(gpio)));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_GetPullType                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                                                                                                         */
/* Returns:         GPIO_PULL_NONE, GPIO_PULL_UP, GPIO_PULL_DOWN                                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the GPIO PULL Status                                              */
/*---------------------------------------------------------------------------------------------------------*/
GPIO_PULL_T GPIO_GetPullType (UINT gpio_a_b)
{
	GPIO_GET_VAL(gpio_a_b);

	/*-----------------------------------------------------------------------------------------------------*/
	/*  Read PU or PD                                                                                      */
	/*-----------------------------------------------------------------------------------------------------*/
	if (READ_REG_FIELD(GPIOPUD, GPIOPUD_GPPU(gpio)) == 1)
	{
		return GPIO_PULL_UP;
	}
	else if (READ_REG_FIELD(GPIOPUD, GPIOPUD_GPPD(gpio)) == 1)
	{
		return GPIO_PULL_DOWN;
	}
	return GPIO_PULL_NONE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_GetOutputType                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                                                                                                         */
/* Returns:         GPIO_OTYPE_PUSH_PULL, GPIO_OTYPE_OPEN_DRAIN                                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the GPIO Output Type (Push Pull or Open Drain)                    */
/*---------------------------------------------------------------------------------------------------------*/
GPIO_OTYPE_T GPIO_GetOutputType (UINT gpio_a_b)
{
	return 0;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_GetStatus                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                  stat    - pointer to a GPIO_STAT_T structure                                           */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine update the "stat" structure with the current GPIO status.                 */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_GetStatus (UINT gpio_a_b, GPIO_STAT_T* stat)
{
	GPIO_GET_VAL(gpio_a_b);

#if defined (GPIO_EVENT_SUPPORT)
	stat->interruptPolarity = READ_REG_FIELD(GPIOIEP, GPIOIEP_GPPL(gpio));
	stat->interruptIsEdge = READ_REG_FIELD(GPIOILG, GPIOILG_GPLG(gpio));
	stat->interruptBothEdges = READ_REG_FIELD(GPIOILG, GPIOILG_GPBE(gpio));
	if (READ_REG_FIELD(GPIOIEP, GPIOIEP_GPIE(gpio)) == 1)
	{
		stat->interruptEnabled = TRUE;
	}
#endif

	stat->dataIn     = (UINT)GPIO_Read(gpio);
	stat->dataOut    = (UINT)GPIO_ReadOutput(gpio);
	stat->dir        = GPIO_GetDir(gpio);
	stat->pull       = GPIO_GetPullType(gpio);
	stat->OutputType = GPIO_GetOutputType(gpio);
#if defined GPIO_CAPABILITY_VDD_DRIVEN
	stat->VddDrv     = (UINT)GPIO_GetVddDriven(gpio);
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_PrintRegs                                                                         */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_PrintRegs (void)
{
	HAL_PRINT("/*--------------*/\n");
	HAL_PRINT("/*     GPIO     */\n");
	HAL_PRINT("/*--------------*/\n\n");
	GPIO_PrintModuleRegs(0);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_PrintModuleRegs                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port - The port to be printed.                                                         */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module instance registers                                      */
/*lint -e{715}      Suppress 'port' not referenced                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_PrintModuleRegs (UINT port)
{
	HAL_PRINT("GPIO%1X:\n", port);
	HAL_PRINT("------\n");
	HAL_PRINT("GPIO%1DOD             = 0x%02X\n", port, REG_READ(GPIODOD));
	HAL_PRINT("GPIO%1DIN             = 0x%02X\n", port, REG_READ(GPIODIN));
	HAL_PRINT("GPIO%1PUD             = 0x%02X\n", port, REG_READ(GPIOPUD));
	HAL_PRINT("GPIO%1VDD             = 0x%02X\n", port, REG_READ(GPIOVDD));
	HAL_PRINT("GPIO%1EIP             = 0x%02X\n", port, REG_READ(GPIOIEP));
	HAL_PRINT("GPIO%1ILG             = 0x%02X\n", port, REG_READ(GPIOILG));
	HAL_PRINT("GPIO%1IST             = 0x%04X\n", port, REG_READ(GPIOIST));

	HAL_PRINT("\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_PrintVersion                                                                      */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_PrintVersion (void)
{
	HAL_PRINT("GPIO        = %X\n", MODULE_VERSION(GPIO_MODULE_TYPE));
}

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#if defined (GPIO_EVENT_SUPPORT) || (defined (MIWU_MODULE_TYPE) && defined (GPIO_MIWU_INTERRUPT_SUPPORT))
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_GetInterruptStatus                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to clear                                                       */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Returns the status of the GPIO Event                                                   */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN GPIO_GetInterruptStatus (UINT gpio_a_b)
{
	GPIO_GET_VAL(gpio_a_b);

	return (BOOLEAN)(READ_REG_FIELD(GPIOIST, GPIOIST_GPST(gpio)));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_GetEventType                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to clear                                                       */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Returns Event type allocated for this GPIO                                             */
/*---------------------------------------------------------------------------------------------------------*/
GPIO_EVENT_T GPIO_GetEventType(UINT gpio_a_b)
{
	UINT intType;
	UINT polarity;

	GPIO_GET_VAL(gpio_a_b);

	// Is it Level of Edge
	intType = READ_REG_FIELD(GPIOILG, GPIOILG_GPLG(gpio));
	polarity = READ_REG_FIELD(GPIOIEP, GPIOIEP_GPPL(gpio));
	if (READ_REG_FIELD(GPIOIEP, GPIOIEP_GPIE(gpio)) == 0)
	{
		return GPIO_INTR_NONE;
	}
	if (intType == 0)
	{
		// Level
		intType = GPIO_INTR_LOW;
		if (polarity == 0)
		{
			intType = GPIO_INTR_HIGH;
		}
	}
	else
	{
		if (READ_REG_FIELD(GPIOILG, GPIOILG_GPBE(gpio)) != 0)
		{
			return GPIO_INTR_ANY_EDGE;
		}
		if (READ_REG_FIELD(GPIOIEP, GPIOIEP_GPPL(gpio)))
		{
			return GPIO_INTR_FALLING;
		}
		else
		{
			return GPIO_INTR_RISING;
		}
	}

	return GPIO_INTR_NONE;
}
#endif