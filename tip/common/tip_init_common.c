// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
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

/*
 * TIP L0 header handler for flag SystemControlFlags (control in IGPS, TipFwAndHeader_L1.xml)
 */
struct tip_L0_system_control tip_L0_sys_ctrl;
struct log_buffer_t tip_log_buffer;

void tip_L0_system_control_init (struct tip_L0_system_control *tip_L0_sys_ctrl_l, uint32_t val)
{
	/* extract each bit */
	tip_L0_sys_ctrl_l->tip_disable_print_to_uart = READ_VAR_BIT (val, 0);
	tip_L0_sys_ctrl_l->tip_print_to_memory = READ_VAR_BIT (val, 1);
}

/*  check the log_start_address  :
 *	1. if it's < _16MB_ don't allow at all
 *	2. if it's RAM2 or RAM3 it's allowed
 *	4. if it's < DDR, it's OK but the memory_puts will have to check
 *	   that the DDR has been read and done
 */
bool tip_check_mem_log_valid_location (uint32_t start_addr, uint32_t size)
{
	/* Reject addresses below 16MB TIP internal peripherals and check for non-zero size */
	if (start_addr < _16MB_ || size == 0) {
		return false;
	}

	/*  Allow DDR region (addresses below 0x80000000) */
	if (start_addr + size <= (SDRAM_BASE_ADDR + SDRAM_MAPPED_SIZE)) {
		return true;
	}

	/*  RAM2 range check */
	uint32_t ram2_start = RAM2_BASE_ADDR + 0x30000; /* assume that first 0x30000 is used by L0+L1 */
	uint32_t ram2_end = RAM2_BASE_ADDR + 0x40000;
	/*  start within range, and prevent overflow */
	if (start_addr >= ram2_start && start_addr <= ram2_end - size) {
		return true;
	}

	/* RAM3 range check */
	uint32_t ram3_start = RAM3_BASE_ADDR;
	uint32_t ram3_end = RAM3_BASE_ADDR + RAM3_MEMORY_SIZE;
	/* start within range, and prevent overflow */
	if (start_addr >= ram3_start && start_addr <= ram3_end - size) {
		return true;
	}

	/* All other cases are invalid */
	return false;
}

/**
 * Run low-level initialization for TIP hardware
 */
void hardware_init (void)
{
	bool print_memory_valid_addr = true;
	DISABLE_INTERRUPTS ();

	SET_REG_FIELD (TIPCFGR, TIPCFGR_APB_CLK_DIV, 4);
	SET_REG_FIELD (AHBCKFI, AHBCKFI_AHB_CLK_FRQ, 0xFA);

	/* init the tip_L0_sys_ctrl */
	TIP_HEADER_STRUCT_T *head = (TIP_HEADER_STRUCT_T *) 0x20000;
	tip_L0_system_control_init (&tip_L0_sys_ctrl, head->SystemControlFlags);
	if (tip_L0_sys_ctrl.tip_print_to_memory) {
		uint16_t tip_scr1 = REG_READ (TIP_RST_INDICATION);
		if (tip_scr1 & RESET_PORST) {
			if (tip_check_mem_log_valid_location (head->log_start_address, head->log_size) == true) {
				memory_printf_init (head->log_start_address, head->log_size,
					head->log_start_address);
				REG_WRITE (SCRPAD_10_41 (17), head->log_start_address);
				REG_WRITE (SCRPAD_10_41 (18), head->log_size);
				/* on the init function , the next address will be the start */
				REG_WRITE (SCRPAD_10_41 (19), head->log_start_address);
			} else {										 // not valid
				tip_L0_sys_ctrl.tip_print_to_memory = false; /* dismiss the option to write to
																memory */
				print_memory_valid_addr = false;
				/* no need to update the address\size in the scratch pad since they will remain 0 */
			}
		} else /* not PORST , no need to re check the validness of the data */ {
			uint32_t start_addr = REG_READ (SCRPAD_10_41 (17));
			uint32_t size = REG_READ (SCRPAD_10_41 (18));
			uint32_t next_addr = REG_READ (SCRPAD_10_41 (19));
			/* pull the information from the scratch pad and save it in the global var */
			memory_printf_udpate (start_addr, size, next_addr);
		}
	}
	serial_printf_init (!tip_L0_sys_ctrl.tip_disable_print_to_uart,
		tip_L0_sys_ctrl.tip_print_to_memory);
	if (!print_memory_valid_addr) {
		serial_printf (KRED "cannot print to memory 0x%08X, size 0x%08X " KNRM,
			head->log_start_address, head->log_size);
	}

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
	/* extract each bit */
	tip_L1_sys_ctrl_l->tip_recovery_force = READ_VAR_BIT (val, 0);
	tip_L1_sys_ctrl_l->tip_start_wd = READ_VAR_BIT (val, 1);
	tip_L1_sys_ctrl_l->tip_support_encryption = READ_VAR_BIT (val, 2);
	tip_L1_sys_ctrl_l->tip_gpio_wol = READ_VAR_BIT (val, 3);
	tip_L1_sys_ctrl_l->tip_disable_print_to_uart = READ_VAR_BIT (val, 5);
	tip_L1_sys_ctrl_l->tip_print_to_memory = READ_VAR_BIT (val, 6);

	REG_WRITE (TIP_SCR7, val);
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
								  "NVIC_TrapHandlerCommon: error %#010lx" NEWLINE KNRM,num);

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