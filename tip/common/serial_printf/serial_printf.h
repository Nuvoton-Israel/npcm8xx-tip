/*
 *  Nuvoton Technology Corporation confidential

 *  Copyright (c) 2014-2021 by Nuvoton Technology Corporation
 *  All rights reserved

 *<<<--------------------------------------------------------------------------------
 * File Contents:
 *   serial_printf.h
 *            This file contains interface to serial printf module
 *  Project:
 *            SWC HAL
 */

#include "../../SWC_HAL/ModuleDrivers/uart/uart_if.h"

#ifndef __SERIAL_PRINTF_H
#define __SERIAL_PRINTF_H

#include "hal.h"
#include <stdbool.h>

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
void serial_printf_init (void);
void serial_printf_reconfig (void);
int serial_get_char (char *c, bool bWait);

#endif /*__SERIAL_PRINTF_H */
