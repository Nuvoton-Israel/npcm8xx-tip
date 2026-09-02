/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2021 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *   tip_flash.c
 *            This file contains the flash flow and helper functions
 *  Project:  Arbel
 *------------------------------------------------------------------------*/

#include <string.h>
#include "flash/flash_common.h"
#include "tip_flash.h"
#include "hal.h"
#include "tip_boot.h"
#include "tip_rom_utils.h"
#include "platform_io.h"
#include "hal_regs.h"


#define BOOT_SPI_ALTERNATIVES	  12
#define FIU_NUM_OF_MODULES_ACTIVE 3
#define FIU_TIMEOUT				  100000


#ifndef TIP_FLASH_BASE_ADDR
const uint32_t TIP_FLASH_ADDR_ARRAY[(FIU_NUM_OF_MODULES + 1) * FLASH_NUM_OF_DEVICES] = {
	SPI0CS0_BASE_ADDR,
	SPI0CS1_BASE_ADDR,
	0,
	0,

	SPI1CS0_BASE_ADDR,
	SPI1CS1_BASE_ADDR,
	SPI1CS2_BASE_ADDR,
	SPI1CS3_BASE_ADDR,

	SPI3CS0_BASE_ADDR,
	SPI3CS1_BASE_ADDR,
	SPI3CS2_BASE_ADDR,
	SPI3CS3_BASE_ADDR,

	SPI3CS0_BASE_ADDR,
	SPI3CS1_BASE_ADDR,
	SPI3CS2_BASE_ADDR,
	SPI3CS3_BASE_ADDR
};

const uint32_t TIP_FLASH_SIZE_ARRAY[(FIU_NUM_OF_MODULES + 1) * FLASH_NUM_OF_DEVICES] = {
	SPI0CS0_SIZE,
	SPI0CS1_SIZE,
	0,
	0,

	SPI1CS0_SIZE,
	SPI1CS1_SIZE,
	SPI1CS2_SIZE,
	SPI1CS3_SIZE,

	SPI3CS0_SIZE,
	SPI3CS1_SIZE,
	SPI3CS2_SIZE,
	SPI3CS3_SIZE,

	SPI3CS0_SIZE,
	SPI3CS1_SIZE,
	SPI3CS2_SIZE,
	SPI3CS3_SIZE
};

#define TIP_FLASH_BASE_ADDR(fiu, cs) 	TIP_FLASH_ADDR_ARRAY[fiu * FLASH_NUM_OF_DEVICES + cs]
#define TIP_FLASH_SIZE(fiu, cs) 		TIP_FLASH_SIZE_ARRAY[fiu * FLASH_NUM_OF_DEVICES + cs]
#endif


/**
 * TIP flash master array.
 */
static struct tip_flash_master tip_flash_master[BOOT_SPI_ALTERNATIVES];

/**
 * TIP flash device array.
 */
static struct spi_flash tip_spi_flash[BOOT_SPI_ALTERNATIVES];

/**
 * Variable context array for the flash devices.
 */
static struct spi_flash_state tip_spi_flash_context[BOOT_SPI_ALTERNATIVES];

/**
 * Current state of the FIU address mode.
 * This flag is needed only when same FIU has different size flash connected to it.
 * When switching between flashes need to change FIU settings in this case.
 */
static uint32_t flash_FIU_addr_mode_state[FIU_NUM_OF_MODULES];

/**
 * read command for direct read is in default settings
 */
static uint8_t read_cmd_default[FIU_NUM_OF_MODULES];


/**
 * Get TIP spi_flash handler pointer
 *
 * @param spi Index in the tip flash array
 * @return struct spi_flash *  TIP spi_flash handler pointer or NULL
 */
struct spi_flash *tip_flash_get_spi_flash (uint32_t spi)
{
	return (spi < BOOT_SPI_ALTERNATIVES ? &tip_spi_flash[spi] : NULL);
}

/**
 * Get flash physical base address
 *
 * @param spi Index in the tip flash array
 * @return base address of flash master direct access memory window.
 */
uint32_t tip_flash_get_base (uint32_t spi)
{
	return (spi < BOOT_SPI_ALTERNATIVES ? tip_flash_master[spi].base_addr : 0);
}

/**
 * Get the index of flash from which the current image is running.
 *
 * @param spi SPI index.
 * @param fiu FIU index.
 * @param cs Chip select index.
 * @param offset Offset in flash.
 * @return int 0 if successful or an error code.
 */
int tip_flash_get_index_of_currently_running_image (uint32_t *spi, uint32_t *fiu,
	uint32_t *cs, uint32_t *offset)
{
	int status;
	uint32_t next_boot_addr = (READ_REG_FIELD (TIP_SEARCH_HDR_START, TIP_SEARCH_HDR_ADDR) << 19);

	status = tip_flash_phys_to_logical (next_boot_addr, fiu, cs, spi, offset);

	platform_printf (KGRN NEWLINE "tip rom will load from spi %d, fiu %d, cs %d offset %#010lx, \
		next_boot_addr %#010lx" NEWLINE KNRM,
		*spi, *fiu, *cs, *offset, next_boot_addr);

	if (status != 0) {
		return status;
	}
	return 0;
}

/**
 * Submit a transfer to be executed by the SPI master.
 *
 * @param spi The SPI master to use to execute the transfer.
 * @param xfer The transfer to execute.
 *
 * @return 0 if the transfer was executed successfully or an error code.
 */
int tip_flash_xfer (const struct flash_master *spi, const struct flash_xfer *xfer)
{
	struct tip_flash_master *fl_tip_m = (struct tip_flash_master *) spi;
	DEFS_STATUS hal_status;
	uint32_t fiu;
	uint32_t cs;
	uint32_t addrBytes = 0;
	uint32_t cmdBits;
	uint32_t addrBits;
	uint32_t dataBits;
	uint32_t dummy_bytes;

	if (fl_tip_m == NULL) {
		return FLASH_MASTER_INVALID_ARGUMENT;
	}

	fiu = fl_tip_m->fiu;
	cs = fl_tip_m->cs;

#ifdef FIU_DIRECT_READ
	/* Handle direct read commands */
	if ((xfer->cmd == FLASH_CMD_READ) ||
		(xfer->cmd == FLASH_CMD_FAST_READ) ||
		(xfer->cmd == FLASH_CMD_4BYTE_READ) ||
		(xfer->cmd == FLASH_CMD_4BYTE_FAST_READ) ||
		(xfer->cmd == FLASH_CMD_DUAL_READ) ||
		(xfer->cmd == FLASH_CMD_DIO_READ)  ||
		(xfer->cmd == FLASH_CMD_QUAD_READ) ||
		(xfer->cmd == FLASH_CMD_QIO_READ)  ||
		(xfer->cmd == FLASH_CMD_4BYTE_QIO_READ)) {

		uint32_t spi_ind = 0;
		int status;
		uint32_t phys_addr;
		uint32_t dst_addr = (uint32_t) xfer->data;
		
		/* check if switching between flashes with different sizes on same FIU: */
		status = tip_flash_reconfig_address_mode (fiu, cs);
		if (status != 0) {
			return status;
		}

		/* replace default read command if needed */
		if (READ_REG_FIELD (FIU_DRD_CFG(fiu), FIU_DRD_CFG_RDCMD) != xfer->cmd) {
			FIU_ConfigCommand (fiu, xfer->cmd, 0);
			read_cmd_default[fiu] = xfer->cmd;
			platform_printf_dbg ("tip restore read command FIU_DRD_CFG[%d] %#010lx" NEWLINE KNRM, 
				fiu, REG_READ (FIU_DRD_CFG (fiu)));
		}

		/* take the address from the flash_xfer request, convert to TIP address space
		 * This is needed in order to use direct access.*/
		status = tip_flash_logical_to_phys (&phys_addr, fiu, cs, &spi_ind, xfer->address);
		if (status != 0) {
			platform_printf (KRED "tip_flash_xfer addr out of range addr %#010lx" NEWLINE KNRM,
				xfer->address);
			return status;
		}
		if (xfer->flags & FLASH_FLAG_QUAD_DATA) {
			FIU_ConfigReadMode (fiu, FIU_FAST_READ_QUAD_IO);
			FIU_ConfigDummyCyclesCount (fiu, FIU_DUMMY_COUNT_3);
		}

		/* if src, dst and length are alligned to 4 bytes copy in uint32_t which 
		 * yields faster transactions on the bus.
		 * Otherwise perform memcpy which AHB pass as single byte reads.
		 */
		if ((dst_addr % 4 == 0) && (phys_addr % 4 == 0) && (xfer->length % 4 == 0)) {
			for (int cnt = 0; cnt < xfer->length; cnt += 4) {
				*(uint32_t*) (dst_addr + cnt) = *(uint32_t*) (phys_addr + cnt);
			}
		}
		else {
			/* single byte reads. Less optimized. */
			memcpy (xfer->data, (uint8_t *) phys_addr, xfer->length);
		}
		
		return 0;
	}
#endif

	platform_printf_dbg ("\ttip_flash_xfer: CMD 0x%lx, fiu%d, cs%d, phys %#010lx, len 0x%lx \
		flags %#010lx" NEWLINE,
		xfer->cmd, fiu, cs, phys_addr, xfer->length, xfer->flags);

	if (xfer->flags & FLASH_FLAG_NO_ADDRESS) {
		addrBytes = 0;
	} else {
		addrBytes = (xfer->flags & FLASH_FLAG_4BYTE_ADDRESS) ? 4 : 3;
	}

	addrBits = 1;
	if (xfer->flags & FLASH_FLAG_NO_ADDRESS) {
		addrBits = 1;
	} else {
		switch (xfer->flags & (FLASH_FLAG_DUAL_ADDR | FLASH_FLAG_QUAD_ADDR)) {
			case FLASH_FLAG_DUAL_ADDR:
				addrBits = 2;
				break;

			case FLASH_FLAG_QUAD_ADDR:
				addrBits = 4;
				break;

			default:
				addrBits = 1;
		}
	}

	cmdBits = 1;
	switch (xfer->flags & (FLASH_FLAG_DUAL_CMD | FLASH_FLAG_QUAD_CMD)) {
		case FLASH_FLAG_DUAL_CMD:
			cmdBits = 2;
			break;

		case FLASH_FLAG_QUAD_CMD:
			cmdBits = 4;
			break;

		default:
			cmdBits = 1;
	}

	dataBits = 1;
	switch (xfer->flags & (FLASH_FLAG_DUAL_DATA | FLASH_FLAG_QUAD_DATA)) {
		case FLASH_FLAG_DUAL_DATA:
			dataBits = 2;
			break;

		case FLASH_FLAG_QUAD_DATA:
			dataBits = 4;
			break;

		default:
			dataBits = 1;
	}

#ifdef PRE_PRODUCTION
	/* erase commands are slow, warn the user */
	if((xfer->cmd == FLASH_CMD_4BYTE_4K_ERASE) || 
		(xfer->cmd == FLASH_CMD_4BYTE_64K_ERASE) || 
		(xfer->cmd == FLASH_CMD_4K_ERASE)  ||
		(xfer->cmd == FLASH_CMD_64K_ERASE))
		platform_printf("ERASE fiu%d cs%d %#010lx" NEWLINE, fiu, cs, xfer->address);

	/* program commands are slow, warn the user */
	if (((xfer->cmd == FLASH_CMD_PP) || 
		(xfer->cmd == FLASH_CMD_4BYTE_PP)) && ((xfer->address & 0xFFFF) == 0))
		platform_printf("PROG fiu%d cs%d %#010lx" NEWLINE, fiu, cs, xfer->address);
#endif

	dummy_bytes = xfer->dummy_bytes;
	if (addrBits > 1) {
		++dummy_bytes;
	}

	hal_status = FIU_UMA_ioctl (fiu, cs,
								xfer->cmd,
								xfer->address,
								addrBytes,
								xfer->data,
								(xfer->flags & FLASH_FLAG_DATA_TX) ? 0 : xfer->length,
								xfer->data,
								(xfer->flags & FLASH_FLAG_DATA_TX) ? xfer->length : 0, cmdBits,
								addrBits,
								dataBits,
								dataBits,
								FIU_TIMEOUT,
								dummy_bytes);

	/* config next direct read op to 4 bytes */
	if (xfer->cmd == FLASH_CMD_EN4B) {
		FIU_Config4ByteAddress (fiu, cs, cs, TRUE);
		flash_FIU_addr_mode_state[fiu] = 4;
	}

	/* config next direct read op to 3 bytes */
	if (xfer->cmd == FLASH_CMD_EX4B) {
		FIU_Config4ByteAddress (fiu, cs, cs, FALSE);
		flash_FIU_addr_mode_state[fiu] = 3;
	}

	if (hal_status != DEFS_STATUS_OK) {
		platform_printf_dbg ("TIP_FLASH XFER failed fiu%d cs%d status %#010lx" NEWLINE, fiu, cs, hal_status);
		return FLASH_MASTER_XFER_FAILED;
	}

	return 0;
}

/**
 * Get a set of capabilities supported by the SPI master.
 *
 * @param spi The SPI master to query.
 *
 * @return A capabilities bitmask for the SPI master.
 */
static uint32_t tip_flash_capabilities (const struct flash_master *spi)
{
	return FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR;
}

/**
 * Get a set of capabilities supported by the SPI master.
 *
 * @param spi The SPI master to query.
 *
 * @return A capabilities bitmask for the SPI master.
 */
static uint32_t tip_flash_capabilities_quad (const struct flash_master *spi)
{
	return FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR | FLASH_CAP_QUAD_1_4_4;
}

/**
 * Initialize SWC_HAL side only
 *
 * @return 0 if flash access was successfully initialized or an error code.
 */
static void tip_flash_init_data_array (void)
{
	uint32_t bootIdx = 0;
	uint32_t cs;
	uint32_t fiu;

	memset (tip_flash_master, 0, sizeof (tip_flash_master));
	memset (tip_spi_flash, 0, sizeof (tip_spi_flash));

	/* Init flash muxing */
	for (fiu = FIU_MODULE_0; fiu < FIU_MODULE_3; ++fiu) {
		for (cs = FIU_CS_0; cs <= FIU_CS_3; ++cs) {
			bootIdx = 4 * fiu + cs;

			tip_flash_master[bootIdx].base.xfer = tip_flash_xfer;
			tip_flash_master[bootIdx].base.capabilities = tip_flash_capabilities;

			/* FIU0 HW set to quad mode */
			if ((fiu == FIU_MODULE_0) && FLASH_FIU0_QUAD) {
				tip_flash_master[bootIdx].base.capabilities = tip_flash_capabilities_quad;
			}

			/* FIU1 HW set to quad mode */
			if ((fiu == FIU_MODULE_1) && FLASH_FIU1_QUAD) {
				tip_flash_master[bootIdx].base.capabilities = tip_flash_capabilities_quad;
			}
			
			/* FIU3 HW set to quad mode */
			if ((fiu == FIU_MODULE_2) && FLASH_FIU3_QUAD) {
				tip_flash_master[bootIdx].base.capabilities = tip_flash_capabilities_quad;
			}

			tip_flash_master[bootIdx].fiu = fiu;
			tip_flash_master[bootIdx].cs = cs;
			tip_flash_master[bootIdx].base_addr = TIP_FLASH_BASE_ADDR (fiu, cs);

			if (tip_flash_master[bootIdx].base_addr == 0)
				continue;

			/* Minimal flash size until proven otherwise. */
			tip_flash_master[bootIdx].size = _1MB_;
		}

		/* FIU is set to 3 bytes address mode by default. */
		flash_FIU_addr_mode_state[fiu] = 3;

		/* check which read command is configured to the module */
		read_cmd_default[fiu] = READ_REG_FIELD (FIU_DRD_CFG (fiu), FIU_DRD_CFG_RDCMD);
	}
}

/**
 * Initialize access to the flash for the Cerberus.
 *
 * @param fiu FIU index. FIU is the Flash Interface Unit of the TIP.
 * @param cs CS index. Chip Select. Each FIU has up to 4 Chip selects connected to
 *        up to 4 flashes.
 * @return 0 if success or an error code
 */
int tip_flash_initialize_access_single_flash (uint32_t fiu, uint32_t cs)
{
	uint32_t bootIdx = 0;
	int status;
	uint32_t size = 0;

	if ((fiu >= FIU_NUM_OF_MODULES) || (cs >= FLASH_NUM_OF_DEVICES)) {
		platform_printf (KRED "tip_flash_initialize_access_single_flash out of " 
			"bounds fiu=%d cs=%d" NEWLINE KNRM, fiu, cs);
		return FLASH_ADDRESS_OUT_OF_RANGE;
	}

	bootIdx = 4 * fiu + cs;

	tip_flash_mux_init (fiu, cs);
	FIU_Init (fiu, cs, 0);

	/* assume FIU module direct read in 3 bytes mode, until proven otherwise */
	FIU_Config4ByteAddress (fiu, cs, cs, FALSE);

	FIU_ConfigDummyCyles (fiu, 8, TRUE);

	status = spi_flash_initialize_device (&tip_spi_flash[bootIdx],
										  &tip_spi_flash_context[bootIdx],
										  &tip_flash_master[bootIdx].base, true, false,
										  true, false);

	if (status != 0) {
		if (status == SPI_FLASH_NO_DEVICE) {
			platform_printf ("fiu%d cs%d: flash not detected" NEWLINE KNRM, fiu, cs);
		} else {
			platform_printf (KRED "fiu%d cs%d status %x: flash init error" NEWLINE KNRM,
				fiu, cs, status);
		}
	}

	status = spi_flash_get_device_size (&tip_spi_flash[bootIdx], &size);
	if (status != 0) {
		platform_printf (KRED "fiu%d cs%d status %x: get device size failed" NEWLINE KNRM,
			fiu, cs, status);
		return status;
	}

	tip_flash_master[bootIdx].size = size;

	if (size > _16MB_) {
		status = spi_flash_enable_4byte_address_mode (&tip_spi_flash[bootIdx], true);
		FIU_Config4ByteAddress (fiu, cs, cs, true);
		flash_FIU_addr_mode_state[fiu] = 4;

		if (status != 0) {
			platform_printf (KRED "flash enable 4B fiu%d cs%d status %#010lx" NEWLINE KNRM,
				fiu, cs, status);
			return status;
		}
	} else {
		/* flash needs only 4 bytes. No need to configure it.
		 * return FIU to 3 bytes state
		 */
		if (flash_FIU_addr_mode_state[fiu] == 4) {
			FIU_Config4ByteAddress (fiu, cs, cs, false);
			flash_FIU_addr_mode_state[fiu] = 3;
		}
	}

	FIU_ConfigCommand (fiu, tip_spi_flash[bootIdx].state->command.read,
		tip_spi_flash[bootIdx].state->command.write);
	FIU_ConfigDummyCyles (fiu, tip_spi_flash[bootIdx].state->command.read_dummy * 8, TRUE);
	read_cmd_default[fiu] = tip_spi_flash[bootIdx].state->command.read;

	status = tip_flash_direct_read_quad (fiu);
	if (status == 0) {
		platform_printf ("fiu%d: 1-4-4 mode enabled" NEWLINE, fiu);
	}

	platform_printf (KGRN "%s complete fiu%d cs%d pass %x, flash size %#010lx" NEWLINE KNRM,
				__func__, fiu, cs, status, tip_flash_master[bootIdx].size);

	return 0;
}

/**
 * Initialize access to the flash for the Cerberus.
 *
 * @param max_fiu FIU index
 * @param max_cs CS index
 * @return 0 if success or an error code
 */
int tip_flash_initialize_access (uint32_t max_fiu, uint32_t max_cs)
{
	uint32_t bootIdx = 0;
	uint32_t cs = 0;
	uint32_t fiu = 0;
	int status;
	uint32_t size = 0;

	if ((max_fiu >= FIU_NUM_OF_MODULES_ACTIVE) || (max_cs >= FLASH_NUM_OF_DEVICES)) {
		platform_printf (KRED "tip_flash_initialize_access out of bounds fiu=%d cs=%d" NEWLINE KNRM,
			max_fiu, max_cs);
		return FLASH_ADDRESS_OUT_OF_RANGE;
	}

	/* init SW related data */
	tip_flash_init_data_array ();

	/* init HW: mux all used modules */
	for (fiu = FIU_MODULE_0; fiu <= max_fiu; ++fiu) {
		for (cs = FIU_CS_0; cs <= max_cs; ++cs) {
			tip_flash_mux_init (fiu, cs);
		}

		/* start with 3 bytes address mode before init */
		FIU_Init (fiu, cs, 0);
	}

	/* cerberus flash init */
	for (fiu = FIU_MODULE_0; fiu <= max_fiu; ++fiu) {
		cs = FIU_CS_0;

		/* assume FIU module direct read in 3 bytes mode, until proven otherwise */
		FIU_Config4ByteAddress (fiu, cs, cs, FALSE);

		FIU_ConfigDummyCyles (fiu, 8, TRUE);

		for (cs = FIU_CS_0; cs <= max_cs; ++cs) {
			bootIdx = 4 * fiu + cs;

			if (tip_flash_master[bootIdx].base_addr == 0)
				continue;

			if ((bootIdx == 1) && READ_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_BSPI01_DIS)) {
				platform_printf ("Skip FIU0 CS1" NEWLINE);
				continue;
			}

			if ((fiu == 1) && READ_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_BSPI10_DIS)) {
				platform_printf ("Skip FIU1" NEWLINE);
				continue;
			}

			status = spi_flash_initialize_device (&tip_spi_flash[bootIdx],
												  &tip_spi_flash_context[bootIdx],
												  &tip_flash_master[bootIdx].base, true, false,
												  true, false);
			if (status != 0) {
				if (status == SPI_FLASH_NO_DEVICE) {
					platform_printf ("fiu%d cs%d: flash not detected" NEWLINE KNRM, fiu, cs);
				} else {
					platform_printf (KRED "fiu%d cs%d status %x: flash init error" NEWLINE KNRM,
						fiu, cs, status);
				}

				if (flash_FIU_addr_mode_state[fiu] == 4) {
					FIU_Config4ByteAddress (fiu, cs, cs, TRUE);
				}

				continue;
			}

			status = spi_flash_get_device_size (&tip_spi_flash[bootIdx], &size);
			if (status != 0) {
				platform_printf (KRED "fiu%d cs%d status %x: get device size failed" NEWLINE KNRM,
					fiu, cs, status);
				continue;
			}

			tip_flash_master[bootIdx].size = size;

			if (size > _16MB_) {
				status = spi_flash_enable_4byte_address_mode (&tip_spi_flash[bootIdx], true);

				if (status != 0) {
					platform_printf (KRED "flash enable 4B fiu%d cs%d status %#010lx" NEWLINE KNRM,
						fiu, cs, status);
					continue;
				}
			} else {
				/* flash needs only 4 bytes. No need to configure it.
				 * return FIU to 3 bytes state
				 */
				if (flash_FIU_addr_mode_state[fiu] == 4) {
					FIU_Config4ByteAddress (fiu, cs, cs, FALSE);
					flash_FIU_addr_mode_state[fiu] = 3;
				}
			}


			FIU_ConfigCommand (fiu, tip_spi_flash[bootIdx].state->command.read,
				tip_spi_flash[bootIdx].state->command.write);
			FIU_ConfigDummyCyles (fiu, tip_spi_flash[bootIdx].state->command.read_dummy * 8, TRUE);
			read_cmd_default[fiu] = tip_spi_flash[bootIdx].state->command.read;

			platform_printf (KGRN "%s complete fiu%d cs%d pass %x, flash size %#010lx" NEWLINE KNRM,
				__func__, fiu, cs, status, tip_flash_master[bootIdx].size);
		}

		status = tip_flash_direct_read_quad (fiu);
		if (status == 0) {
			platform_printf ("fiu%d: 1-4-4 mode enabled" NEWLINE, fiu);
		}
	}

	return 0;
}

/**
 * Get main flash and recovery flash handler. Must be called after tip_flash_initialize_access.
 *
 * @param max_fiu FIU index
 * @param max_cs CS index
 * @param main_flash Pointer to main flash handler
 * @param recovery_flash Pointer to recovery flash handler
 * @param active_flash Pointer to active flash handler
 * @param recovery_offset Offset for recovery flash
 * @return 0 if success -1 failure
 */
int tip_flash_get_layout (uint32_t max_fiu, uint32_t max_cs, struct spi_flash **main_flash,
	struct spi_flash **recovery_flash, struct spi_flash **active_flash, uint32_t *recovery_offset)
{
	int flash_cnt = 0;
	uint32_t fiu;
	uint32_t cs;
	uint32_t offset;
	uint32_t bootIdx;
	uint32_t main_flash_ind;
	uint32_t active_flash_ind;
	int status;

	*recovery_offset = 0;
	*main_flash = NULL;
	*recovery_flash = NULL;
	*active_flash = NULL;

	status = tip_flash_get_index_of_currently_running_image (&active_flash_ind, &fiu, &cs, &offset);
	if (status != 0) {
		platform_printf (KRED "Failed to get ACTIVE flash" NEWLINE KNRM, status);
		return -1;
	}

	*active_flash = tip_flash_get_spi_flash (active_flash_ind);
	platform_printf (KGRN "%s ACTIVE flash fiu%d cs%d" NEWLINE KNRM, __func__, fiu, cs);

	/* check how many flashes are connect and assign roles for main and recovery */
	for (fiu = FIU_MODULE_0; fiu <= max_fiu; ++fiu) {
		for (cs = FIU_CS_0; cs <= max_cs; ++cs) {
			bootIdx = 4 * fiu + cs;

			if (tip_flash_master[bootIdx].size >= _4MB_) {
				if (flash_cnt == 0) {
					*main_flash = &tip_spi_flash[bootIdx];
					main_flash_ind = bootIdx;
					platform_printf (KGRN "%s MAIN flash fiu%d cs%d" NEWLINE KNRM, __func__, fiu,
						cs);
				}

				/* option 1: found two good flashes: */
				if (flash_cnt == 1) {
					*recovery_flash = &tip_spi_flash[bootIdx];
					platform_printf (KGRN "%s RECOVERY flash fiu%d cs%d" NEWLINE KNRM, __func__,
						fiu, cs);
				}

				/* two flashes is enough */
				if (++flash_cnt >= 2) {
					return 0;
				}
			}
		}
	}

	/* option 2: only one big flash was found, split it to two. */
	if (flash_cnt == 1) {
		*recovery_flash = *main_flash;
		*recovery_offset = (*main_flash)->state->device_size >> 1;
		platform_printf (KGRN "%s RECOVERY flash fiu%d cs%d offset %#010lx" NEWLINE KNRM, __func__,
			tip_flash_master[main_flash_ind].fiu, tip_flash_master[main_flash_ind].cs,
			*recovery_offset);

		return 0;
	}

	return -1;
}

/**
 * Configure the controller to 1-4-4 mode (quad)
 *
 * This function configures direct mode access to quad, if
 * FLASH_FIU0_QUAD\FLASH_FIU1_QUAD\FLASH_FIU3_QUAD
 * are enabled. This setting impact all CS.
 *
 * @param fiu FIU indexes
 *
 * @return 0 if success or an error code
 */
int tip_flash_direct_read_quad (uint32_t fiu)
{
	uint8_t default_cmd = FLASH_CMD_FAST_READ;
	bool quad = false;

	if (fiu >= FIU_NUM_OF_MODULES_ACTIVE) {
		return FLASH_INVALID_ARGUMENT;
	}

	/* FIU0 HW set to quad mode */
	if ((fiu == FIU_MODULE_0) && FLASH_FIU0_QUAD) {
		quad = true;
	}

	/* FIU1 HW set to quad mode */
	if ((fiu == FIU_MODULE_1) && FLASH_FIU1_QUAD) {
		quad = true;
	}
	
	/* FIU3 HW set to quad mode */
	if ((fiu >= FIU_MODULE_2) && FLASH_FIU3_QUAD) {
		quad = true;
	}

	if (quad == true) {
		if (flash_FIU_addr_mode_state[fiu] == 4)
			default_cmd = FLASH_CMD_4BYTE_QIO_READ;
		else
			default_cmd = FLASH_CMD_QIO_READ;
		FIU_ConfigCommand (fiu, default_cmd , 0);
		FIU_ConfigReadMode (fiu, FIU_FAST_READ_QUAD_IO);
		FIU_ConfigDummyCyclesCount (fiu, FIU_DUMMY_COUNT_3);
		return 0;
	}

	return 0;
}

/**
 * Set addressing mode for active flash
 *
 * This function should be called after tip_flash_initialize_access, and before start to
 * direct reading from a specific flash.
 *
 * @param fiu FIU index
 * @param cs CS index
 * @return 0 if success or an error code
 */
int tip_flash_reconfig_address_mode (uint32_t fiu, uint32_t cs)
{
	int bootIdx = 4 * fiu + cs;

	if (bootIdx >= BOOT_SPI_ALTERNATIVES) {
		return FLASH_INVALID_ARGUMENT;
	}

	if (tip_spi_flash[bootIdx].state->device_size == 0) {
		platform_printf ("%s: select fiu%d cs%d: flash disconnect" NEWLINE, __func__, fiu, cs);
		return FLASH_NO_MEMORY;
	}

	/* after initializing all flashes, reconfigure the FIU to match the flash size on CS0~ (if it's
	 * connected) */
	if ((tip_spi_flash[bootIdx].state->device_size <= _16MB_) &&
		(flash_FIU_addr_mode_state[fiu] == 4)) {
		platform_printf ("%s: select fiu%d cs%d 3B addr" NEWLINE, __func__, fiu, cs);
		FIU_Config4ByteAddress (fiu, cs, cs, FALSE);
		flash_FIU_addr_mode_state[fiu] = 3;
		return 0;
	}

	if ((tip_spi_flash[bootIdx].state->device_size > _16MB_) &&
		(flash_FIU_addr_mode_state[fiu] == 3)) {
		platform_printf ("%s: select fiu%d cs%d 4B addr" NEWLINE, __func__, fiu, cs);
		FIU_Config4ByteAddress (fiu, cs, cs, TRUE);
		flash_FIU_addr_mode_state[fiu] = 4;
		return 0;
	}

	tip_flash_direct_read_quad (fiu);

	return 0;
}

/**
 * Initialize flash mux
 *
 * @param fiu FIU index
 * @param cs CS index
 * @return 0 if success or an error code
 */
int tip_flash_mux_init (uint32_t fiu, uint32_t cs)
{
	if (fiu > FIU_MODULE_3) {
		return FLASH_INVALID_ARGUMENT;
	}

	if (fiu == FIU_MODULE_0) {
		CHIP_MuxFIU (FIU_MODULE_0, TRUE,
			READ_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_BSPI01_DIS) ? FALSE : TRUE, FALSE, FALSE,
			FLASH_FIU0_QUAD);
	}

	/* Mux FIU1 */
	if (fiu == FIU_MODULE_1) {
		/* check if FIU1 is disabled via OTP bit */
		if (READ_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_BSPI10_DIS)) {
			return -1;
		}

		/* If STRAP3 is active (low), then set VSRCR bit 2 to select the SPI0 supply level (VSBV3
		 * level is 1.8V).*/
		SET_REG_FIELD (VSRCR, VSRCR_V4, !READ_REG_FIELD (PWRON, PWRON_SPI1F18));

		CHIP_MuxFIU (FIU_MODULE_1, TRUE, TRUE, FALSE, FALSE, FLASH_FIU1_QUAD);
	}

	if ((fiu == FIU_MODULE_2) || (fiu == FIU_MODULE_3)) {
		/*
		 * If STRAP12 is active (low): And also if boot from SP
		 *  is not disabled (BSPI10_DIS is 0), set VSRCR bit 3
		 * to select the SPI1 supply level (VSBV4 level is 1.8V)
		 */
		SET_REG_FIELD (VSRCR, VSRCR_V4, !READ_REG_FIELD (PWRON, PWRON_SPI1F18));

		CHIP_MuxFIU (FIU_MODULE_2, TRUE, FALSE, FALSE, FALSE, FLASH_FIU3_QUAD);
	}

	return 0;
}

/**
 * Translate logical address to physical address for flash access.
 *
 * This routine sets the device number according to the source address.
 * If the address is not in flash memory map range - returns a corresponding status.
 *
 * @param[out] physAddr Physical address (Should be in flash memory map range)
 * @param fiu FIU index
 * @param cs Chip select
 * @param[out] spi Index of flash relative to array of boot devices
 * @param offset Offset in flash
 * @return 0 if success or error code
 */
int tip_flash_logical_to_phys (uint32_t *physAddr, const uint32_t fiu, const uint32_t cs,
	uint32_t *spi, const uint32_t offset)
{
	if (physAddr == NULL || spi == NULL) {
		return FLASH_INVALID_ARGUMENT;
	}

	for (int bootIdx = 0; bootIdx < BOOT_SPI_ALTERNATIVES; ++bootIdx) {
		if ((fiu == tip_flash_master[bootIdx].fiu) && (cs == tip_flash_master[bootIdx].cs)) {
			*spi = bootIdx;
			if (offset >= tip_flash_master[bootIdx].size) {
				return FLASH_ADDRESS_OUT_OF_RANGE;
			}

			*physAddr = tip_flash_master[bootIdx].base_addr + offset;
			platform_printf_dbg ("tip addr %#010lx, fiu%d cs%d spi%d offset %#010lx" NEWLINE,
				*physAddr, fiu, cs, *spi, offset);

			return 0;
		}
	}

	return FLASH_INVALID_ARGUMENT;
}

/**
 * Translate physical address to logical address for flash access.
 *
 * This routine sets the device number according to the source address.
 * If the address is not in flash memory map range - returns a corresponding status.
 *
 * @param physAddr Physical address (Should be in flash memory map range)
 * @param[out] fiu FIU index
 * @param[out] cs Chip select
 * @param[out] spi Index of flash relative to array of boot devices
 * @param[out] offset Offset in flash
 * @return 0 if success or error code
 *
 */
int tip_flash_phys_to_logical (const uint32_t srcAddr, uint32_t *fiu, uint32_t *cs, uint32_t *spi,
	uint32_t *offset)
{
	if (fiu == NULL || cs == NULL || spi == NULL || offset == NULL) {
		return FLASH_INVALID_ARGUMENT;
	}

	for (int bootIdx = 0; bootIdx < BOOT_SPI_ALTERNATIVES; ++bootIdx) {
		if ((srcAddr >= tip_flash_master[bootIdx].base_addr) &&
			(srcAddr < (tip_flash_master[bootIdx].base_addr + tip_flash_master[bootIdx].size))) {
			*fiu = tip_flash_master[bootIdx].fiu;
			*cs = tip_flash_master[bootIdx].cs;
			*spi = bootIdx;
			*offset = srcAddr - tip_flash_master[bootIdx].base_addr;

			platform_printf_dbg (
				"tip start from addr %#010lx, fiu%d cs%d spi%d offset %#010lx" NEWLINE, srcAddr,
				*fiu, *cs, *spi, *offset);
			return 0;
		}
	}

	return FLASH_ADDRESS_OUT_OF_RANGE;
}

/**
 * Lock a flash region as read only
 *
 * This routine configures a flash region to be read only. This setting is cleared by PORST only.
 * Locking a region is intended to lock recovery image, as well as protecting flash from accidential
 * changes.
 *
 * @param flash[in] flash to lock.
 * @param addr_start[in] Start of region. Addresses are in logical format.
 * @param addr_stop[in] End of region.
 * @param num_or_allowed_regions[in]: num of ranges. up to 2.
 * @return 0 if success or error code
 *
 */
int tip_flash_protect_region (struct spi_flash *flash, uint32_t *addr_start, 
							  uint32_t *addr_stop, uint32_t num_or_allowed_regions)
{
	struct tip_flash_master *fl_tip_m = (struct tip_flash_master *) flash->spi;
	uint32_t fiu = fl_tip_m->fiu;
	uint32_t cs = fl_tip_m->cs;
	FIU_PROTECTION_SETTING_T setting;
	int rgn = 0;
	int cmd = 0;
	DEFS_STATUS stat = 0;

	/* in current implementation can support up to 2 RO ranges on flash.*/
	if (num_or_allowed_regions > 2)
		return FLASH_INVALID_ARGUMENT;

	/* Reading is always allowed */
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_READ      , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_FAST_READ , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_DUAL_READ , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_DIO_READ  , 3, 1, 2);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_QUAD_READ , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_QIO_READ  , 3, 1, 4);

	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_4BYTE_READ     , 4, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_4BYTE_FAST_READ, 4, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_4BYTE_DUAL_READ, 4, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_4BYTE_DIO_READ , 4, 1, 2);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_4BYTE_QUAD_READ, 4, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_4BYTE_QIO_READ , 4, 1, 4);

	/* allow reading flash settings commands are also allowed */
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_RDSR     , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_WREN     , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_RDSR3    , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_RDSR2    , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_ALT_RDSR2, 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_SFDP     , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_RDSR_FLAG, 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_RDID     , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_RDP      , 3, 1, 1);
	stat += FIU_SetProtectionCommand (fiu, cmd++, 0x7, FLASH_CMD_RD_NV_CFG, 3, 1, 1);

	/* Note: the following command will be blockes on this CS device: 
	 * FLASH_CMD_EX4B, FLASH_CMD_ALT_RST, FLASH_CMD_DP, FLASH_CMD_EN4B, FLASH_CMD_WR_NV_CFG,
	 * FLASH_CMD_RST, FLASH_CMD_GBULK, FLASH_CMD_RSTEN, FLASH_CMD_VOLATILE_WREN, 
	 * FLASH_CMD_ALT_WRSR2, FLASH_CMD_WRSR2
	 */

	/* full chip erase is not allowed if there are RO regions */
	FIU_SetProtectionCommand (fiu, cmd++, 0x6 ,FLASH_CMD_CE, 3, 1, 1);

	/* for each region on the list: define the range and block erase and program commands to that region */
	for (int i = 0; i < num_or_allowed_regions; i++) {

		/* Set RO region */
		platform_printf ("Flash protection FIU%d CS%d RO %#010lx-%#010lx" NEWLINE, 
			fiu, cs, addr_start[i], addr_stop[i]);

		stat += FIU_SetProtectionRegion (fiu, rgn, addr_start[i], addr_stop[i], cs + 1);

		/* erase is allowed only on the RW regions */
		stat += FIU_SetProtectionCommand (fiu, cmd++, rgn, FLASH_CMD_4K_ERASE,        3, 1, 1);
		stat += FIU_SetProtectionCommand (fiu, cmd++, rgn, FLASH_CMD_4BYTE_4K_ERASE,  4, 1, 1);
		stat += FIU_SetProtectionCommand (fiu, cmd++, rgn, FLASH_CMD_64K_ERASE,       3, 1, 1);
		stat += FIU_SetProtectionCommand (fiu, cmd++, rgn, FLASH_CMD_4BYTE_64K_ERASE, 4, 1, 1);
		if (stat != 0)
			return FLASH_INVALID_ARGUMENT;

		/* programming is allowed only on the RW regions */
		stat += FIU_SetProtectionCommand (fiu, cmd++, rgn, FLASH_CMD_PP ,       3, 1, 1);
		stat += FIU_SetProtectionCommand (fiu, cmd++, rgn, FLASH_CMD_4BYTE_PP , 4, 1, 1);
		if (stat != 0)
			return FLASH_INVALID_ARGUMENT;
		
		rgn++;
	}

	/* After configuration: enable protection and lock it till next TIP reset */
	memset ((void *)&setting, 0, sizeof(FIU_PROTECTION_SETTING_T));
	setting.otherCommandsAllowed = false;
	stat += FIU_ConfigProtection (fiu, true, true, fl_tip_m->size, &setting);

	return stat;
}

