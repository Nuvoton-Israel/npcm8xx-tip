/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef TIP_FW_UPDATE_TASK_H_
#define TIP_FW_UPDATE_TASK_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "firmware/firmware_update.h"
#include "firmware/firmware_update_control.h"
#include "cmd_interface/cerberus_protocol.h"
#include "system/system.h"


/**
 * TIP FW updater based on combo type.
 *
 * Combo 0 and Combo 2 can share one updater since the same flash partition map can apply to both
 * cases. Combo 1 must have a separate updater.
 */
enum {
	TIP_FW_UPDATER_COMBO_0_2 = 0, 	/**< Updater for Combo 0 and Combo 2. */
	TIP_FW_UPDATER_COMBO_1 = 1, 	/**< Updater for Combo 1. */
	NUM_TIP_FW_UPDATER = 2,
};

struct tip_fw_update_task;

/**
 * An update notification implementation for the update task.
 */
struct tip_fw_update_task_notify {
	struct firmware_update_notification base;	/**< The base notification instance. */
	struct tip_fw_update_task *task;			/**< The parent task instance. */
};

/**
 * The task that will run the firmware update.
 */
struct tip_fw_update_task {
	struct firmware_update_control base;							/**< The base control instance. */
	struct tip_fw_update_task_notify notify;						/**< The update notification interface. */
	struct firmware_update *updater[NUM_TIP_FW_UPDATER];			/**< The firmware updater for TIP img combo. */
	struct system *system;											/**< The system manager. */
	int update_status;												/**< The last firmware update status. */
	uint8_t running;												/**< Flag indicating if an update is running. */
	TaskHandle_t task;												/**< The task that will run the update. */
	SemaphoreHandle_t lock;											/**< Mutex for status updates. */
	size_t staging_size;											/**< Size of image to clear in staging area */
	uint8_t staging_buf[CERBERUS_PROTOCOL_MAX_PAYLOAD_PER_MSG];		/**< Buffer of image data to write to staging area. */
	size_t staging_buf_len;											/**< Length of buffer of image data. */
};


int tip_fw_update_task_init (struct tip_fw_update_task *task, struct firmware_update *updater,
	int updater_cnt, struct system *system);
int tip_fw_update_task_start (struct tip_fw_update_task *task, uint16_t stack_words,
	bool running_recovery);


#endif /* TIP_FW_UPDATE_TASK_H_ */
