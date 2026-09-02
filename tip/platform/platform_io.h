/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef PLATFORM_IO_H_
#define PLATFORM_IO_H_

#include <hal.h>


/* UART debug output. */
#define NEWLINE  "\n"

#define platform_printf  serial_printf

#define	platform_printf_dbg(x...)
// HAL_PRINT


#endif /* PLATFORM_IO_H_ */
