// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "tip_fw_update_task.h"
#include "firmware/firmware_logging.h"
#include "tip_image_combo.h"
#include "tip_version.h"

#define RUN_UPDATE_BIT 			(1 << 0)
#define PREP_STAGING_BIT 		(1 << 1)
#define WRITE_TO_STAGING_BIT	(1 << 2)

extern struct tip_version_handler *tip_version;

/**
 * TIP L1 header handler for flag SystemControlFlags (control in IGPS, TipFwAndHeader_L1.xml)
 */
extern struct tip_L1_system_control tip_L1_sys_ctrl;

/**
 * Identify TIP FW updater
 *
 * @param task TIP FW update task
 * @return FW updater that will be used.
 */
static struct firmware_update *identify_tip_fw_updater (struct tip_fw_update_task *task)
{
	int status = 0;

	if (task->updater[TIP_FW_UPDATER_COMBO_0_2] == NULL ||
		task->updater[TIP_FW_UPDATER_COMBO_1] == NULL) {
		return NULL;
	}

	struct firmware_update *tmp_updater = task->updater[TIP_FW_UPDATER_COMBO_0_2];
	struct tip_image_combo *fw_combo = (struct tip_image_combo *) tmp_updater->fw;

	status = tmp_updater->fw->load (tmp_updater->fw, tmp_updater->flash->staging_flash,
		tmp_updater->flash->staging_addr + tmp_updater->state->img_offset);
	if (status != 0) {
		return NULL;
	}

	/* Combo 0 and 2 share the same updater. Combo 1 has a separate updater. */
	return (fw_combo->img_type == IMG_COMBO1 ? task->updater[TIP_FW_UPDATER_COMBO_1] :
		task->updater[TIP_FW_UPDATER_COMBO_0_2]);
}

/**
 * Erase the entire staging area and prepare for incoming FW update file.
 *
 * @param updater Updater to use
 * @param size FW update file size to clear in staging area
 * @param callback A set of notification handlers to use during the update process.  This can be
 * null if no notifications are necessary.  Also, individual callbacks that are not desired can be
 * left null.
 *
 * @return Preparation status, 0 if success or an error code.
 */
static int tip_firmware_update_prepare_staging_erase_all (const struct firmware_update *updater,
	const struct firmware_update_notification *callback, size_t size)
{
	int status;

	if (updater == NULL) {
		if ((callback != NULL) && (callback->status_change != NULL)) {
			callback->status_change (callback, UPDATE_STATUS_STAGING_PREP_FAIL);
		}
		return FIRMWARE_UPDATE_INVALID_ARGUMENT;
	}

	if ((callback != NULL) && (callback->status_change != NULL)) {
		callback->status_change (callback, UPDATE_STATUS_STAGING_PREP);
	}

	status = flash_updater_prepare_for_update_erase_all (&updater->state->update_mgr, size);
	if (status != 0) {
		if ((callback != NULL) && (callback->status_change != NULL)) {
			callback->status_change (callback, UPDATE_STATUS_STAGING_PREP_FAIL);
		}
	}

	return status;
}

/**
 * The task function that will run the firmware update.
 *
 * @param task The updater task instance.
 */
static void tip_fw_update_task_updater (struct tip_fw_update_task *task)
{
	struct firmware_update *default_updater = task->updater[TIP_FW_UPDATER_COMBO_0_2];
	uint32_t notification;
	bool reset = false;
	int status;

	platform_printf (KMAG NEWLINE "%s: start" NEWLINE, __func__);

	if (task->running == 2) {
		platform_printf (KMAG "%s : restore active image\n" KNRM, __func__);
		/* The system is running from the recovery image, so mark that image as good and restore the
		 * active image to a functional state. */
		debug_log_create_entry (DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_CERBERUS_FW,
			FIRMWARE_LOGGING_ACTIVE_RESTORE_START, 0, 0);

		firmware_update_set_recovery_good (default_updater, true);
		status = firmware_update_restore_active_image (default_updater);

		debug_log_create_entry ((status == 0) ? DEBUG_LOG_SEVERITY_INFO : DEBUG_LOG_SEVERITY_ERROR, 
			DEBUG_LOG_COMPONENT_CERBERUS_FW, FIRMWARE_LOGGING_ACTIVE_RESTORE_DONE, status, 0);

		if (status) {
			platform_printf (KRED "ERROR: fail to restore main flash status %#010lx" NEWLINE KNRM,
							 status);
			TWD_WatchDogRestart ();
			CLK_Delay_Sec (5);
		}
	} else {
#ifdef FORCE_RECOVERY_IMAGE_MATCH_ACTIVE
		if (tip_L1_sys_ctrl.tip_recovery_force) {
			platform_printf(KMAG "\nupdate and recovery flow\n" KNRM);
			status = firmware_update_recovery_matches_active_image (default_updater);
			if (status != 0) {
				firmware_update_set_recovery_good (default_updater, false);
				platform_printf (KMAG
				"Recovery image doesn't match active image, status=%#010lx. Restore recovery image"
				NEWLINE KNRM, status);
			}
			debug_log_create_entry ((status == 0) ? DEBUG_LOG_SEVERITY_INFO : DEBUG_LOG_SEVERITY_WARNING,
			DEBUG_LOG_COMPONENT_CERBERUS_FW, FIRMWARE_LOGGING_RECOVERY_IMAGE, (status != 0),
			status);
		}
		else {
			firmware_update_set_recovery_good (default_updater, true);
		}
#endif
		/* Ensure the recovery image is in a good state. */
		if (firmware_update_is_recovery_good (default_updater)) {
			firmware_update_validate_recovery_image (default_updater);
		}

		/* Default updater can parse Combo 2 on flash and then restore if needed. */
		status = firmware_update_restore_recovery_image (default_updater);
		if (status == 0) {
			debug_log_create_entry (DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_CERBERUS_FW,
				FIRMWARE_LOGGING_RECOVERY_IMAGE, 0, 0);
#ifdef TIP_DEBUG_BUILD
			platform_printf (KGRN "%s : restore recovery image pass" NEWLINE KNRM, __func__);
#endif
		}
		else if (status != FIRMWARE_UPDATE_RESTORE_NOT_NEEDED) {
			debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_CERBERUS_FW,
				FIRMWARE_LOGGING_RECOVERY_RESTORE_FAIL, status, 0);
			tip_version_init (tip_version);
#ifdef TIP_DEBUG_BUILD
			platform_printf (KRED "%s : restore recovery image fail status=%#010lx" NEWLINE KNRM, __func__, status);
#endif
		}
	}
	

	xSemaphoreTake (task->lock, portMAX_DELAY);
	task->running = 0;
	xSemaphoreGive (task->lock);

	do {
		/* Wait for a signal to perform update action. */
		status = 1;
		xTaskNotifyWait (pdFALSE, ULONG_MAX, &notification, portMAX_DELAY);

		struct firmware_update *combo_updater = task->updater[TIP_FW_UPDATER_COMBO_0_2];

		if (notification & RUN_UPDATE_BIT) {
			debug_log_create_entry (DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_CERBERUS_FW,
				FIRMWARE_LOGGING_UPDATE_START, 0, 0);
			debug_log_flush ();

			/* Identify the fw updater before doing actual update. */
			combo_updater = identify_tip_fw_updater (task);

			/* Use the new API since key revocation workflow is not supported for now. */
			status = firmware_update_run_update_no_revocation (combo_updater, &task->notify.base);
			if (status != 0) {
				debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_CERBERUS_FW,
					FIRMWARE_LOGGING_UPDATE_FAIL, task->update_status, status);
			}
#ifndef SWD_DEBUG
			else {
				debug_log_create_entry (DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_CERBERUS_FW,
					FIRMWARE_LOGGING_UPDATE_COMPLETE, 0, 0);

				reset = true;
			}
#endif
		} else if (notification & PREP_STAGING_BIT) {
			status = tip_firmware_update_prepare_staging_erase_all (combo_updater, &task->notify.base,
				task->staging_size);
			if (status != 0) {
				debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_CERBERUS_FW,
					FIRMWARE_LOGGING_ERASE_FAIL, task->update_status, status);
			}
		} else if (notification & WRITE_TO_STAGING_BIT) {
			status = firmware_update_write_to_staging (combo_updater, &task->notify.base,
				task->staging_buf, task->staging_buf_len);
			if (status != 0) {
				debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_CERBERUS_FW,
					FIRMWARE_LOGGING_WRITE_FAIL, task->update_status, status);
			}

		}

		xSemaphoreTake (task->lock, portMAX_DELAY);
		if (status != 1) {
			if (status == 0) {
				task->update_status = 0;
			} else {
				task->update_status |= (status << 8);
			}
		}
		task->running = (reset) ? 1 : 0;
		xSemaphoreGive (task->lock);

		if (reset) {
			/* After a successful FW update, reset the system.  We need to wait a bit before
			 * triggering the reset to give time for the application that started the update to
			 * know that it was successful. */
			platform_msleep (5000);
			system_reset (task->system);
			reset = false; /* We should never get here, but clear the flag if the reset fails. */
		}
	} while (1);
}

static int tip_fw_update_task_start_update (const struct firmware_update_control *update)
{
	struct tip_fw_update_task *task = (struct tip_fw_update_task*) update;
	int status = 0;

	if (task == NULL) {
		return FIRMWARE_UPDATE_INVALID_ARGUMENT;
	}

	if (task->task) {
		xSemaphoreTake (task->lock, portMAX_DELAY);
		if (!task->running) {
			task->update_status = UPDATE_STATUS_STARTING;
			task->running = 1;
			xSemaphoreGive (task->lock);
			xTaskNotify (task->task, RUN_UPDATE_BIT, eSetBits);
		} else {
			task->update_status = UPDATE_STATUS_REQUEST_BLOCKED;
			status = FIRMWARE_UPDATE_TASK_BUSY;
			xSemaphoreGive (task->lock);
		}
	} else {
		task->update_status = UPDATE_STATUS_TASK_NOT_RUNNING;
		status = FIRMWARE_UPDATE_NO_TASK;
	}

	return status;
}

static int tip_fw_update_task_get_status (const struct firmware_update_control *update)
{
	struct tip_fw_update_task *task = (struct tip_fw_update_task*) update;
	int status;

	if (task == NULL) {
		return UPDATE_STATUS_UNKNOWN;
	}

	xSemaphoreTake (task->lock, portMAX_DELAY);
	status = task->update_status;
	xSemaphoreGive (task->lock);

	return status;
}

static int32_t tip_fw_update_task_get_remaining_len (const struct firmware_update_control *update)
{
	struct tip_fw_update_task *task = (struct tip_fw_update_task*) update;
	int32_t bytes;

	if (task == NULL) {
		return 0;
	}

	xSemaphoreTake (task->lock, portMAX_DELAY);
	bytes = firmware_update_get_update_remaining (task->updater[TIP_FW_UPDATER_COMBO_0_2]);
	xSemaphoreGive (task->lock);

	return bytes;
}

static int tip_fw_update_task_prepare_staging (const struct firmware_update_control *update, size_t size)
{
	struct tip_fw_update_task *task = (struct tip_fw_update_task*) update;
	int status = 0;

	if (task == NULL) {
		return FIRMWARE_UPDATE_INVALID_ARGUMENT;
	}

	if (task->task) {
		xSemaphoreTake (task->lock, portMAX_DELAY);
		if (!task->running) {
			task->update_status = UPDATE_STATUS_STARTING;
			task->staging_size = size;
			task->running = 1;
			xSemaphoreGive (task->lock);
			xTaskNotify (task->task, PREP_STAGING_BIT, eSetBits);
		} else {
			task->update_status = UPDATE_STATUS_REQUEST_BLOCKED;
			status = FIRMWARE_UPDATE_TASK_BUSY;
			xSemaphoreGive (task->lock);
		}
	} else {
		task->update_status = UPDATE_STATUS_TASK_NOT_RUNNING;
		status = FIRMWARE_UPDATE_NO_TASK;
	}

	return status;
}

static int tip_fw_update_task_write_staging (const struct firmware_update_control *update, uint8_t *buf,
	size_t buf_len)
{
	struct tip_fw_update_task *task = (struct tip_fw_update_task*) update;
	int status = 0;

	if ((task == NULL) || (buf == NULL)) {
		return FIRMWARE_UPDATE_INVALID_ARGUMENT;
	}

	if (task->task) {
		xSemaphoreTake (task->lock, portMAX_DELAY);
		if (!task->running) {
			task->update_status = UPDATE_STATUS_STARTING;
			memcpy (task->staging_buf, buf, buf_len);
			task->staging_buf_len = buf_len;
			task->running = 1;
			xSemaphoreGive (task->lock);
			xTaskNotify (task->task, WRITE_TO_STAGING_BIT, eSetBits);
		} else {
			task->update_status = UPDATE_STATUS_REQUEST_BLOCKED;
			status = FIRMWARE_UPDATE_TASK_BUSY;
			xSemaphoreGive (task->lock);
		}
	} else {
		task->update_status = UPDATE_STATUS_TASK_NOT_RUNNING;
		status = FIRMWARE_UPDATE_NO_TASK;
	}

	return status;
}

static void tip_fw_update_task_status_change (const struct firmware_update_notification *context,
	enum firmware_update_status status)
{
	struct tip_fw_update_task_notify *notify = (struct tip_fw_update_task_notify*) context;

	if (notify != NULL) {
		xSemaphoreTake (notify->task->lock, portMAX_DELAY);
		notify->task->update_status = status;
		xSemaphoreGive (notify->task->lock);
	}
}

/**
 * Initialize the task interface for controlling the firmware update.
 *
 * @param task The task interface to initialize.
 * @param updater The updater instance in the task.
 * @param updater_cnt The number of updater instance to use in the task.
 * @param system The manager for system operations.
 *
 * @return 0 if the task was successfully initialized or an error code.
 */
int tip_fw_update_task_init (struct tip_fw_update_task *task, struct firmware_update *updater,
	int updater_cnt, struct system *system)
{
	if ((task == NULL) || (updater_cnt > NUM_TIP_FW_UPDATER) || (system == NULL)) {
		return FIRMWARE_UPDATE_INVALID_ARGUMENT;
	}

	memset (task, 0, sizeof (struct tip_fw_update_task));

	task->lock = xSemaphoreCreateMutex ();
	if (task->lock == NULL) {
		return FIRMWARE_UPDATE_NO_MEMORY;
	}

	for (int i = TIP_FW_UPDATER_COMBO_0_2; i < updater_cnt; i++) {
		task->updater[i] = &updater[i];
	}

	task->system = system;
	task->update_status = UPDATE_STATUS_NONE_STARTED;

	task->base.start_update = tip_fw_update_task_start_update;
	task->base.get_status = tip_fw_update_task_get_status;
	task->base.get_remaining_len = tip_fw_update_task_get_remaining_len;
	task->base.prepare_staging = tip_fw_update_task_prepare_staging;
	task->base.write_staging = tip_fw_update_task_write_staging;

	task->notify.base.status_change = tip_fw_update_task_status_change;
	task->notify.task = task;

	return 0;
}

/**
 * Start running the firmware update task.  This doesn't start an update process, just starts the
 * task that will run the update.  No update can be run until the update task has been started.
 *
 * @param task The update task to start.
 * @param stack_words The size of the update task stack.  The stack size is measured in words.
 * @param running_recovery Indicate that the system is running from the recovery image.
 *
 * @return 0 if the task was started or an error code.
 */
int tip_fw_update_task_start (struct tip_fw_update_task *task, uint16_t stack_words,
	bool running_recovery)
{
	int status;

	if (task == NULL) {
		return FIRMWARE_UPDATE_INVALID_ARGUMENT;
	}

	/* The task will clear the running flag after it has finished initializing the updater. */
	task->running = (running_recovery) ? 2 : 1;

	status = xTaskCreate ((TaskFunction_t) tip_fw_update_task_updater, "TIP FW Update", stack_words,
		task, CERBERUS_PRIORITY_NORMAL, &task->task);
	if (status != pdPASS) {
		task->task = NULL;
		return FIRMWARE_UPDATE_NO_MEMORY;
	}

	return 0;
}
