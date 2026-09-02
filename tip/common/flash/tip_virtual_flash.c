/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/
#include "tip_virtual_flash.h"
#include "tip_utils.h"
#include "flash/flash_common.h"
#include "platform_io.h"


/**
 * Initialize a virtual flash interface and device so it is read for use.
 * This includes:
 * 		- Initializing the SPI flash interface.
 * 		- Set the capacity explicity
 *
 * @param flash The flash interface to initialize.
 * @param spi The SPI master connected to the flash.
 * @param size The size of virtual flash
 * @param fast_read Flag indicating if the FAST_READ command should be used for SPI reads.
 *
 * @return 0 if the SPI flash was successfully initialized or an error code.
 */
static int tip_virtual_flash_initialize_device (struct spi_flash *flash,
												struct spi_flash_state *state,
												const struct flash_master *spi, uint32_t size,
												bool fast_read)
{
	int status;

	if (fast_read) {
		status = spi_flash_init_fast_read (flash, state, spi);
	} else {
		status = spi_flash_init (flash, state, spi);
	}

	if (status != 0) {
		return status;
	}

	/* Virtual flash doesn't support device properties discover from SFDP. So set the capacity explicitly. */
	status = spi_flash_set_device_size (flash, size);
	if (status != 0) {
		return status;
	}

	return 0;
}

/**
 * Xfer implementation for TIP virtual flash.
 *
 * @param spi The SPI master to use to execute the transfer.
 * @param xfer The transfer to execute.
 *
 * @return 0 if the transfer was executed successfully or an error code.
 */
static int tip_flash_virtual_xfer (const struct flash_master *spi, const struct flash_xfer *xfer)
{
	struct tip_flash_master_virtual *virtual_spi = (struct tip_flash_master_virtual *) spi;
	uint32_t erase_length = 0;
	uint32_t phys_addr;

	if (virtual_spi == NULL) {
		return FLASH_MASTER_INVALID_ARGUMENT;
	}

	/* Find virtual flash physical addrres to access. */
	phys_addr = xfer->address + virtual_spi->base_addr;

	platform_printf_dbg ("CMD 0x%lx, addr %#010lx, len %#010lx flags %#010lx" NEWLINE, xfer->cmd,
						 xfer->address, xfer->length, xfer->flags);

	switch (xfer->cmd) {
		case FLASH_CMD_RDSR:
			xfer->data[0] = 0xE0;
			break;

		/* Read cmd processing */
		case FLASH_CMD_READ:
		case FLASH_CMD_FAST_READ:
		case FLASH_CMD_4BYTE_FAST_READ:
		case FLASH_CMD_DUAL_READ:
		case FLASH_CMD_DIO_READ:
		case FLASH_CMD_QUAD_READ:
		case FLASH_CMD_QIO_READ:
		case FLASH_CMD_4BYTE_QIO_READ:
			memcpy (xfer->data, (uint8_t *) phys_addr, xfer->length);
			break;

		/* Write cmd processing */
		case FLASH_CMD_PP:
		case FLASH_CMD_4BYTE_PP:
			memcpy ((uint8_t *) phys_addr, xfer->data, xfer->length);
			break;

		/* Erase cmd processing */
		case FLASH_CMD_4BYTE_64K_ERASE:
		case FLASH_CMD_64K_ERASE:
			erase_length = min (_64KB_, virtual_spi->size - xfer->address);
			memset ((uint8_t *) phys_addr, 0xFFFFFFFF, erase_length);
			break;

		case FLASH_CMD_4BYTE_4K_ERASE:
		case FLASH_CMD_4K_ERASE:
			erase_length = min (_4KB_, virtual_spi->size - xfer->address);
			memset ((uint8_t *) phys_addr, 0xFFFFFFFF, erase_length);
			break;

		case FLASH_CMD_CE:
			erase_length = virtual_spi->size;
			memset ((uint8_t *) virtual_spi->base_addr, 0xFFFFFFFF, erase_length);
			break;

		default:
			break;
	}

	return 0;
}

/**
 * Get a set of capabilities supported by the SPI master.
 *
 * @param spi The SPI master to query.
 * @return A capabilities bitmask for the SPI master.
 */
static uint32_t tip_flash_virtual_capabilities (const struct flash_master *spi)
{
	return FLASH_CAP_3BYTE_ADDR;
}

/**
 * Check if virtual flash address range is valid.
 * That means it is not a real flash.
 * It is DRAM/SRAM but not below 16MB.
 *
 * @param base_addr Base address of virtual flash
 * @param size Virtual flash size
 * @return true if virtual flash is valid
 */
static bool tip_flash_virtual_is_valid (uint32_t base_addr, uint32_t size)
{
	uint32_t addr_low = SPI0CS0_BASE_ADDR;
	uint32_t addr_hi = SPI3CS0_BASE_ADDR + SPI3_MEMORY_SIZE;

	/* check values are not a real flash (can be DRAM\SRAM) but not below 16MB */
	if ((base_addr >= addr_low && base_addr < addr_hi) ||
		((base_addr + size >= addr_low) && (base_addr + size < addr_hi)) || (base_addr < _16MB_)) {
		return false;
	}

	return true;
}

/**
 * Initialize the virtual flash which is DRAM
 *
 * @param flash The flash interface to initialize.
 * @param state Variable context for the flash interface. This must be uninitialized.
 * @param flash_master The SPI master connected to the flash.
 * @param addr The base address of flash device connected to the master
 * @param size The size of flash device connect to the master
 * @return int 0 if successful or an error code
 */
int tip_initialize_flash_access_virtual (struct spi_flash *flash, struct spi_flash_state *state,
										 const struct flash_master *flash_master, uint32_t addr,
										 uint32_t size)
{
	int status;
	struct tip_flash_master_virtual *virtual_spi = (struct tip_flash_master_virtual *) flash_master;

	if (virtual_spi == NULL) {
		return FLASH_MASTER_INVALID_ARGUMENT;
	}

	if (!tip_flash_virtual_is_valid (addr, size)) {
		platform_printf (KRED "virtual flash invalid: addr %#010lx size %#010lx" NEWLINE KNRM, addr,
						 size);
		return FLASH_ADDRESS_OUT_OF_RANGE;
	}

	/* Init virtual flash master */
	virtual_spi->base.xfer = &tip_flash_virtual_xfer;
	virtual_spi->base.capabilities = &tip_flash_virtual_capabilities;
	virtual_spi->base_addr = addr;
	virtual_spi->size = size;

	status = tip_virtual_flash_initialize_device (flash, state, flash_master, size, true);
	if (status != 0) {
		return status;
	}

	return 0;
}
