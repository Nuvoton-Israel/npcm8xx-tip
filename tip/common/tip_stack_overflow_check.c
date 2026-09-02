// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"


#ifdef _TIP_DEBUG_BUILD_

static void tip_print_call_stack (uint32_t *stack_ptr, int num_frames)
{
	platform_printf (NEWLINE "Call Stack:" NEWLINE);

	/* Iterate through the call stack for a limited number of frames */
	for (int i = 0; i < num_frames && stack_ptr; ++i) {
		platform_printf ("Frame %d: Addr: %#010lx" NEWLINE, i, *(uint32_t *) stack_ptr);
		stack_ptr++;
	}
}

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
	register uint32_t *stack_ptr;

	/* Get the current stack pointer (do not relocate code into subfunctions) */
	__asm volatile ("MRS %0, msp" : "=r"(stack_ptr));
	platform_printf (KRED "Addr: %#010lx" NEWLINE,
		__builtin_extract_return_addr (__builtin_return_address (0)) - 1);

	platform_printf (KRED "%s: stack overflow detected!" NEWLINE KNRM, pcTaskName);
	tip_print_call_stack (stack_ptr, 10);
	for (;;);
}

/**
 * Implementation of the application memory allocation failure hook.
 * It is called when memory allocation failure is detected for a task.
 *
 */
void vApplicationMallocFailedHook ()
{
	taskDISABLE_INTERRUPTS ();
	register uint32_t *stack_ptr;

	/* Get the current stack ptr (do not relocate code into subfunctions) */
	__asm volatile ("MRS %0, msp" : "=r"(stack_ptr));
	platform_printf (KRED "Addr: %#010lx" NEWLINE,
		__builtin_extract_return_addr (__builtin_return_address (0)) - 1);

	platform_printf ("platform_malloc failed!" NEWLINE KNRM);
	tip_print_call_stack (stack_ptr, 10);
	for (;;);
}


#endif