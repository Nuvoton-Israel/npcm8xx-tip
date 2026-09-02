// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"


#ifdef _TIP_DEBUG_BUILD_
/**
  * Implementation of the application stack overflow hook.
  * It is called when a stack overflow is detected for a task.
  *
  * @param xTask the task that just exceeded its stack boundaries.
  * @param pcTaskName A character string containing the name of the offending task.
 */
void vApplicationStackOverflowHook (TaskHandle_t xTask, char *pcTaskName)
{
	taskDISABLE_INTERRUPTS ();
	platform_printf (KRED "%s: stack overflow detected!" NEWLINE KNRM, pcTaskName);
	for (;;);
}


#endif