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

	platform_printf ("%s %s otp version %#010lx" NEWLINE, __func__,
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

	/* All version are bad until proven.*/
	ver->kmt_version = 0;
	ver->kmt_version_minimal = tip_get_otp_version (IMG_KMT);
	ver->tip_fw_l0_version = 0;
	ver->tip_fw_l0_version_minimal = tip_get_otp_version (IMG_TFT_L0);
	ver->skmt_version = 0;
	ver->skmt_version_minimal = tip_get_otp_version (IMG_SKMT);
	ver->tip_fw_l1_version = 0;
	ver->tip_fw_l1_version_minimal = tip_get_otp_version (IMG_TFT_L1);
	ver->bmc_version = 0;
	ver->bmc_version_minimal = tip_get_otp_version (IMG_COMBO1);

	/* TODO: lock OTP at some point? */

	return 0;
}

/**
 * Set and check SVN number
 *
 * @param Version handler
 * @param version_num  Version number to set
 * @param img FW image type
 * @return 0 if success or an error code
 */
int tip_version_set_and_check (struct tip_version_handler *ver, uint16_t version_num,
	IMG_TYPE_E img)
{
	uint16_t minimal_version = 0xFFFF;
	if (ver == NULL) {
		return -1;
	}

	switch (img) {
		case IMG_KMT:
			ver->kmt_version = version_num;
			minimal_version = ver->kmt_version_minimal;
			break;

		case IMG_TFT_L0:
			ver->tip_fw_l0_version = version_num;
			minimal_version = ver->tip_fw_l0_version_minimal;
			break;

		case IMG_SKMT:
			ver->skmt_version = version_num;
			minimal_version = ver->skmt_version_minimal;
			break;

		case IMG_TFT_L1:
			ver->tip_fw_l1_version = version_num;
			minimal_version = ver->tip_fw_l1_version_minimal;
			break;

		case IMG_BOOTBLOCK:
		case IMG_BL31:
		case IMG_OPTEE:
		case IMG_UBOOT:
		case IMG_COMBO1:
			ver->bmc_version = version_num;
			minimal_version = ver->bmc_version_minimal;
			break;

		default:
			return -1;
	}

	platform_printf ("%s %s %s version check %#010lx, minimal allowed version is %#010lx"
		NEWLINE KNRM, __func__, tip_firmware_component_get_name (img),
		(version_num < minimal_version ? KRED "FAIL" : KGRN "PASS"), version_num, minimal_version);

	return (version_num < minimal_version ? -1 : 0);
}
