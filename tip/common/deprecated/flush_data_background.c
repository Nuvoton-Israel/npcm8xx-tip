/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "flush_data_background.h"
#include "state_manager/state_logging.h"
#include "platform_api.h"


/**
 * Runs the background persistence task.
 *
 * @param flush_data The persistence task to run.
 */
static void flush_data_background_task (struct flush_data_background *flush_data)
{
	int status;

	while (1) {

		/* Sleep timeout before updating system state and flushing debug logs. */
		platform_msleep (5000);

		xSemaphoreTake (flush_data->lock, portMAX_DELAY);

		if (flush_data->system_state) {
			status = state_manager_store_non_volatile_state (flush_data->system_state);
			if (status != 0) {
				debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_STATE_MGR,
					STATE_LOGGING_PERSIST_FAIL, 2, status);
			}
		}

		if (flush_data->logger) {
			flush_data->logger->flush (flush_data->logger);
		}

		xSemaphoreGive (flush_data->lock);
	}
}

/**
 * Initialize the background task to persist host and system state data.
 *
 * @param flush_data The background persistence task to initialize.
 * @param logger The log instance to flush.
 * @param system_state The manager for system state to persist.
 * @param priority The priority level for running the persistance task.
 *
 * @return 0 if the persistence task was initialized or an error code.
 */
int flush_data_background_init (struct flush_data_background *flush_data,
	const struct logging *logger, struct state_manager *system_state, int priority)
{
	int status;

	if (flush_data == NULL) {
			return LOGGING_INVALID_ARGUMENT;
	}

	memset (flush_data, 0, sizeof (struct flush_data_background));

	flush_data->system_state = system_state;
	flush_data->logger = logger;

	flush_data->lock = xSemaphoreCreateMutex ();
	if (flush_data->lock == NULL) {
		return LOGGING_NO_MEMORY;
	}

	status = xTaskCreate ((TaskFunction_t) flush_data_background_task, "Flush", 1 * 256, flush_data,
		priority, &flush_data->task);
	if (status != pdPASS) {
		vSemaphoreDelete (flush_data->lock);
		return LOGGING_NO_MEMORY;
	}

	return 0;
}

/**
 * Release resources for a persistence task instance.
 *
 * @param flush_data The persistence task to release.
 */
void flush_data_background_release (struct flush_data_background *flush_data)
{
	if (flush_data) {
		xSemaphoreTake (flush_data->lock, portMAX_DELAY);
		vTaskDelete (flush_data->task);
		vSemaphoreDelete (flush_data->lock);
	}
}
