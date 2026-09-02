/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef TIP_FIRMWARE_COMPONENT_H_
#define TIP_FIRMWARE_COMPONENT_H_

#include "firmware/firmware_component.h"
#include "tip_boot.h"
#include "crypto/ecc_hw.h"


/* LMS related sizes */
#define LMS_SIG_SIZE_H_20_W_1 0x23CC
#define LMS_SIG_SIZE_H_20_W_2 0x134C
#define LMS_SIG_SIZE_H_20_W_4 0xB0C
#define LMS_SIG_SIZE_H_20_W_8 0x6EC

#define LMS_SIG_SIZE LMS_SIG_SIZE_H_20_W_4

/**
 * TIP FW image type
 */
typedef enum {
	IMG_UNKNOWN = 0,
	IMG_KMT = 0x01,
	IMG_TFT_L0 = 0x02,
	IMG_SKMT = 0x04,
	IMG_TFT_L1 = 0x08,
	IMG_BOOTBLOCK = 0x10,
	IMG_BL31 = 0x20,
	IMG_OPTEE = 0x40,
	IMG_UBOOT = 0x80,
	IMG_COMBO0 = 0x0F,	/**< Combo 0 includes KMT/L0/SKMT/L1. Used by FW update flow. */
	IMG_COMBO1 = 0xF0,	/**< Combo 1 includes BB/BL31/OPTEE/UBOOT. Used by update flow. */
	IMG_COMBO2 = 0xFF,  /**< Combo 2 includes KMT/L0/SKMT/L1/BB/BL31/OPTEE/UBOOT. Used by update flow. */
	IMG_LINUX_KERNEL,
	IMG_LINUX_DTS,
	IMG_LINUX_FS,
	IMG_LINUX_OPENBMC,
	IMG_FULL,
} IMG_TYPE_E;

/**
 * Definition of a single TIP firmware component
 */
struct tip_firmware_component {
	struct firmware_component base; /**< Firmware component base instance. */
	HEADER_GENERAL_T *header_flash; /**< The pointer of original header on the flash. */
	HEADER_GENERAL_T *header_ram;	/**< The pointer of header in RAM when it's loaded to memory.*/
	HEADER_GENERAL_T *header_decrypt_staging;	/**< The pointer of header in RAM when a staging area is required - for decrpyted flash */
	uint32_t size;					/**< Firmware component size including the header. */
	IMG_TYPE_E img_type;			/**< Firmware component type. */
	uint8_t key_index;				/**< Key index inside OTP or KMT or SKMT. Will be used for public key lookup. */
	bool is_lms_sig_exists;			/**< indicates that the image contains the lms signature in the footer  */
};

#define TIP_PER_DEVICE_AES_KEY 3
#define TIP_PER_DEVICE_DECRYPT_KEY 4
#define TIP_GLOBAL_DECRYPT_KEY 5

struct tip_primary_key_manifest;
struct tip_secondary_key_manifest;
struct spi_flash;

/* Use this define to call tip_firmware_component_init without scanning */
#define SKIP_SCAN 1

#define TIP_FIRMWARE_MAX_HARDENING_REGISTERS_COUNT 100

char *tip_firmware_component_get_name (IMG_TYPE_E img_type);
int tip_firmware_component_get_index (IMG_TYPE_E img_type);

int tip_firmware_component_init (struct tip_firmware_component *fw, struct spi_flash *flash,
	IMG_TYPE_E img_type, uint32_t start_offset, bool scan_flash);

int tip_firmware_component_load_to_memory (struct tip_firmware_component *fw);
int tip_firmware_component_verify (struct tip_firmware_component *fw,
	const struct hash_engine *hash, struct tip_primary_key_manifest *kmt,
	struct tip_secondary_key_manifest *skmt, uint8_t *digest_out, size_t digest_length,
	const struct ecc_hw *ecc);

int tip_firmware_component_get_next_img (struct tip_firmware_component *fw, struct spi_flash *flash,
	uint32_t offset);
int tip_firmware_component_load_external_reg_table (struct tip_firmware_component *fw, uint16_t reset);
bool tip_firmware_component_check_external_reg_table (struct tip_firmware_component *fw);
bool tip_is_kmt_and_LMS_is_forced_in_otp (struct tip_firmware_component *fw);


#endif /* TIP_FIRMWARE_COMPONENT_H_ */
