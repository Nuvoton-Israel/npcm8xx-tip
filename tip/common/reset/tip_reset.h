// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_RESET_H_
#define TIP_RESET_H_

#include "system/system.h"


/****************************
 * Reset counters: set to zero to disable counters.
 ****************************/

/**
 * RESET type, bitwise. bit numbers are selected according to TIP_SCR1.
 * can be multiple types of resets at the same time.
 */
typedef enum
{
	RST_WD0 = 0,
	RST_WD1 = 1,
	RST_WD2 = 2,
	RST_TIP = 3,
	RST_SW1 = 4,
	RST_SW2 = 5,
	RST_SW3 = 6,
	RST_CORST = 7,
	RST_PORST = 8,
	RST_WOL = 9,
	RST_MAX = 10
}  RESET_T;

/**
 * Max reset counters, one value per reset type.
 * When the number of resets exceeds TIP will do TIP reset
 * and go to recovery flow.
 * If a value is zero feature is disabled for this type of reset.
 */

/**
 * Max WD0 reset counter. After exceeding go to recovery.
 */
#define MAX_RESET_COUNT_WD0 							0

/**
 * Max WD1 reset counter. After exceeding go to recovery.
 */
#define MAX_RESET_COUNT_WD1 							2

/**
 * Max WD2 reset counter. After exceeding go to recovery.
 */
#define MAX_RESET_COUNT_WD2 							0

/**
 * Max SW1 reset counter. After exceeding go to recovery.
 */
#define MAX_RESET_COUNT_SW1 							0

/**
 * Max SW2 reset counter. After exceeding go to recovery.
 */
#define MAX_RESET_COUNT_SW2 							0

/**
 * Max SW3 reset counter. After exceeding go to recovery.
 */
#define MAX_RESET_COUNT_SW3 							0

/**
 * Max CORST reset counter. After exceeding go to recovery.
 */
#define MAX_RESET_COUNT_CORST 							0

/**
 * Max WOL reset counter. After exceeding go to recovery.
 */
#define MAX_RESET_COUNT_WOL 							2

/**
 * Max time for BMC to boot and get to Linux. If multiple resets happen during
 * this time frame - go to recovery.
 */
#define MAX_BMC_BOOT_TIME_IN_SECONDS                 600

void platform_reset (uint32_t reset_type);
void tip_reset_platform_shutdown (struct system *system);

void tip_clear_reset_indication (void);
uint16_t tip_get_reset_indication (void);
void tip_update_reset_indication (bool updateIntcr2);
void tip_reset_counters_init (uint8_t reset_counters_l[RST_MAX]);
void tip_reset_counters_increment (uint8_t reset_counters_l[RST_MAX], uint16_t tip_scr1,
	struct system *system);

#ifdef ENABLE_RESET_COUNTER_CACHE
void tip_reset_counters_init_cache (uint8_t reset_counters_l[RST_MAX]);
void tip_reset_counters_increment_cache (uint8_t reset_counters_l[RST_MAX], uint16_t tip_scr1,
	struct system *system);
void tip_reset_counters_clear_cache (uint8_t reset_counters_l[RST_MAX]);
#endif /* ENABLE_RESET_COUNTER_CACHE */

#endif /* TIP_RESET_H_ */
