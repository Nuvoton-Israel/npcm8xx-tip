/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef COMPOSITE_EAT_TYPES_H_
#define COMPOSITE_EAT_TYPES_H_

#include <stddef.h>
#include <stdint.h>


#define COMPOSITE_EAT_SHA384_LENGTH 48u

struct composite_eat_buffer {
	const uint8_t *data;
	size_t length;
};


#endif /* COMPOSITE_EAT_TYPES_H_ */
