// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#include <stdint.h>
#include <stdbool.h>
#include "tip_boot.h"
#include "platform_api.h"
#include "platform_io.h"
#include "tip_firmware_component.h"
#include "tip_flash.h"
#include "tip_rom_utils.h"

/**
 * The position bits 23-31 of destination register to copy TIP reset type indication
 */
#define RESSR_FIELDS_TO_COPY 23, 9

/**
 * Run low-level initialization for TIP hardware
 */
void hardware_init (void)
{
	DISABLE_INTERRUPTS ();

	SET_REG_FIELD (TIPCFGR, TIPCFGR_APB_CLK_DIV, 4);
	SET_REG_FIELD (AHBCKFI, AHBCKFI_AHB_CLK_FRQ, 0xFA);
	serial_printf_init ();
	TWD_WatchDogStop ();
	TWD_WatchDogResetClearEvent ();
	NVIC_Init (TRUE);
	NVIC_Reset ();
	SCS_Init ();
	SCS_FPEnableAccess (TRUE);
	MPU_Reset ();

	for (unsigned int i = 1; i < NVIC_TRAP_NUM; i++) {
		NVIC_EnableInt (i, FALSE);
		NVIC_InstallSwTrap (i, (SW_HANDLER_T) NVIC_TrapHandlerCommon);
	}

	SCS_ClearPendingSysTickInt ();

	for (unsigned int i = 0; i < NVIC_INTERRUPT_NUM; i++) {
		NVIC_EnableInt (i, FALSE);
		NVIC_InstallSwHandler (i, (SW_HANDLER_T) NVIC_IntHandlerCommon);
	}

	for (uint32_t i = 0; i < NVIC_INTERRUPT_NUM; i++) {
		NVIC_ClearInt (i);
	}

	/* clear BMC reset event: */
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_DBGRST_STS, 1);
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_BMC_CRST_EV, 1);

	ENABLE_INTERRUPTS ();

	for (int i = 0; i < 8; i++) {
		BMC_FCFG_LOCK_MASK (i, 0, 0x00, 0x00);
	}
}

/**
 * @brief Reset TIP
 *
 * @param reset_type 0: SW Reset
 */
void platform_reset (uint32_t reset_type)
{
	platform_printf (KRED NEWLINE NEWLINE "====== RESET TIP %#010lx ======" NEWLINE KNRM, reset_type);

	/* Reset everything.*/
	REG_WRITE (SWRSTC3, 0xFFFFFFFF);
	REG_WRITE (SWRSTC3B, 0xFFFFFFFF);

	/* Don't reset MC.*/
	SET_REG_FIELD (SWRSTC3, WD0RCR_MC, 0);

	/* Perform TIP Reset: TIP_ROM will scan again but it will try to load the next image on flash.*/
	REG_WRITE (SEC_ERR, 0xAA00); /* SEC_QUAL */
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_TIP_SW_RST, 1);

	/*TODO: add other resets */

	/* Un-reachable line */
	while (1);
}

/**
 * Clear the reset indication after it was processed.
 */
void tip_clear_reset_indication (void)
{
	SET_REG_FIELD (TIP_SCR1, TIP_RST_INDICATION_RESSR_23_31, 0);

	SET_REG_FIELD (INTCR2, RESSR_FIELDS_TO_COPY, READ_REG_FIELD (RESSR, RESSR_FIELDS_TO_COPY));

	/* clear RESSR too for the case there was a BMC reset but TIP was not reset. */
	REG_WRITE (RESSR, MASK_FIELD (RESSR_FIELDS_TO_COPY));
}

/**
 * Print the type of the last reset.
 */
uint16_t tip_get_reset_indication (void)
{
	uint16_t tip_scr1 = REG_READ (TIP_SCR1);

	platform_printf (KNRM "RESSR    =  %#010lx" NEWLINE, REG_READ (RESSR));
	platform_printf ("TIP_SCR1 =  %#010lx" NEWLINE, REG_READ (TIP_SCR1));
	platform_printf ("INTCR2   =  %#010lx" NEWLINE, REG_READ (INTCR2));

	/* secondary reset, info is on RESSR */
	if (READ_VAR_FIELD (tip_scr1, TIP_RST_INDICATION_RESSR_23_31) == 0) {
		tip_scr1 = READ_REG_FIELD (RESSR, RESSR_FIELDS_TO_COPY);
	}

	if (tip_scr1 & RESET_WD2)
		platform_printf (KBLU "Last reset was WD2" NEWLINE KNRM);
	if (tip_scr1 & RESET_WD1)
		platform_printf (KBLU "Last reset was WD1" NEWLINE KNRM);
	if (tip_scr1 & RESET_TIP)
		platform_printf (KBLU "Last reset was TIP reset" NEWLINE KNRM);
	if (tip_scr1 & RESET_SW3)
		platform_printf (KBLU "Last reset was SW3" NEWLINE KNRM);
	if (tip_scr1 & RESET_SW2)
		platform_printf (KBLU "Last reset was SW2" NEWLINE KNRM);
	if (tip_scr1 & RESET_SW1)
		platform_printf (KBLU "Last reset was SW1" NEWLINE KNRM);
	if (tip_scr1 & RESET_WD0)
		platform_printf (KBLU "Last reset was WD0" NEWLINE KNRM);
	if (tip_scr1 & RESET_CORST)
		platform_printf (KBLU "Last reset was CORST" NEWLINE KNRM);
	if (tip_scr1 & RESET_PORST)
		platform_printf (KBLU "Last reset was PORST" NEWLINE KNRM);

	return tip_scr1;
}

/**
 * Update TIP reset indication
 *
 * @param updateIntcr2 if true, update also INTCR2 so the BMC will have the access in case
 * we jump directly from TIP ROM to BMC.
 */
void tip_update_reset_indication (bool updateIntcr2)
{
	uint32_t rcr1;
	uint32_t rcr2;
	uint32_t intcr2 = REG_READ (INTCR2) & 0x007FFFFF;
	uint16_t tip_scr1 = REG_READ (TIP_SCR1);

	/* in PORST only: init RCR registers. PORST and CFGDONE are set */
	if (tip_scr1 & RESET_PORST) {
		/* configure warm reset handling */
		rcr1 = 0xFFFFFFFF;
		rcr2 = 0xFFFFFFFF;

		SET_VAR_FIELD (rcr1, WD0RCR_BMCDBG, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_MC, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_CLKS, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_TIP_Reset, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_GPIOM0, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_GPIOM1, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_GPIOM2, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_GPIOM3, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_GPIOM4, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_GPIOM5, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_GPIOM6, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_GPIOM7, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_SPIBMC, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_SPER, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_PWM, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_SHM, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_PCIERC, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_ESPI, 0);

		if (CHIP_Get_Version () >= 0x04)
			SET_VAR_FIELD (rcr2, WD0RCRB_SEC_REG_RST_A1, 0);
		else
			/* for Z1 only. In A1 this bit is locked by ROM, except for TIP RESET (TIPRSTCB) */
			SET_VAR_FIELD (rcr2, WD0RCRB_CP2_TIP_Z1, 1);

		SET_VAR_FIELD (rcr2, WD0RCRB_HGPIO, 0);
		SET_VAR_FIELD (rcr2, WD0RCRB_FLM, 0);
		SET_VAR_FIELD (rcr2, WD0RCRB_PCIGFX, 0);
		SET_VAR_FIELD (rcr2, WD0RCRB_HOSTPER, 1);

		REG_WRITE (WD0RCR, rcr1);
		REG_WRITE (WD1RCR, rcr1);
		REG_WRITE (WD2RCR, rcr1);

		REG_WRITE (WD0RCRB, rcr2);
		REG_WRITE (WD1RCRB, rcr2);
		REG_WRITE (WD2RCRB, rcr2);


		REG_WRITE (SWRSTC1, rcr1);
		REG_WRITE (SWRSTC2, rcr1);
		REG_WRITE (SWRSTC3, rcr1);

		REG_WRITE (SWRSTC1B, rcr2);
		REG_WRITE (SWRSTC2B, rcr2);
		REG_WRITE (SWRSTC3B, rcr2);


		REG_WRITE (CORSTC, rcr1);
		REG_WRITE (CORSTCB, rcr2);

		platform_printf ("Init reset control regs: WD0RCR = %#010lx WD0RCRB = %#010lx" NEWLINE,
			rcr1, rcr2);

		SET_VAR_FIELD (rcr1, WD0RCR_SPIBMC, 1);

		if (CHIP_Get_Version () >= 0x04) {
			SET_VAR_FIELD (rcr2, WD0RCRB_SEC_REG_RST_A1, 1);
		} else {
			SET_VAR_FIELD (rcr2, WD0RCRB_CP2_TIP_Z1, 1);
		}

		REG_WRITE (TIPRSTC, rcr1);
		REG_WRITE (TIPRSTCB, rcr2);

		platform_printf ("Init reset control regs: TIPRSTC = %#010lx TIPRSTCB = %#010lx" NEWLINE,
			REG_READ (TIPRSTC), REG_READ (TIPRSTCB));
	} else {
		platform_printf ("Skip RCR init" NEWLINE);
	}

	/* in PORST only: configure INTCR2 */
	if (tip_scr1 & RESET_PORST) {
		intcr2 |= BUILD_FIELD_VAL (RESSR_FIELDS_TO_COPY,
			READ_VAR_FIELD (tip_scr1, TIP_RST_INDICATION_RESSR_23_31));
		intcr2 |= BUILD_FIELD_VAL (INTCR2_TIP_WDRST_STS,
			READ_VAR_FIELD (tip_scr1, TIP_RST_INDICATION_WDRST_STS));
		intcr2 |= BUILD_FIELD_VAL (INTCR2_TIP_SWRST_STS,
			READ_VAR_FIELD (tip_scr1, TIP_RST_INDICATION_TIP_SWRST_STS));
		intcr2 |= BUILD_FIELD_VAL (INTCR2_DBGRST_STS,
			READ_VAR_FIELD (tip_scr1, TIP_RST_INDICATION_DBGRST_STS));
		intcr2 |= BUILD_FIELD_VAL (INTCR2_SW_SEC_ERR,
			READ_VAR_FIELD (tip_scr1, TIP_RST_INDICATION_SW_SEC_ERR));
	} else {
		uint32_t scrReg = REG_READ (TIP_RST_INDICATION);
		scrReg |= BUILD_FIELD_VAL (TIP_RST_INDICATION_RESSR_23_31,
			READ_REG_FIELD (RESSR, RESSR_FIELDS_TO_COPY));
		intcr2 |= BUILD_FIELD_VAL (RESSR_FIELDS_TO_COPY,
			READ_VAR_FIELD (scrReg, TIP_RST_INDICATION_RESSR_23_31));
		/* Then clear the bits in RESSR register. */
		REG_WRITE (RESSR, MASK_FIELD (RESSR_FIELDS_TO_COPY));
	}

	REG_WRITE (INTCR2, intcr2);

	/* clear TIP_SCR1 for next round */
	SET_REG_FIELD (TIP_SCR1, TIP_RST_INDICATION_RESSR_23_31, 0);

	/* clear RESSR too for the case there was a BMC reset but TIP was not reset. */
	REG_WRITE (RESSR, MASK_FIELD (RESSR_FIELDS_TO_COPY));

	platform_printf (KBLU "RESSR    =  %#010lx" NEWLINE, REG_READ (RESSR));
	platform_printf ("TIP_SCR1 =  %#010lx" NEWLINE, REG_READ (TIP_SCR1));
	platform_printf ("INTCR2   =  %#010lx" NEWLINE KNRM, REG_READ (INTCR2));
}

/**
 * NVIC Trap handler with debug logging information
 *
 * @param num Interrupt number
 */
void NVIC_TrapHandlerCommon (uint16_t num)
{
	register uint32_t lr_reg, ipsr_reg, msp_reg, psp_reg, cntl_reg, epsr_reg;

	__asm volatile ("cpsid if");
	__asm volatile ("MOV %0, LR" NEWLINE : "=r"(lr_reg));
	__asm volatile ("MRS %0, IPSR" NEWLINE : "=r"(ipsr_reg));
	__asm volatile ("MRS %0, MSP" NEWLINE : "=r"(msp_reg));
	__asm volatile ("MRS %0, PSP" NEWLINE : "=r"(psp_reg));
	__asm volatile ("MRS %0, CONTROL" NEWLINE : "=r"(cntl_reg));
	__asm volatile ("MRS %0, EPSR" NEWLINE : "=r"(epsr_reg));

	serial_printf_reconfig ();

	platform_printf (KRED NEWLINE "========" NEWLINE "TIP_FW TRAP!" NEWLINE "========" NEWLINE
								  "NVIC_TrapHandlerCommon: error %#010lx" NEWLINE KNRM, num);

	platform_printf ("IPSR = %#010lx " NEWLINE, ipsr_reg);
	platform_printf ("EPSR = %#010lx " NEWLINE, epsr_reg);
	platform_printf ("LR   = %#010lx " NEWLINE, lr_reg);
	platform_printf ("PSP   = %#010lx " NEWLINE, psp_reg);
	platform_printf ("MSP   = %#010lx " NEWLINE, msp_reg);
	platform_printf ("CONTROL   = %#010lx " NEWLINE, cntl_reg);

	switch (ipsr_reg & 0x1F) {
		case 0:
			platform_printf ("NoException" NEWLINE);
			break;
		case 1:
			platform_printf ("Reset" NEWLINE);
			break;
		case 2:
			platform_printf ("NMI" NEWLINE);
			break;
		case 3:
			platform_printf ("HardFault" NEWLINE);
			break;
		case 4:
			platform_printf ("MemManage" NEWLINE);
			break;
		case 5:
			platform_printf ("BusFault" NEWLINE);
			break;
		case 6:
			platform_printf ("UsageFault" NEWLINE);
			break;
		case 7:
			platform_printf ("Reserved" NEWLINE);
			break;
		case 8:
			platform_printf ("Reserved" NEWLINE);
			break;
		case 9:
			platform_printf ("Reserved" NEWLINE);
			break;
		case 10:
			platform_printf ("Reserved" NEWLINE);
			break;
		case 11:
			platform_printf ("SVCall" NEWLINE);
			break;
		case 12:
			platform_printf ("DebugMonitor" NEWLINE);
			break;
		case 13:
			platform_printf ("Reserved" NEWLINE);
			break;
		case 14:
			platform_printf ("PendSV" NEWLINE);
			break;
		case 15:
			platform_printf ("SysTick" NEWLINE);
			break;
		default:
			platform_printf ("INTISR%d ", (ipsr_reg & 0x1F) - 16);
			break;
	}

	SCS_PrintRegs ();
	NVIC_PrintRegs ();

	/* Since it's a trap, we can clear everything and prepare for restarting. */
	REG_WRITE (NVIC_ICPR (0), 0xFFFFFFFF);
	REG_WRITE (NVIC_ICPR (1), 0xFFFFFFFF);
	REG_WRITE (NVIC_ICPR (2), 0xFFFFFFFF);

	/* Clear all traps and restart SCS */
	SCS_Init ();

	/* Give the user time to read the terminal. */
	CLK_Delay_MicroSec (1000000);

	/* For debugging */
	while (1);
}

/**
 * NVIC common interrupt handler.
 *
 * @param num Interrupt number
 */
_ISR_ void NVIC_IntHandlerCommon (uint16_t num)
{
	uint16_t int_num = SCS_GetActiveVectorNumber ();
	serial_printf_reconfig ();
	platform_printf (KRED "NVIC_IntHandlerCommon: interrupt num %#010lx" NEWLINE KNRM, int_num);
	NVIC_PrintRegs ();

	CLK_Delay_Sec (5);
	NVIC_ClearInt ((NVIC_INT_SRC_T) int_num);
}