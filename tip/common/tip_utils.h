// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_UTILS_H
#define TIP_UTILS_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>


#define ARBEL_VERSION_Z1 0x00
#define ARBEL_VERSION_A1 0x04
#define ARBEL_VERSION_A2 0x08

uint32_t tip_memcpy (uint32_t dst_addr, uint32_t src_flash_addr, uint32_t src_size, bool dram,
	bool print);
void tip_memset (uint32_t addr, uint32_t val, uint32_t size, bool print);
int tip_memcmp (uint32_t addr1, uint32_t addr2, uint32_t size, bool print);
void hex_dump (uint32_t addr, uint32_t size, char *title);
void tip_mem_swap_endianness (void *data, uint8_t *out, uint32_t size);
void tip_print_status (const char *ver);
bool tip_check_recovery_boot (void);
void tip_select_next_boot_image (uint32_t offset);


#endif /* TIP_UTILS_H */
