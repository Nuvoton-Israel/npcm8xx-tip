// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <stdlib.h>
#include <stdint.h>
#include "platform_base.h"


/* Initialization for platform abstraction routines. */
void platform_init (void);

typedef uint8_t platform_clock;
typedef uint32_t platform_timer;


#endif /* PLATFORM_H_ */
