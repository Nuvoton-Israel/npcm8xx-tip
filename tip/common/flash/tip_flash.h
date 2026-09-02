/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *    tip_flash.h
 *            This file contains TIP flash definitions and declerations
 * Project:
 *            Arbel
 *------------------------------------------------------------------------*/

#ifndef TIP_FLASH_H_
#define TIP_FLASH_H_

#include <stddef.h>
#include "flash/flash_master.h"
#include "flash/spi_flash.h"


#define FLASH_MAX_FIU FIU_MODULE_1
#define FLASH_MAX_CS  FIU_CS_1

#define FLASH_FIU0_QUAD    false
#define FLASH_FIU1_QUAD    false
#define FLASH_FIU3_QUAD    false

/* Do UMA operations only.*/
//#define FIU_DIRECT_READ

/**
 * A wrapper for the flash master API.
 * Do not reorder. This struct match the order of tip_flash_virtual.
 * Pack is required beacsue of the shared struct with the virtual flash.
 */
#pragma pack(push, 1)
struct tip_flash_master {
	struct flash_master base; 	/**< Flash master base instance. */
	uint32_t base_addr;		 	/**< The memory-mapped base address of FIU. */
	uint32_t size;
	uint32_t fiu; 			 	/**< FIU module */
	uint32_t cs;  				/**< Chip select */
};
#pragma pack(pop)


int tip_flash_mux_init (uint32_t fiu, uint32_t cs);
int tip_flash_initialize_access_single_flash (uint32_t fiu, uint32_t cs);
int tip_flash_initialize_access (uint32_t max_fiu, uint32_t max_cs);
struct spi_flash *tip_flash_get_spi_flash (uint32_t spi);
int tip_flash_get_layout (uint32_t max_fiu, uint32_t max_cs, struct spi_flash **main_flash,
	struct spi_flash **recovery_flash, struct spi_flash **active_flash, uint32_t *recovery_offset);
int tip_flash_reconfig_address_mode (uint32_t fiu, uint32_t cs);
bool tip_flash_direct_read_quad (uint32_t fiu);
int tip_flash_phys_to_logical (const uint32_t srcAddr, uint32_t *fiu, uint32_t *cs, uint32_t *spi,
	uint32_t *offset);
int tip_flash_logical_to_phys (uint32_t *physAddr, const uint32_t fiu, const uint32_t cs,
	uint32_t *spi, const uint32_t offset);
int tip_flash_get_index_of_currently_running_image (uint32_t *spi, uint32_t *fiu,
	uint32_t *cs, uint32_t *offset);
uint32_t tip_flash_get_base (uint32_t spi);
int tip_flash_protect_region (struct spi_flash *flash, uint32_t *addr_start, 
	uint32_t *addr_stop, uint32_t num_or_allowed_regions);
uint32_t tip_flash_get_fiu_cs (struct spi_flash *spi_fl, uint32_t *fiu, uint32_t *cs);
uint32_t tip_flash_get_recovery_phys_addr (void);

#endif /* TIP_FLASH_H_ */
