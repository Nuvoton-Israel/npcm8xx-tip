// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#ifndef _TWD_TASK_H_
#define _TWD_TASK_H_

#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"


#define MIN_WD_DELAY 5
#define MAX_WD_DELAY 20

enum wd_update_status {
	TIP_WD_UPDATE_INVALID_SUCCESS = 0, /**< Successful update. */
	TIP_WD_UPDATE_ABORTED,			   /**< periodic task returned unexpectdly . */
	TIP_WD_UPDATE_INVALID_ARGUMENT,	   /**< Failed due to invalid argument. */
};

int tip_watchdog_service_start (TaskHandle_t *task, int delay, uint16_t stack_words);


#endif /* _TWD_TASK_H_ */
