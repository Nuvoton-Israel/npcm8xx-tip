/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stddef.h>
#include "platform_io.h"
#include "twd_task.h"
#include "hal_regs.h"

static int tip_wd_delay;

/**
 * WD task: restart WD
 *
 * @param delay Pointer to an integer containing the delay
 *
 */
static void tip_task_wd_handler (void *delay)
{
	int wd_delay = *(unsigned int *) delay;
	TickType_t xLastWakeTime;
	wd_delay = pdMS_TO_TICKS (wd_delay * 1000);

	/* TIP init xLastWakeTime once */
	xLastWakeTime = xTaskGetTickCount ();
	for (;;) {
		vTaskDelayUntil (&xLastWakeTime, wd_delay);

		/* pet the WD, once in every "delay" seconds */
		TWD_WatchDogRestart ();
	}
}

/**
 * Start running the wd update task.  which starts the periodic update
 *
 * @param task The update task to start.
 * @param stack_words The size of the update task stack.  The stack size is measured in words.
 *
 * @return 0 if the task was started or an error code.
 */
int tip_watchdog_service_start (TaskHandle_t *task, int delay, uint16_t stack_words)
{
	int status;

	tip_wd_delay = delay;

	if (task == NULL) {
		return TIP_WD_UPDATE_INVALID_ARGUMENT;
	}

	if ((delay < MIN_WD_DELAY) || (delay > MAX_WD_DELAY)) {
		return TIP_WD_UPDATE_INVALID_ARGUMENT;
	}
	status = xTaskCreate ((TaskFunction_t) tip_task_wd_handler, "TIP_WD", stack_words,
		(void *) &tip_wd_delay, CERBERUS_PRIORITY_NORMAL, task);
	if (status != pdPASS) {
		return TIP_WD_UPDATE_ABORTED;
	}

	return TIP_WD_UPDATE_INVALID_SUCCESS;
}
