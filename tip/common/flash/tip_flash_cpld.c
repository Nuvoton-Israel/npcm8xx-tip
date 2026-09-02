/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2024 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *   tip_flash_cpld.c
 *            This file contains the flash flow and helper functions
 *  Project:  Arbel
 *------------------------------------------------------------------------*/
#include <string.h>
#include "tip_utils.h"
#include "flash/flash_common.h"
#include "tip_flash.h"
#include "tip_flash_cpld.h"
#include "hal.h"
#include "tip_boot.h"
#include "tip_rom_utils.h"
#include "platform_io.h"
#include "hal_regs.h"
#include "flash/flash_xfer_tip.h"

#ifdef CPLD
// optional:
// #define FIU_CPLD_PRINT_DEBUG
#define FIU_CPLD_PRINT_DEBUG_SHORT

#define FIU_TIMEOUT				  100000
#define CPLD_READ_SIZE     16
#define GPIO_SPI_BMC_CPLD_FPGA_SEL 29

static int tip_flash_cpld_slave_spi_op (uint8_t *wr_data, int wr_count, uint8_t *rd_data, int rd_count)
{
	DEFS_STATUS hal_status;
	uint32_t fiu = FIU_MODULE_1;
	uint32_t cs = FIU_CS_0;
	uint32_t cmdBits = 1;
	uint32_t addrBits = 1;
	uint32_t dataBits = 1;
	uint32_t dummy_bytes = 0;
	int status = 0;
	uint8_t  cmd = 0;

	if (wr_count)
		cmd = wr_data[0];

	if (rd_count) {
		memset (rd_data, 0xAA, rd_count);
	}

	hal_status = FIU_UMA_ioctl (fiu,
		cs,
		cmd,
		0,
		0,
		rd_data,
		rd_count,
		(wr_count > 0) ? (wr_data + 1) : NULL,
		(wr_count > 0) ? (wr_count - 1) : 0,
		cmdBits,
		addrBits,
		dataBits,
		dataBits,
		FIU_TIMEOUT,
		dummy_bytes);

	if (hal_status != DEFS_STATUS_OK) {
		platform_printf ("CPLD XFER failed fiu%d cs%d status %#010lx" NEWLINE, fiu, cs,
			hal_status);
		status = FLASH_MASTER_XFER_FAILED;
	}

#if FIU_CPLD_PRINT_DEBUG
	if (rd_count)
		platform_printf ("read: ");
	for (int i = 0; i < rd_count ; i++) {
		if ((i % 32) == 0)
			platform_printf ("\n");
		platform_printf ("0x%x ", rd_data[i]);
	}
	platform_printf ("\n");
#endif

	return status;
}

static int tip_flash_cpld_ufm_set_addr (unsigned int pageno, unsigned int ufm)	// 14 bits UFM Page address
{
	unsigned int flash_addr;
	unsigned char address_cmd[8] =
	    { 0xB4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned int i;

	if (ufm == UFM3) {
		if (pageno >= UFM3_SIZE)
			return -1;
	} else if (ufm == UFM2) {
		if (pageno >= UFM2_SIZE)
			return -1;
	} else if (ufm == UFM1) {
		if (pageno >= UFM1_SIZE)
			return -1;
	} else if (ufm == UFM0) {
		if (pageno >= UFM0_SIZE)
			return -1;
	} else if (ufm == CFG1) {
		if (pageno >= CFG1_SIZE)
			return -1;
	} else if (ufm == CFG0) {
		if (pageno >= CFG0_SIZE)
			return -1;
	} else {
		return -1;
	}

	flash_addr = (ufm << 14) + pageno;
	/* load address into buffer */
	for (i = 7; i > 4; i--) {
		address_cmd[i] = flash_addr & 0xFF;
		flash_addr = flash_addr >> 8;
	}
	tip_flash_cpld_slave_spi_op (address_cmd, sizeof(address_cmd), NULL, 0);
	return 0;
}

/**
 * Read CPLD flash data via SPI hardened bus.
 *
 * @param rd_data Buffer for read data.
 * @param rd_count Number of bytes to read.
 * @param cfg Range to read. Valid values are CFG0,1,2 UFM0,1,2.
 * @return 0 if success or an error code
 */
int tip_flash_cpld_read (uint8_t *rd_data, int rd_count, unsigned int cfg)
{
	uint8_t enable_cmd[4] = { 0x74, 0x08, 0x00, 0x00};
	uint8_t disable_cmd[3] = {0x26, 0x00, 0x00};
	uint8_t read_cmd[4] = { 0x73, 0x01, 0x3F, 0xFF };
	uint8_t no_op[4] = {0xFF, 0xFF, 0xFF, 0xFF};

	int offset = 0;
	int size = rd_count;
	int buffer_size;

	if (rd_data == NULL) {
		return -1;
	}

	memset(rd_data, 0, rd_count);

	do {
		buffer_size = MIN(size, CPLD_READ_SIZE);
		tip_flash_cpld_slave_spi_op (enable_cmd, sizeof(enable_cmd), NULL, 0);
		while (tip_flash_cpld_busy () == 1);
		tip_flash_cpld_ufm_set_addr (offset, cfg);
		tip_flash_cpld_slave_spi_op (read_cmd, sizeof(read_cmd), rd_data + offset, CPLD_READ_SIZE);
		tip_flash_cpld_slave_spi_op (disable_cmd, sizeof(disable_cmd), NULL, 0);
		tip_flash_cpld_slave_spi_op (no_op, sizeof(no_op), NULL, 0);
		offset += CPLD_READ_SIZE;
		size -= buffer_size;
	} while (size);

#ifdef FIU_CPLD_PRINT_DEBUG_SHORT
	hex_dump ((uint32_t)rd_data, rd_count, "CFG");
#endif

	return 0;
}

/**
 * Read Single CPLD flash page via SPI hardened bus.
 *
 * @param rd_data Buffer for read data.
 * @param rd_count Number of bytes to read.
 * @param cfg Range to read. Valid values are CFG0,1,2 UFM0,1,2.
 * @param page_no Page Index to read.
 * @return 0 if success or an error code
 */
int tip_flash_cpld_read_page (uint8_t *rd_data, int rd_count, unsigned int cfg, int page_no)
{
	uint8_t enable_cmd[4] = { 0x74, 0x08, 0x00, 0x00};
	uint8_t disable_cmd[3] = {0x26, 0x00, 0x00};
	uint8_t read_cmd[4] = { 0x73, 0x01, 0x00, 0x00 };
	uint8_t no_op[4] = {0xFF, 0xFF, 0xFF, 0xFF};
	int status = 0;
	
	if ((rd_data == NULL) || (rd_count > CPLD_READ_SIZE)) {
    	return -1;
	}

	memset(rd_data, 0, rd_count);
	tip_flash_cpld_slave_spi_op (enable_cmd, sizeof(enable_cmd), NULL, 0);
	while (tip_flash_cpld_busy () == 1);
	tip_flash_cpld_ufm_set_addr (page_no, cfg);
	status = tip_flash_cpld_slave_spi_op (read_cmd, sizeof(read_cmd), rd_data, CPLD_READ_SIZE);
	tip_flash_cpld_slave_spi_op (disable_cmd, sizeof(disable_cmd), NULL, 0);
	tip_flash_cpld_slave_spi_op (no_op, sizeof(no_op), NULL, 0);

	return status;
}

/**
 * Read status. Read the four bytes status from CPLD
 *
 * @param rd_data, must be 4 bytes.
 * @return 0 if success or an error code
 */
int tip_flash_cpld_read_status (uint8_t *rd_data)
{
	int rd_count = 4;
	uint8_t read_status[4] = {0x3C, 0x0, 0x0, 0x0};

	if (rd_data == NULL) {
		return -1;
	}

	memset(rd_data, 0, rd_count);

	tip_flash_cpld_slave_spi_op (read_status, sizeof(read_status), rd_data, rd_count);

	return 0;
}

/**
 * Read USERCODE.
 *
 * @param rd_data, must be 4 bytes
 * @return 0 if success or an error code
 * @note if transparent / enable cmd set, reads usercode from CFG0/1
 * 		 otherwise reads from SRAM.
 */
int tip_flash_cpld_read_usercode (uint8_t *rd_data)
{
	int rd_count = 4;
	uint8_t read_usercode[4] = {0xC0, 0x0, 0x0, 0x0};

	if (rd_data == NULL) {
		return -1;
	}

	memset(rd_data, 0, rd_count);

	tip_flash_cpld_slave_spi_op (read_usercode, sizeof(read_usercode), rd_data, rd_count);

	return 0;
}

/**
 * Check if flash is busy.
 *
 * @return 0 if free, 1 if busy.
 */
int tip_flash_cpld_busy (void)
{
	uint8_t status [4];

	tip_flash_cpld_read_status (status);

	/* (bit 8-DONE, bit 12-BUSY, bit 13-FailFlag)   */
	return (0x10 & status[1]);
}

/**
 * Read ID. Value should be the same as the one the Latice Dimond programmer indication.
 *
 * @param rd_data, read ID value, must be 4 bytes.
 * @return 0 if success or an error code
 */
int tip_flash_cpld_read_id (uint8_t *rd_data)
{
	int rd_count = 4;
	uint8_t read_id_cmd[4] = {0xE0, 0x00, 0x00, 0x00};

	if (rd_data == NULL) {
		return -1;
	}

	memset(rd_data, 0, rd_count);

	tip_flash_cpld_slave_spi_op (read_id_cmd, sizeof(read_id_cmd), rd_data, rd_count);

#ifdef FIU_CPLD_PRINT_DEBUG_SHORT
	hex_dump ((uint32_t)rd_data, rd_count, "READ_ID");
#endif

	return 0;
}

/**
 * Set mux value of CPLD for DC-SCM. using GPIO29
 *
 * @param val: 0 for A port, 1 for B port.
 * @return void
 */
void tip_flash_cpld_set_mux (unsigned int val)
{
	GPIO_Init (GPIO_SPI_BMC_CPLD_FPGA_SEL, GPIO_DIR_OUTPUT, GPIO_PULL_NONE, GPIO_OTYPE_PUSH_PULL, false);
	GPIO_Write (GPIO_SPI_BMC_CPLD_FPGA_SEL, val);
}

/**
 * Initialize access to the Lattice CPLD flash for the Cerberus.
 *
 * @param fiu FIU index. FIU is the Flash Interface Unit of the TIP.
 * @param cs CS index. Chip Select. Each FIU has up to 4 Chip selects connected to
 *        up to 4 flashes.
 * @return 0 if success or an error code
 */
int tip_flash_cpld_initialize_access (uint32_t fiu, uint32_t cs)
{
	uint32_t size = 0;

	if ((fiu >= FIU_NUM_OF_MODULES) || (cs >= FLASH_NUM_OF_DEVICES)) {
		platform_printf (KRED "%s out of bounds fiu=%d cs=%d" NEWLINE KNRM, __func__, fiu, cs);
		return FLASH_ADDRESS_OUT_OF_RANGE;
	}

	CHIP_MuxFIU (fiu, true, false, false, false, false);
	FIU_Init (fiu, cs, 0);

	/* Not sure we will use direct at all. */
	FIU_Config4ByteAddress (fiu, cs, cs, FALSE);

	FIU_ConfigDummyCyles (fiu, 0, TRUE);

	/* for Lattice 4300 (EVB): 8KB.  */
	/* for Lattice 9300 (EVB): 32KB. */
	size = _32KB_;

	FIU_ConfigCommand (fiu, 0x08, 0);
	FIU_ConfigDummyCyles (fiu, 8, true);

	/* Reduce FIU1 speed */
	CLK_ConfigureFIUClock(fiu, 250);

	platform_printf (KGRN "%s complete fiu%d cs%d flash size %#010lx" NEWLINE KNRM,
				__func__, fiu, cs, size);

	return 0;
}
#endif  // if CPLD
