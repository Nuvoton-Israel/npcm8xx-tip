/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *    			tip_version.c
 *            This file contains version handling  manager
 * Project:
 *            Arbel
 *------------------------------------------------------------------------*/

#include "tip_version.h"
#include "platform_io.h"
#include "tip_rom_utils.h"


/**
 * Get the OTP version for firmware image
 *
 * @param img FW image type
 * @return OTP version
 */
uint16_t tip_get_otp_version (IMG_TYPE_E img)
{
	uint16_t addr, length, offset;
	uint16_t version;
	uint16_t bits_per_version;
	uint8_t bits_mask = 0x80;

	switch (img) {
		case IMG_KMT:
			addr = FUSE_WRPR_PROP_ADDRESS (KMT_VERSION_PROPERTY);
			length = FUSE_WRPR_PROP_SIZE (KMT_VERSION_PROPERTY);
			bits_per_version = 2;
			bits_mask = 0xC0;
			break;

		case IMG_TFT_L0:
			addr = FUSE_WRPR_PROP_ADDRESS (TFT_VERSION_PROPERTY);
			length = FUSE_WRPR_PROP_SIZE (TFT_VERSION_PROPERTY);
			bits_per_version = 2;
			bits_mask = 0xC0;
			break;

		case IMG_SKMT:
			addr = FUSE_WRPR_PROP_ADDRESS (SKMT_VERSION_PROPERTY);
			length = FUSE_WRPR_PROP_SIZE (SKMT_VERSION_PROPERTY);
			bits_per_version = 2;
			break;

		case IMG_TFT_L1:
			addr = FUSE_WRPR_PROP_ADDRESS (TIP_FW_L1_VERSION_PROPERTY);
			length = FUSE_WRPR_PROP_SIZE (TIP_FW_L1_VERSION_PROPERTY);
			bits_per_version = 2;
			break;

		case IMG_BOOTBLOCK:
		case IMG_BL31:
		case IMG_OPTEE:
		case IMG_UBOOT:
		case IMG_COMBO1:
			addr = FUSE_WRPR_PROP_ADDRESS (BMC_VERSION_PROPERTY);
			length = FUSE_WRPR_PROP_SIZE (BMC_VERSION_PROPERTY);
			bits_per_version = 2;
			break;

		default:
			return 0xFFFF;
	}

	/* Read version number from OTP. Note: KMT and L0 have CRC check. All other images have bit per
	 * version. It's the maximal version. */
	version = length * 8 / bits_per_version;

	/* As the FW version is stored in the otp as a monotonic counter, this function finds
	   the index of the first '1' encoded bit starting from offset the end in otp version */
	for (offset = addr + length - 1; offset >= addr; offset--) {
		uint8_t byte = 0xFF;
		uint8_t i;

		CHIP_SecureFuseFieldRead (offset, 1, FUSE_ECC_NONE, &byte);

		for (i = 0; i < (8 / bits_per_version); i++) {
			if ((byte & bits_mask) != 0) {
				return version;
			}
			version--;
			byte <<= bits_per_version;
		}
	}

	platform_printf ("%s %s otp version is %#010lx" NEWLINE, __func__,
		tip_firmware_component_get_name (img), version);

	return version;
}

/**
 * Initialize TIP SVN handler
 *
 * @param ver Version handler instance
 * @return 0 if success or failure -1
 */
int tip_version_init (struct tip_version_handler *ver)
{
	if (ver == NULL) {
		return -1;
	}

	memset ((void *) ver, 0, sizeof (struct tip_version_handler));

	/* All version are bad until proven.*/
	ver->kmt_version = 0;
	ver->kmt_version_minimal_in_otp = tip_get_otp_version (IMG_KMT);
	ver->kmt_version_next = 0;

	ver->tip_fw_l0_version = 0;
	ver->tip_fw_l0_version_minimal_in_otp = tip_get_otp_version (IMG_TFT_L0);
	ver->tip_fw_l0_version_next = 0;

	ver->skmt_version = 0;
	ver->skmt_version_minimal_in_otp = tip_get_otp_version (IMG_SKMT);
	ver->skmt_version_next = 0;

	ver->tip_fw_l1_version = 0;
	ver->tip_fw_l1_version_minimal_in_otp = tip_get_otp_version (IMG_TFT_L1);
	ver->tip_fw_l1_version_next = 0;

	ver->bmc_version = 0;
	ver->bmc_version_minimal_in_otp = tip_get_otp_version (IMG_COMBO1);
	ver->bmc_version_next = 0;

	/* TODO: lock OTP at some point? */
	return 0;
}

/**
 *  check SVN number
 *
 * @param Version handler
 * @param version_num  Version number to set
 * @param next_minimal_otp_version
 * @return 0 if success or an error code
 */
int tip_version_check (struct tip_version_handler *ver, uint16_t current_fw_version,
	uint16_t next_minimal_otp_version, IMG_TYPE_E img)
{
	uint16_t minimal_version_in_OTP = 0x0;
	if (ver == NULL) {
		return -1;
	}

	switch (img) {
		case IMG_KMT:
			minimal_version_in_OTP = ver->kmt_version_minimal_in_otp;
			break;

		case IMG_TFT_L0:
			minimal_version_in_OTP = ver->tip_fw_l0_version_minimal_in_otp;
			break;

		case IMG_SKMT:
			minimal_version_in_OTP = ver->skmt_version_minimal_in_otp;
			break;

		case IMG_TFT_L1:
			minimal_version_in_OTP = ver->tip_fw_l1_version_minimal_in_otp;
			break;

		case IMG_BOOTBLOCK:
			minimal_version_in_OTP = ver->bmc_version_minimal_in_otp;
			break;

		default:
			return 0;
	}

	platform_printf (
		"%s %s %s version check %#010lx, minimal allowed version is %#010lx" NEWLINE KNRM, __func__,
		tip_firmware_component_get_name (img),
		(current_fw_version < minimal_version_in_OTP ? KRED "FAIL" : KGRN "PASS"),
		current_fw_version, minimal_version_in_OTP);


	/*If the next OTP version number is greater than the version of currently running image , do not
	 * program*/
	if (next_minimal_otp_version > current_fw_version) {
		platform_printf (KRED
			"next OTP version number %#010lx is greater than the currently running image %#010lx\n" KNRM,
			next_minimal_otp_version, current_fw_version);
		return -1;
	}
	if (next_minimal_otp_version < minimal_version_in_OTP) {
		platform_printf (KRED
			"next_minimal_otp_version %#010lx is smaller or equal to the next OTP version number  %#010lx \n" KNRM,
			next_minimal_otp_version, minimal_version_in_OTP);
		return -1;
	}
	return (current_fw_version < minimal_version_in_OTP ? -1 : 0);
}

/**
 *  set SVN number
 *
 * @param Version handler
 * @param current_fw_version  Version number to set
 * @param next_minimal_otp_version
 * @param img the IMG_TYPE_E
 * @return 0 if success or an error code
 */
int tip_version_set (struct tip_version_handler *ver, uint16_t current_fw_version,
	uint16_t next_minimal_otp_version, IMG_TYPE_E img)
{
	if (ver == NULL) {
		return -1;
	}
	
	switch (img) {
		case IMG_KMT:
			ver->kmt_version = current_fw_version;
			ver->kmt_version_next = next_minimal_otp_version;
			break;

		case IMG_TFT_L0:
			ver->tip_fw_l0_version = current_fw_version;
			ver->tip_fw_l0_version_next = next_minimal_otp_version;
			break;

		case IMG_SKMT:
			ver->skmt_version = current_fw_version;
			ver->skmt_version_next = next_minimal_otp_version;
			break;

		case IMG_TFT_L1:
			ver->tip_fw_l1_version = current_fw_version;
			ver->tip_fw_l1_version_next = next_minimal_otp_version;
			break;

		case IMG_BOOTBLOCK:
			ver->bmc_version = current_fw_version;
			ver->bmc_version_next = next_minimal_otp_version;
			break;

		 default:
  			  break;
	}
	
	return 0;
}


/**
 * Set SVN number on OTP for ALL images
 *
 * @param Version handler
 * @return 0 if success or an error code
 */
int tip_version_set_in_OTP (struct tip_version_handler *ver)
{
	uint16_t minimal_version_in_OTP = 0xFFFF;
	uint16_t next_minimal_version = 0xFFFF;
	uint16_t current_fw_version = 0xFFFF;
	uint16_t address;
	if (ver == NULL) {
		return -1;
	}
	for (IMG_TYPE_E img = IMG_KMT; img <= IMG_UBOOT; img = img << 1) {
		
		switch (img) {
			case IMG_KMT:
				minimal_version_in_OTP = ver->kmt_version_minimal_in_otp;
				next_minimal_version = ver->kmt_version_next;
				current_fw_version = ver->kmt_version;
				address = FUSE_WRPR_PROP_ADDRESS (KMT_VERSION_PROPERTY);
				break;

			case IMG_TFT_L0:
				minimal_version_in_OTP = ver->tip_fw_l0_version_minimal_in_otp;
				next_minimal_version = ver->tip_fw_l0_version_next;
				current_fw_version = ver->tip_fw_l0_version;
				address = FUSE_WRPR_PROP_ADDRESS (TFT_VERSION_PROPERTY);
				break;

			case IMG_SKMT:
				minimal_version_in_OTP = ver->skmt_version_minimal_in_otp;
				next_minimal_version = ver->skmt_version_next;
				current_fw_version = ver->skmt_version;
				address = FUSE_WRPR_PROP_ADDRESS (SKMT_VERSION_PROPERTY);
				break;

			case IMG_TFT_L1:
				minimal_version_in_OTP = ver->tip_fw_l1_version_minimal_in_otp;
				next_minimal_version = ver->tip_fw_l1_version_next;
				current_fw_version = ver->tip_fw_l1_version;
				address = FUSE_WRPR_PROP_ADDRESS (TIP_FW_L1_VERSION_PROPERTY);
				break;

			case IMG_BOOTBLOCK:
				minimal_version_in_OTP = ver->bmc_version_minimal_in_otp;
				next_minimal_version = ver->bmc_version_next;
				current_fw_version = ver->bmc_version;
				address = FUSE_WRPR_PROP_ADDRESS (BMC_VERSION_PROPERTY);
				break;

			 default:
      			  break;
		}

		/*if the next OTP version number is greater than the version of currently running image , do not
		 * program*/
		if (next_minimal_version <= current_fw_version &&
			next_minimal_version > minimal_version_in_OTP) {
			platform_printf ("will program %#010lx to address %d\n", next_minimal_version, address);
#if FUSE_OTP
			/* enable FCFG to fuse to OTP */
			uint8_t block = FUSE_WRPR_PROP_BLOCK_ADDR (address);
			uint8_t fcfg_val = READ_REG_FIELD (FCFG0_7 (block), FCFG0_7_FPRGDIS);
			SET_REG_FIELD (FCFG0_7 (block), FCFG0_7_FPRGDIS, fcfg_val & 0xFE);
			/*
			 * example : next_minimal_otp_version = 05 00
			 * we should program 03 ff , but actually we only need to program bits 9 and 8 = (5-1)*2,
			 *and the follow
			 */
			uint16_t bitOffset = (next_minimal_version - 1) * 2;
			FUSE_ProgramBit (0, address + (bitOffset / 8), (bitOffset % 8));
			FUSE_ProgramBit (0, address + (bitOffset / 8), (bitOffset % 8) + 1);
			/* disable FCFG once done */
			SET_REG_FIELD (FCFG0_7 (block), FCFG0_7_FPRGDIS, fcfg_val);
			
#endif
		}
	}
	return 0;
}
