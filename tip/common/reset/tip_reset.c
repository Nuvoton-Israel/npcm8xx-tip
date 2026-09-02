// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "logging/debug_log.h"
#include "system/system.h"
#include "platform_api.h"
#include "platform_io.h"
#include "tip_boot.h"
#include "tip_flash.h"
#include "tip_gpio.h"
#include "tip_reset.h"
#include "tip_rom_utils.h"
#include "tip_utils.h"


/**
 * The position bits 23-31 of destination register to copy TIP reset type indication
 */
#define RESSR_FIELDS_TO_COPY 23, 9

#ifdef ENABLE_RESET_COUNTER_CACHE
/**
 * Flag indicating if reset indication has been processed after TIP reset.
 */
static bool reset_processed;

/**
 * The last TIP reset.  This holds the value of TIP_SCR1 immediately after TIP/BMC goes through
 * a reset.
 */
static uint16_t last_reset;
#else
/**
 * Timestamp when reset counters were initialized after TIP reset.
 *
 */
static uint32_t last_reset_seconds_uptime;
#endif /* ENABLE_RESET_COUNTER*/

/**
 * @brief Reset TIP
 *
 * @param reset_type 0: SW Reset
 */
void platform_reset (uint32_t reset_type)
{
	platform_printf (KRED NEWLINE NEWLINE "====== RESET TIP %#010lx ======" NEWLINE KNRM, reset_type);

	/* Perform TIP Reset: TIP_ROM will scan again but it will try to load the next image on flash.*/
	REG_WRITE (SEC_ERR, 0xAA00); /* SEC_QUAL */
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_TIP_SW_RST, 1);

	/* Un-reachable line */
	while (1);
}

/**
 * Clear the reset indication after it was processed.
 */
void tip_clear_reset_indication (void)
{
	SET_REG_FIELD (TIP_RESET_INDICATION_SCR, TIP_RST_INDICATION_RESSR_23_31, 0);

	SET_REG_FIELD (INTCR2, RESSR_FIELDS_TO_COPY, READ_REG_FIELD (RESSR, RESSR_FIELDS_TO_COPY));

	/* clear RESSR too for the case there was a BMC reset but TIP was not reset. */
	REG_WRITE (RESSR, MASK_FIELD (RESSR_FIELDS_TO_COPY));
}

#ifndef ENABLE_RESET_COUNTER_CACHE
/**
 * Init reset counters.
 *
 * @param reset_counters_l Buffer to store reset counter count.
 *
 */
void tip_reset_counters_init (uint8_t reset_counters_l[RST_MAX])
{
	memset (reset_counters_l, 0xFF, RST_MAX);

	reset_counters_l[RST_WD0] = MAX_RESET_COUNT_WD0;
	reset_counters_l[RST_WD1] = MAX_RESET_COUNT_WD1;
	reset_counters_l[RST_WD2] = MAX_RESET_COUNT_WD2;
	reset_counters_l[RST_SW1] = MAX_RESET_COUNT_SW1;
	reset_counters_l[RST_SW2] = MAX_RESET_COUNT_SW2;
	reset_counters_l[RST_SW3] = MAX_RESET_COUNT_SW3;
	reset_counters_l[RST_CORST] = MAX_RESET_COUNT_CORST;
	reset_counters_l[RST_WOL] = MAX_RESET_COUNT_WOL;
	reset_counters_l[RST_FORCE_RESTART] = MAX_RESET_COUNT_FORCE_RESTART;

	last_reset_seconds_uptime = CLK_GetUpTimeSeconds ();
	return;
}

/**
 * Increment reset counters. note that multiple types of reset may happen at the same time.
 *
 * @param reset_counters_l Buffer to store reset counter count.
 * @param tip_scr1 TIP last reset type as indicated by TIP_SCR1.
 * @param system TIP system manager instance to utilize
 *
 */
void tip_reset_counters_increment (uint8_t reset_counters_l[RST_MAX], uint16_t tip_scr1,
	struct system *system)
{
	bool reset_maxed_out = false;
	uint32_t time_now = CLK_GetUpTimeSeconds ();

	/* debounce: if too much time passed since last reset (any reset!)
	 * restart the reset max counters
	 */
	if ((time_now - last_reset_seconds_uptime) > MAX_BMC_BOOT_TIME_IN_SECONDS) {
		last_reset_seconds_uptime = time_now;
		tip_reset_counters_init (reset_counters_l);
		return;
	}

	if (tip_scr1 & RESET_WD2) {
		if (reset_counters_l[RST_WD2] != 0) {
			reset_counters_l[RST_WD2]--;
			if (reset_counters_l[RST_WD2] == 0)
				reset_maxed_out = true;
		}
	}
	if (tip_scr1 & RESET_WD1) {
		if (reset_counters_l[RST_WD1] != 0) {
			reset_counters_l[RST_WD1]--;
			if (reset_counters_l[RST_WD1] == 0)
				reset_maxed_out = true;
		}
	}
	if (tip_scr1 & RESET_SW3) {
		if (reset_counters_l[RST_SW3] != 0) {
			reset_counters_l[RST_SW3]--;
			if (reset_counters_l[RST_SW3] == 0)
				reset_maxed_out = true;
		}
	}
	if (tip_scr1 & RESET_SW2) {
		if (reset_counters_l[RST_SW2] != 0) {
			reset_counters_l[RST_SW2]--;
			if (reset_counters_l[RST_SW2] == 0)
				reset_maxed_out = true;
		}
	}
	if (tip_scr1 & RESET_SW1) {
		if (reset_counters_l[RST_SW1] != 0) {
			reset_counters_l[RST_SW1]--;
			if (reset_counters_l[RST_SW1] == 0)
				reset_maxed_out = true;
		}
	}
	if (tip_scr1 & RESET_WD0) {
		if (reset_counters_l[RST_WD0] != 0) {
			reset_counters_l[RST_WD0]--;
			if (reset_counters_l[RST_WD0] == 0)
				reset_maxed_out = true;
		}
	}
	if (tip_scr1 & RESET_CORST) {
		if (reset_counters_l[RST_CORST] != 0) {
			reset_counters_l[RST_CORST]--;
			if (reset_counters_l[RST_CORST] == 0)
				reset_maxed_out = true;
		}
	}

#ifdef GPIO_WOL
	if (tip_scr1 & RESET_TIP_WOL) {
		if (reset_counters_l[RST_WOL] != 0) {
			reset_counters_l[RST_WOL]--;
			if (reset_counters_l[RST_WOL] == 0)
				reset_maxed_out = true;
		}
		tip_gpio_handler_complete (TIP_WOL_GPIO);
	}

	if (tip_scr1 & RESET_TIP_FORCE_RESTART) {
		if (reset_counters_l[RST_FORCE_RESTART] != 0) {
			reset_counters_l[RST_FORCE_RESTART]--;
			if (reset_counters_l[RST_FORCE_RESTART] == 0)
				reset_maxed_out = true;
		}
		tip_gpio_handler_complete (TIP_AUX_BMC_RESTART_GPIO);
	}
#endif

	/* one or more of the reset counters reached zero, reset and recovery */
	if (reset_maxed_out == true) {
		platform_printf (NEWLINE KRED "BMC RESET COUNT MAXED OUT. START RECOVERY" NEWLINE KNRM);
		tip_select_next_boot_image (tip_flash_get_recovery_phys_addr());
		tip_reset_platform_shutdown (system);
	}
	return;
}
#else
/**
 * Initialize reset counters with cached values in TIP_SCR.
 */
void tip_reset_counters_init_cache (uint8_t reset_counters_l[RST_MAX])
{
	uint8_t wd1_count = REG_READ (TIP_RESET_COUNTERS_SCR) & 0xFF;
	uint8_t wol_count = (REG_READ (TIP_RESET_COUNTERS_SCR) & 0x0F00) >> 8U;
	uint8_t force_restart_count = (REG_READ (TIP_RESET_COUNTERS_SCR) & 0xF000) >> 12U;

	memset (reset_counters_l, 0x00, RST_MAX);

	if ((last_reset & RESET_PORST) == 0) {
		reset_counters_l[RST_WD1] = wd1_count;
		reset_counters_l[RST_WOL] = wol_count;
		reset_counters_l[RST_FORCE_RESTART] = force_restart_count;
	}

	return;
}

/**
 * Increment reset counters. note that multiple types of reset may happen at the same time.
 *
 * @param reset_counters_l Buffer to store reset counter count.
 * @param tip_scr1 TIP last reset type as indicated by TIP_SCR1.
 * @param system TIP system manager instance to utilize
 *
 */
void tip_reset_counters_increment_cache (uint8_t reset_counters_l[RST_MAX], uint16_t tip_scr1,
	struct system *system)
{
	bool reset_maxed_out = false;
	uint16_t reg_cache;

	if ((MAX_RESET_COUNT_WD0 != 0) && (tip_scr1 & RESET_WD0)) {
		reset_counters_l[RST_WD0]++;
		if (reset_counters_l[RST_WD0] >= MAX_RESET_COUNT_WD0) {
			reset_maxed_out = true;
		}
	}

	if ((MAX_RESET_COUNT_WD1 != 0) && (tip_scr1 & RESET_WD1)) {
		reset_counters_l[RST_WD1]++;
		if (reset_counters_l[RST_WD1] < MAX_RESET_COUNT_WD1) {
			reg_cache = REG_READ (TIP_RESET_COUNTERS_SCR) & ~(0xFF);
			reg_cache |= reset_counters_l[RST_WD1];
			REG_WRITE (TIP_RESET_COUNTERS_SCR, reg_cache);
		}
		else {
			reset_maxed_out = true;
		}
	}

	if ((MAX_RESET_COUNT_WD2 != 0) && (tip_scr1 & RESET_WD2)) {
		reset_counters_l[RST_WD2]++;
		if (reset_counters_l[RST_WD2] >= MAX_RESET_COUNT_WD2) {
			reset_maxed_out = true;
		}
	}

	if ((MAX_RESET_COUNT_SW3 != 0) && (tip_scr1 & RESET_SW3)) {
		reset_counters_l[RST_SW3]++;
		if (reset_counters_l[RST_SW3] >= MAX_RESET_COUNT_SW3) {
			reset_maxed_out = true;
		}
	}

	if ((MAX_RESET_COUNT_SW2 != 0) && (tip_scr1 & RESET_SW2)) {
		reset_counters_l[RST_SW2]++;
		if (reset_counters_l[RST_SW2] >= MAX_RESET_COUNT_SW2) {
			reset_maxed_out = true;
		}
	}

	if ((MAX_RESET_COUNT_SW1 != 0) && (tip_scr1 & RESET_SW1)) {
		reset_counters_l[RST_SW1]++;
		if (reset_counters_l[RST_SW1] >= MAX_RESET_COUNT_SW1) {
			reset_maxed_out = true;
		}
	}

	if ((MAX_RESET_COUNT_CORST != 0) && (tip_scr1 & RESET_CORST)) {
		reset_counters_l[RST_CORST]++;
		if (reset_counters_l[RST_CORST] >= MAX_RESET_COUNT_CORST) {
			reset_maxed_out = true;
		}
	}

#ifdef GPIO_WOL
	if (tip_scr1 & RESET_TIP_WOL) {
		reset_counters_l[RST_WOL]++;
		if ((MAX_RESET_COUNT_WOL != 0) && (reset_counters_l[RST_WOL] < MAX_RESET_COUNT_WOL)) {
			reg_cache = REG_READ (TIP_RESET_COUNTERS_SCR) & ~(0xFF00);
			reg_cache |= ((reset_counters_l[RST_WOL] & 0x0F) << 8U);
			REG_WRITE (TIP_RESET_COUNTERS_SCR, reg_cache);
		}
		else {
			reset_maxed_out = true;
		}
		tip_gpio_handler_complete (TIP_WOL_GPIO);
	}

	if (tip_scr1 & RESET_TIP_FORCE_RESTART) {
		reset_counters_l[RST_FORCE_RESTART]++;
		if ((MAX_RESET_COUNT_FORCE_RESTART != 0) && (reset_counters_l[RST_FORCE_RESTART] < MAX_RESET_COUNT_FORCE_RESTART)) {
			reg_cache = REG_READ (TIP_SCR3) & ~(0xF000);
			reg_cache |= ((reset_counters_l[RST_FORCE_RESTART] & 0x0F) << 12U);
			REG_WRITE (TIP_SCR3, reg_cache);
		}
		else {
			reset_maxed_out = true;
		}
		tip_gpio_handler_complete (TIP_AUX_BMC_RESTART_GPIO);
	}
#endif /* GPIO_WOL */

	/* one or more of the reset counters reached zero, reset and recovery */
	if (reset_maxed_out == true) {
		platform_printf (NEWLINE KRED "BMC RESET COUNT MAXED OUT. START RECOVERY" NEWLINE KNRM);
		tip_select_next_boot_image (tip_flash_get_recovery_phys_addr());
		/* TODO: Add a debug log entry indicating the reason for recovery boot. */
		tip_reset_platform_shutdown (system);
	}
	return;
}

/**
 * clears cached reset counters values in TIP_SCR.
 *
 * @param reset_counters_l Buffer to store reset counter count.
 *
 */
void tip_reset_counters_clear_cache (uint8_t reset_counters_l[RST_MAX])
{
	/* Reset all counters. */
	memset (reset_counters_l, 0x00, RST_MAX);

	/* clear scratch pad register used to persist reset counters across TIP resets. */
	REG_WRITE (TIP_RESET_COUNTERS_SCR, 0);

	return;
}
#endif /* ENABLE_RESET_COUNTER_CACHE */

/**
 * Perform a graceful shutdown of the SoC.  If that fails, force a shutdown.
 *
 * @param system The system manager instance
 *
 */
void tip_reset_platform_shutdown (struct system *system)
{
	/* Reset SoC. */
	system_reset (system);

	/* If a graceful reset didn't work.  Just force a reset. */
	debug_log_flush ();
	platform_reset (0);
}

/**
 * Print the type of the last reset.
 */
uint16_t tip_get_reset_indication (void)
{
	uint16_t tip_scr1;

	platform_printf (KNRM "RESSR    =  %#010lx" NEWLINE, REG_READ (RESSR));
	platform_printf ("TIP_SCR1 =  %#010lx" NEWLINE, REG_READ (TIP_SCR1));
	platform_printf ("INTCR2   =  %#010lx" NEWLINE, REG_READ (INTCR2));

	tip_scr1 = REG_READ (TIP_RST_INDICATION);
	/* secondary reset, ressr info is on RESSR */
	if (READ_VAR_FIELD(tip_scr1, TIP_RST_INDICATION_RESSR_23_31) == 0) {
		SET_VAR_FIELD (tip_scr1, TIP_RST_INDICATION_RESSR_23_31, READ_REG_FIELD (RESSR, RESSR_FIELDS_TO_COPY));
	}

	if (tip_scr1 & RESET_WD2) {
		platform_printf (KBLU "Last reset was WD2" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_WD1) {
		platform_printf (KBLU "Last reset was WD1" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_TIP) {
		platform_printf (KBLU "Last reset was TIP reset" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_SW3) {
		platform_printf (KBLU "Last reset was SW3" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_SW2) {
		platform_printf (KBLU "Last reset was SW2" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_SW1) {
		platform_printf (KBLU "Last reset was SW1" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_WD0) {
		platform_printf (KBLU "Last reset was WD0" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_CORST) {
		platform_printf (KBLU "Last reset was CORST" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_PORST) {
		platform_printf (KBLU "Last reset was PORST" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_TIP_SW_SEC_ERR) {
		platform_printf (KBLU "Last reset was TIP SW ERR reset" NEWLINE KNRM);
	}
	if (tip_scr1 & RESET_TIP_WD) {
		platform_printf (KBLU "Last reset was TIP WD reset" NEWLINE KNRM);
	}

#ifdef GPIO_WOL
	if (tip_scr1 & RESET_TIP_WOL) {
		platform_printf (KBLU "Last reset was WOL" NEWLINE KNRM);
		SET_REG_FIELD (INTCR2, INTCR2_WOL, 1);
	}

	if (tip_scr1 & RESET_TIP_FORCE_RESTART) {
		platform_printf (KBLU "Last reset was Forced Restart" NEWLINE KNRM);
		SET_REG_FIELD (INTCR2, INTCR2_FORCE_RESTART, 1);
	}
#endif

#ifdef ENABLE_RESET_COUNTER_CACHE
	/* We cannot rely on TIP_SCR1 to get the last reset type since it gets updated by
	 * tip_update_reset_indication (), so cache the last reset when tip_so reset counters can be updated
	 * with correct values. */
	if (!reset_processed) {
		last_reset = tip_scr1;
		reset_processed = true;
	}
#endif /* ENABLE_RESET_COUNER_CACHE */

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

		/* All host related modules are not reset in case of warm boot */
		/* same for shared resources like flash */
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
		SET_VAR_FIELD (rcr1, WD0RCR_SPER, 1);
		SET_VAR_FIELD (rcr1, WD0RCR_PWM, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_SHM, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_PCIERC, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_ESPI, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_PCIMBX, 0);
		SET_VAR_FIELD (rcr1, WD0RCR_CP1, 0);

		SET_VAR_FIELD (rcr2, WD0RCRB_SEC_REG_RST_A1, 0);

		SET_VAR_FIELD (rcr2, WD0RCRB_HGPIO, 0);
		SET_VAR_FIELD (rcr2, WD0RCRB_FLM, 0);
		SET_VAR_FIELD (rcr2, WD0RCRB_PCIGFX, 0);
		SET_VAR_FIELD (rcr2, WD0RCRB_HOSTPER, 1);
		SET_VAR_FIELD (rcr2, WD0RCRB_BMCBUS, 0);

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

		/* Init TIP reset settings: same, except for FIU, CP1 and TIP security */
		SET_VAR_FIELD (rcr1, WD0RCR_SPIBMC, 1);
		SET_VAR_FIELD (rcr1, WD0RCR_CP1, 1);

		SET_VAR_FIELD (rcr2, WD0RCRB_SEC_REG_RST_A1, 1);

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
#ifdef GPIO_WOL
		intcr2 |= BUILD_FIELD_VAL (INTCR2_WOL,
			READ_VAR_FIELD (tip_scr1, TIP_RST_INDICATION_WOL));
		intcr2 |= BUILD_FIELD_VAL (INTCR2_FORCE_RESTART,
			READ_VAR_FIELD (tip_scr1, TIP_RST_INDICATION_FORCE_RESTART));
#endif
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
	SET_REG_FIELD (TIP_RESET_INDICATION_SCR, TIP_RST_INDICATION_RESSR_23_31, 0);
	SET_REG_FIELD (TIP_RESET_INDICATION_SCR, TIP_RST_INDICATION_WDRST_STS, 0);
	SET_REG_FIELD (TIP_RESET_INDICATION_SCR, TIP_RST_INDICATION_TIP_SWRST_STS, 0);
	SET_REG_FIELD (TIP_RESET_INDICATION_SCR, TIP_RST_INDICATION_DBGRST_STS, 0);
	SET_REG_FIELD (TIP_RESET_INDICATION_SCR, TIP_RST_INDICATION_SW_SEC_ERR, 0);
#ifdef GPIO_WOL
	SET_REG_FIELD (TIP_SCR1, TIP_RST_INDICATION_WOL, 0);
	SET_REG_FIELD (TIP_SCR1, TIP_RST_INDICATION_FORCE_RESTART, 0);
#endif
	

	/* clear RESSR too for the case there was a BMC reset but TIP was not reset. */
	REG_WRITE (RESSR, MASK_FIELD (RESSR_FIELDS_TO_COPY));

	platform_printf (KBLU "RESSR    =  %#010lx" NEWLINE, REG_READ (RESSR));
	platform_printf ("TIP_SCR1 =  %#010lx" NEWLINE, REG_READ (TIP_SCR1));
	platform_printf ("INTCR2   =  %#010lx" NEWLINE KNRM, REG_READ (INTCR2));
}
