/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/
#include "ModuleDrivers/uart/uart_if.h"

#ifndef __SERIAL_PRINTF_H
#define __SERIAL_PRINTF_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define KBOLD_ON "\x1b[1m"
#define KBOLD_OFF "\x1b[22m"

#define KNRM "\x1B[0m" KBOLD_OFF
#define KRED "\x1B[31m" KBOLD_ON
#define KGRN "\x1B[32m" KBOLD_ON
#define KYEL "\x1B[33m" KBOLD_ON
#define KBLU "\x1B[34m" KBOLD_ON
#define KMAG "\x1B[35m" KBOLD_ON
#define KCYN "\x1B[36m" KBOLD_ON
#define KWHT "\x1B[37m" KBOLD_ON

int	 serial_printf (const char *fmt, ...);
void serial_printf_init (bool is_uart_print_enabled, bool is_memory_print_enabled);
void serial_printf_reconfig (void);
int serial_get_char (char *c, bool bWait);
void memory_printf_init (uint32_t log_start_address, uint32_t log_max_size, uint32_t log_next_address);
void memory_printf_udpate (uint32_t log_start_address, uint32_t log_max_size, uint32_t log_next_address);


#endif /*__SERIAL_PRINTF_H */
