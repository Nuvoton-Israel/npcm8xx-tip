// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include "tip_utils.h"
#include "tip_log.h"
#include "tip_rom_utils.h"
#include "platform_io.h"
#include "tip_flash.h"
#include "tip_ncl_rom_if.h"
#include "tip_firmware_component.h"
#include "hal.h"
#include "hal_regs.h"


/* Stuff from TIP_ROM */
extern TIP_LOG_Arr_T TIP_LOG_Arr __attribute__ ((section (".log")));

/**
 * Copy data from flash to memory via direct read. Only working on aligned data.
 *
 * @param dst_addr Destination address
 * @param src_flash_addr  The flash data
 * @param src_size The size of data to be copy
 * @param dram If 1 destination address is DRAM
 * @param print If 1 debug prints enabled
 * @return The number of bytes copied
 */
uint32_t tip_memcpy (uint32_t dst_addr, uint32_t src_flash_addr, uint32_t src_size, bool dram,
	bool print)
{
	uint32_t cnt;
	uint32_t block_cnt, block_first;
	uint32_t end_block_offset = 0;
	uint32_t win_offset = 0;
	uint32_t src_cnt = 0;
	uint32_t cs;
	uint32_t fiu;
	uint32_t spi;
	uint32_t offset;
	struct spi_flash *flash;

	/* Align the address. */
	dst_addr &= 0xFFFFFFFC;
	src_flash_addr &= 0xFFFFFFFC;

	/* Align the size. */
	if (src_size & 0x3) {
		src_size += 4 - (src_size & 0x3);
	}

	if (print) {
		platform_printf ("\ncopy     %#010lx ", src_flash_addr);
		platform_printf ("size  %#010lx ", src_size);
		platform_printf ("to    %#010lx " NEWLINE, dst_addr);
	}

	if (tip_flash_phys_to_logical (src_flash_addr, &fiu, &cs, &spi, &offset)) {
		platform_printf ("Invalid src flash addr %#010lx" NEWLINE, src_flash_addr);
		return 0;
	}

	flash = tip_flash_get_spi_flash (spi);
	platform_mutex_lock (&flash->state->lock);

	/* If dest address is DRAM (below 16MB only), can't use direct mapping.
	 * Instead need to use a windows and translate the addresses to the window range. */
	if (dram) {
		block_first = (dst_addr >> 16) & 0x00007FFF;
		block_cnt = block_first;
		win_offset = 0;

		/* handle offset in DRAM window */
		if (dst_addr % _64KB_ >  0) {
			win_offset = dst_addr % _64KB_;
		} 
		/* set window size at maximum and leave it fixed */
		REG_WRITE (DRAM1WINC, 0x03FFF);

		while (src_cnt < src_size) {
			end_block_offset =  src_size - src_cnt + win_offset;
			if (end_block_offset > _64KB_)
				end_block_offset = _64KB_;
		
			/* configure window to the current block */
			REG_WRITE (DRAM1WINA, BUILD_FIELD_VAL (DRAM1WINA_WINEN, 1) | block_cnt);
			if ((end_block_offset - win_offset) > 0) {
				platform_printf ("copy[%d] from %#010lx to %#010lx size %#010lx" NEWLINE, block_cnt,
					src_flash_addr + src_cnt,  (block_cnt << 16) + win_offset, end_block_offset - win_offset);

				for (cnt = win_offset; cnt < end_block_offset; cnt += 4, src_cnt += 4) {
					*(uint32_t *) (TIP_DRAM_WIN1_BASE_ADDR + cnt) =
						*(uint32_t *) (src_flash_addr + src_cnt);
				}
			}
			win_offset = 0; /* first block had offset, next blocks start from zero */
			block_cnt++;
		}

		REG_WRITE (DRAM1WINA, BUILD_FIELD_VAL (DRAM1WINA_WINEN, 1) | 
								BUILD_FIELD_VAL(DRAM1WINA_WINSTART, block_first));

	} else {
		/* direct mapping, assuming GLBLEN is set. */
		for (cnt = 0; cnt < src_size; cnt += 4) {
			*(uint32_t *) (dst_addr + cnt) = *(uint32_t *) (src_flash_addr + cnt);
			if ((cnt % 1024 == 0) && print)
				platform_printf (".");
		}
		if (print) {
			platform_printf (NEWLINE);
		}
	}

	platform_mutex_unlock (&flash->state->lock);

	return src_size;
}

/**
 * Initialize the memory or flash region to a fixed value. Only working on aligned data
 *
 * @param addr Starting address of the memory or flash
 * @param val The value to be set
 * @param size The size of data
 * @param print If 1 debug prints is enabled
 */
void tip_memset (uint32_t addr, uint32_t val, uint32_t size, bool print)
{
	uint32_t cnt;

	/* Align the address */
	addr &= 0xFFFFFFFC;

	if (print) {
		platform_printf ("mem   %#010lx \t", addr);
		platform_printf ("size  %#010lx \t", size);
	}

	for (cnt = 0; cnt < size; cnt += 4) {
		*(uint32_t *) (addr + cnt) = val;
		if ((cnt % 4096 == 0) && print)
			platform_printf ("*");
	}
	if (print) {
		platform_printf (NEWLINE);
	}
}

/**
 * Compare the data in two memory or flash regions. Only working on aligned data
 *
 * @param addr1 Starting address of memory region 1
 * @param addr2 Staring address of memory region 2
 * @param size The size of data to compare
 * @param print If true debug prints enabled
 * @return The number of miscompared bytes
 */
int tip_memcmp (uint32_t addr1, uint32_t addr2, uint32_t size, bool print)
{
	uint32_t cnt;
	uint32_t error = 0;

	/* Align the address */
	addr1 &= 0xFFFFFFFC;
	addr2 &= 0xFFFFFFFC;

	if (print) {
		platform_printf ("memcmp  %#010lx  %#010lx \t", addr1, addr2);
	}

	for (cnt = 0; cnt < size; cnt += 4) {
		if (*(uint32_t *) (addr1 + cnt) != *(uint32_t *) (addr2 + cnt)) {
			error++;
			if (error < 5 && print) {
				platform_printf ("%#010lx=%#010lx, %#010lx=%#010lx" NEWLINE, (addr1 + cnt),
					*(uint32_t *) (addr1 + cnt), (addr2 + cnt), *(uint32_t *) (addr2 + cnt));
			}
		}

		if (cnt % 0x1000000 == 0 && print) {
			platform_printf ("*" NEWLINE);
		}
	}

	if (error && print) {
		platform_printf ("error cnt = %d (print first 5 errors)" NEWLINE, error);
	}

	return error;
}

/**
 * reverse endianess from big\little
 *  @param        data
 *  @param        out - can be NULL for an in-place swap
 *  @param        size
 */
void tip_mem_swap_endianness (void *data, uint8_t *out, uint32_t size)
{
	if (data == NULL) {
		return;
	}

	if (out == NULL) /*in place*/
	{
		uint8_t temp;

		/* change from big endian to little endian*/
		for (uint32_t i = 0; i < size / 2; i++) {
			temp = ((uint8_t *) data)[size - i - 1];
			((uint8_t *) data)[size - i - 1] = ((uint8_t *) data)[i];
			((uint8_t *) data)[i] = temp;
		}
	} else {
		for (uint32_t i = 0; i < size; i++)
			out[i] = ((uint8_t *) data)[size - i - 1];
	}
}

/**
 * Dump the hex of data in memory. Only working on aligned data.
 *
 * @param addr The starting address of data in memory
 * @param size The size of data to dump
 * @param title Name of the data buffer
 */
void hex_dump (uint32_t addr, uint32_t size, char *title)
{
	platform_printf ("\n%s %d bytes", title, size);

	for (uint32_t i = 0; i < size; i += 4) {
		if ((i % 16) == 0) {
			platform_printf ("\n %#08x : ", (addr + i));
		}
		platform_printf ("%#08x ", *(uint32_t *) (addr + i));
	}

	platform_printf (NEWLINE);
}

/**
 *  Check whether TIP is booting from recovery flash
 *
 * @return true if last boot was from recovery image, false otherwise
 */
bool tip_check_recovery_boot (void)
{
	uint32_t next_boot_addr;

	next_boot_addr = (READ_REG_FIELD (TIP_SEARCH_HDR_START, TIP_SEARCH_HDR_ADDR) << 19);

	/* check if boot image came from fiu 0 cs 0 offset 0 */
	return (next_boot_addr > SPI0CS0_BASE_ADDR + _512KB_) ? true : false;
}

/**
 * Upon next reset: select which image will boot next. 
 * By default TIP will load the next image at offset 512KB of currently loaded image.
 * @param  offset: offset in flash to start searching a valid image on the next boot.
 *
 */
void tip_select_next_boot_image (uint32_t offset)
{
	uint32_t fl_addr = SPI0CS0_BASE_ADDR + offset;
	REG_WRITE (TIP_SEARCH_HDR_START,  fl_addr >> 16);
	platform_printf ("Next boot addr %#010lx (TIP_SCR0 %#010lx)" NEWLINE, fl_addr,
		REG_READ (TIP_SEARCH_HDR_START));
}

/**
 * Print status of TIP
 *
 * @param ver the version
 */
void tip_print_status (const char *ver)
{
	uint8_t data[100];
	uint8_t valid;
	uint32_t pkInvalid;
	uint8_t key = 0;
	uint32_t ChipVersion;
	bool recovery_boot;

	recovery_boot = tip_check_recovery_boot ();

	platform_printf (KMAG NEWLINE ">================================================" NEWLINE);
	platform_printf (">  Arbel TIP FW L0 Version %s" NEWLINE, ver);
	platform_printf (">================================================" NEWLINE);
	platform_printf ("Compile time: %s %s " NEWLINE KNRM, __DATE__, __TIME__);

	if (recovery_boot == true) {
		platform_printf (KMAG NEWLINE
                         "************************************************************************" NEWLINE);
		platform_printf ("******                                                            ******" NEWLINE);
		platform_printf ("******   #####  #####   ###    ###  #     # ##### #####  #     #  ******" NEWLINE);
		platform_printf ("******   #    # #      #   #  #   # #     # #     #    #  #   #   ******" NEWLINE);
		platform_printf ("******   #    # #      #      #   # #     # #     #    #   # #    ******" NEWLINE);
		platform_printf ("******   #####  #####  #      #   # #     # ##### #####     #     ******" NEWLINE);
		platform_printf ("******   #    # #      #      #   #  #   #  #     #    #    #     ******" NEWLINE);
		platform_printf ("******   #    # #      #   #  #   #   # #   #     #    #    #     ******" NEWLINE);
		platform_printf ("******   #    # #####   ###    ###     #    ##### #    #    #     ******" NEWLINE);
		platform_printf ("******                                                            ******" NEWLINE);
		platform_printf ("************************************************************************" NEWLINE);

#ifdef PRE_PRODUCTION
		char c;
		int status;

		platform_printf (NEWLINE NEWLINE KRED "WARNING!  RECOVERY BOOT! THIS FW WILL COPY ITSELF \
            INSTEAD OF THE MAIN IMAGE" NEWLINE KNRM);

		for (int timeout = 60; timeout > 0; timeout--) {
			CLK_Delay_Sec (1);
			platform_printf ("START RECOVERY IN %d sec \n\n * to continue without delay press Y. for halt press N * \r\r", timeout);
			status = serial_get_char(&c, false);
			if (status == 0) {
				if ((c == 'y') || (c == 'Y'))
					timeout = 0;
				if ((c == 'n') || (c == 'N'))
					while(1);
			}
        }
#endif

	} else if (TIP_SECBOOT_IS_ACTIVE ()) {
		platform_printf (KGRN NEWLINE
						 "**************************************************" NEWLINE);
		platform_printf ("******                                      ******" NEWLINE);
		platform_printf ("******   ######       #      ###   ###      ******" NEWLINE);
		platform_printf ("******   #     #     # #    #   # #   #     ******" NEWLINE);
		platform_printf ("******   #     #    #   #    #     #        ******" NEWLINE);
		platform_printf ("******   ######    #######    #     #       ******" NEWLINE);
		platform_printf ("******   #         #     #     #     #      ******" NEWLINE);
		platform_printf ("******   #         #     #  #   # #   #     ******" NEWLINE);
		platform_printf ("******   #         #     #   ###   ###      ******" NEWLINE);
		platform_printf ("******                                      ******" NEWLINE);
		platform_printf ("**************************************************" NEWLINE);

	} else {
		platform_printf (KRED NEWLINE
						 "**************************************************" NEWLINE);
		platform_printf ("******                                      ******" NEWLINE);
		platform_printf ("******   ########     #      ###   #        ******" NEWLINE);
		platform_printf ("******   #           # #      #    #        ******" NEWLINE);
		platform_printf ("******   #          #   #     #    #        ******" NEWLINE);
		platform_printf ("******   ########  #######    #    #        ******" NEWLINE);
		platform_printf ("******   #         #     #    #    #        ******" NEWLINE);
		platform_printf ("******   #         #     #    #    #        ******" NEWLINE);
		platform_printf ("******   #         #     #   ###   #######  ******" NEWLINE);
		platform_printf ("******                                      ******" NEWLINE);
		platform_printf ("**************************************************" NEWLINE);
	}

	ChipVersion = CHIP_Get_Version ();
	if (ChipVersion == ARBEL_VERSION_A1)
		platform_printf (KGRN NEWLINE "=========" NEWLINE "Arbel A1" NEWLINE "=========" NEWLINE);
	else if (ChipVersion == ARBEL_VERSION_Z1)
		platform_printf (KGRN NEWLINE "=========" NEWLINE "Arbel Z1" NEWLINE "=========" NEWLINE);
	else if (ChipVersion == ARBEL_VERSION_A2)
		platform_printf (KGRN NEWLINE "=========" NEWLINE "Arbel A2" NEWLINE "=========" NEWLINE);
	else
		platform_printf (KRED "UNKNOWN version" NEWLINE KNRM);

	SET_REG_FIELD (TIP_TST_CTL, TIP_TST_CTL_TST_CTL_LK, 1);

	platform_printf (KYEL NEWLINE "Reset count %d" NEWLINE, TIP_LOG_Arr.TIP_LOG_ResetCounter);
	platform_printf (KYEL "Last BMC Reset cnt %#010lx" NEWLINE,
		TIP_LOG_Arr.TIP_LOG_LastBmcReset);
	platform_printf (KYEL "Last TIP Reset cnt %#010lx" NEWLINE,
		TIP_LOG_Arr.TIP_LOG_LastTipReset);
	platform_printf (KYEL "Production Failure %#010lx" NEWLINE,
		TIP_LOG_Arr.TIP_LOG_ProductionFailure);

	if (READ_REG_FIELD (TIP_DBG_CTL, TIP_DBG_CTL_TIP_JEN) == TIP_SEC_TRUE) {
		platform_printf (KMAG "TIP JTAG is enabled, %slocked" NEWLINE,
			READ_REG_FIELD (TIP_TST_CTL, TIP_TST_CTL_TST_CTL_LK) ? " " : "un-");
	} else {
		platform_printf (KMAG "TIP JTAG is disabled, %slocked" NEWLINE,
			READ_REG_FIELD (TIP_TST_CTL, TIP_TST_CTL_TST_CTL_LK) ? " " : "un-");
	}

	if (READ_REG_FIELD (TIP_DBG_CTL, TIP_DBG_CTL_BMC_JEN) == TIP_SEC_TRUE) {
		platform_printf (KMAG "BMC JTAG is enabled" NEWLINE);
	} else {
		platform_printf (KMAG "BMC JTAG is disabled" NEWLINE);
	}

	FUSE_WRPR_get (FUSTRAP2_PROPERTY, (uint8_t *) &pkInvalid);
	for (key = 0; key < 8; key++) {
		FUSE_WRPR_get (oPKn_PROPERTY (key), data);
		FUSE_WRPR_get (oPKnVAL_PROPERTY (key), &valid);

		platform_printf (KNRM "public key %d is 0x%x-0x%x-0x%x-... ,  key is %s valid" NEWLINE, key,
			data[0], data[1], data[2],
			((valid == 0xAA) && (READ_VAR_BIT (pkInvalid, key) == 0)) ? " " : "not");
	}
	platform_printf ("TIP will run next image from address %#010lx " NEWLINE,
		READ_REG_FIELD (TIP_SEARCH_HDR_START, TIP_SEARCH_HDR_ADDR) << 19);

	platform_printf ("SPIX is %s " NEWLINE,
		READ_REG_FIELD (MFSEL4, MFSEL4_SPXSEL) ? "enabled" : "disabled");

	FUSE_WRPR_get (OEM_IDENTIFIER_CODE, data);

	platform_printf ("OEM identifier is %d, ", data[0]);

	switch (data[0]) {
		case 1:
			platform_printf ("Dell" NEWLINE);
			break;
		case 2:
			platform_printf ("MS" NEWLINE);
			break;
		case 3:
			platform_printf ("Google" NEWLINE);
			break;
		case 4:
			platform_printf ("Oracle" NEWLINE);
			break;
		case 5:
			platform_printf ("Nvidia" NEWLINE);
			break;
		case 6:
			platform_printf ("Amazon" NEWLINE);
			break;
		case 7:
			platform_printf ("Meta" NEWLINE);
			break;
		case 8:
			platform_printf ("Rivos" NEWLINE);
			break;
		case 9:
			platform_printf ("xFusion" NEWLINE);
			break;
		case 11:
			platform_printf ("AMI" NEWLINE);
			break;
		case 12:
			platform_printf ("IBM" NEWLINE);
			break;
		case 13:
			platform_printf ("Renesas" NEWLINE);
			break;
		case 14:
			platform_printf ("AMD" NEWLINE);
			break;
		case 15:
			platform_printf ("ARM" NEWLINE);
			break;
		case 16:
			platform_printf ("Fortinet" NEWLINE);
			break;
		case 19:
			platform_printf ("Bootlin" NEWLINE);
			break;
		case 21:
			platform_printf ("Quanta" NEWLINE);
			break;
		case 22:
			platform_printf ("Lenovo" NEWLINE);
			break;
		case 23:
			platform_printf ("WiWynn" NEWLINE);
			break;
		case 24:
			platform_printf ("Foxconn" NEWLINE);
			break;
		case 25:
			platform_printf ("Wistron" NEWLINE);
			break;
		case 26:
			platform_printf ("Inventec" NEWLINE);
			break;
		default:
			platform_printf ("Nuvoton" NEWLINE);
			break;
	}




	FUSE_WRPR_get (DIE_LOCATION_PROPERTY, data);

	hex_dump ((uint32_t) data, 4, "UUID");
}
