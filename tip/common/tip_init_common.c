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
 * TWD interrupt handler for L0.
 * @param num Interrupt number
 */
void tip_twd_inthandler_l (uint16_t num)
{
	uint16_t int_num = SCS_GetActiveVectorNumber () - NVIC_TRAP_NUM;
	/* Clear TWD event */
	NVIC_ClearInt ((NVIC_INT_SRC_T) int_num);
	/* clear TC and reload timer */
	TWD_WatchDogTouched ();

	platform_printf ("TIP watchdog int %d" NEWLINE, int_num);
}

/**
 * Initialize TIP HW watchdog module

 * @param enable enable\disable the TWD.
 * @param preset WD counter value.
 * @param wdiv WD Prescaler value.
 *
 * @return void
 */
void tip_twd_common_init (bool enable, unsigned int preset, unsigned int wdiv)
{
	if (enable == false) {
		/* avoid too early event */
		if (TWD_WatchDogIsRunning () == true) {
			TWD_DisableTooEarlyTouch (true);
		}

		/* Stop and clear previous event if occured */
		TWD_WatchDogStop ();
		TWD_WatchDogResetClearEvent ();
	} else {
		/* Initialize and initiate WD based on given preset/wdev */
		TWD_Init ((TWD_CALLBACK) tip_twd_inthandler_l);
		TWD_TimerConfig (250);
		NVIC_EnableInt (NVIC_INT_1, false);
		NVIC_ClearInt (NVIC_INT_1);
		TWD_WatchDogConfig (preset, TIMER_OUT, COLD_RESET, false, wdiv);
		TWD_DisableTooEarlyTouch (true);
	}
}

/**
 * Run low-level initialization for TIP hardware
 */
void hardware_init (void)
{
	DISABLE_INTERRUPTS ();

	SET_REG_FIELD (TIPCFGR, TIPCFGR_APB_CLK_DIV, 4);
	SET_REG_FIELD (AHBCKFI, AHBCKFI_AHB_CLK_FRQ, 0xFA);
	serial_printf_init ();
	tip_twd_common_init (FALSE, WD_PRESET_L0, WD_WDIV_L0);
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
	tip_twd_common_init (TRUE, WD_PRESET_L0, WD_WDIV_L0);
	ENABLE_INTERRUPTS ();

	for (int i = 0; i < 8; i++) {
		BMC_FCFG_LOCK_MASK (i, 0, 0x00, 0x00);
	}
}

/**
 * TIP L1 syatem contorl flag parsing.
 *
 * @param tip_L1_sys_ctrl_l
 * @param val - value for mthe header.
 */
void tip_L1_system_control_init (struct tip_L1_system_control *tip_L1_sys_ctrl_l, uint32_t val)
{
	tip_L1_sys_ctrl_l->tip_system_control_flag = val;
		
	/* extract each bit */
	tip_L1_sys_ctrl_l->tip_recovery_force  = READ_VAR_BIT (val, 0);
	tip_L1_sys_ctrl_l->tip_start_wd = READ_VAR_BIT (val, 1);
	tip_L1_sys_ctrl_l->tip_support_encryption = READ_VAR_BIT (val, 2);
	tip_L1_sys_ctrl_l->tip_gpio_wol = READ_VAR_BIT (val, 3);

	REG_WRITE (TIP_L1_SYS_CTRL_FLAG_SCR, val);
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

	__asm volatile ("cpsie if");

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
	uint16_t int_num = SCS_GetActiveVectorNumber () - NVIC_TRAP_NUM;
	serial_printf_reconfig ();
	platform_printf (KRED "NVIC_IntHandlerCommon: interrupt num %#010lx" NEWLINE KNRM, int_num);
	NVIC_PrintRegs ();

	CLK_Delay_Sec (5);
	NVIC_ClearInt ((NVIC_INT_SRC_T) int_num);
}