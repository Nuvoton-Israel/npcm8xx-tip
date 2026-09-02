/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *   tip_virtual_flash.h
 *            This file contains TIP virtual flash definitions and declerations
 * Project:
 *            Arbel
 *------------------------------------------------------------------------*/

#ifndef TIP_FLASH_VIRTUAL_H_
#define TIP_FLASH_VIRTUAL_H_

#include "flash/flash_master.h"
#include "flash/spi_flash.h"


/**
 * TIP virtual flash.
 * Note: Do not reorder, it's the same as flash_master_tip.
 */
#pragma pack(push, 1)
struct tip_flash_master_virtual {
	struct flash_master base; /**< The base flash master instance. */
	uint32_t base_addr;		  /**< Base address. */
	uint32_t size;			  /**< The size of virtual flash device connected to the master.*/
};
#pragma pack(pop)


int tip_initialize_flash_access_virtual (struct spi_flash *flash, struct spi_flash_state *state,
										 const struct flash_master *flash_master, uint32_t addr,
										 uint32_t size);


#endif /* TIP_FLASH_VIRTUAL_H_ */
