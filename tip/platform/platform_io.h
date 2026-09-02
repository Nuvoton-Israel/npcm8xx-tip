// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef PLATFORM_IO_H_
#define PLATFORM_IO_H_

#include <hal.h>


/* UART debug output. */
#define NEWLINE  "\n"

#define platform_printf  serial_printf

#define	platform_printf_dbg(x...)
// HAL_PRINT


#endif /* PLATFORM_IO_H_ */
