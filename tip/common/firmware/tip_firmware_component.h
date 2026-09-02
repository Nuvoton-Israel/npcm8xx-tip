// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_FIRMWARE_COMPONENT_H_
#define TIP_FIRMWARE_COMPONENT_H_

#include "firmware/firmware_component.h"
#include "tip_boot.h"


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
	uint32_t size;					/**< Firmware component size including the header. */
	IMG_TYPE_E img_type;			/**< Firmware component type. */
	uint8_t key_index;				/**< Key index inside OTP or KMT or SKMT. Will be used for public key lookup. */
};

struct tip_primary_key_manifest;
struct tip_secondary_key_manifest;
struct spi_flash;

char *tip_firmware_component_get_name (IMG_TYPE_E img_type);
int tip_firmware_component_init (struct tip_firmware_component *fw, struct spi_flash *flash,
	IMG_TYPE_E img_type, int start_offset);
int tip_firmware_component_load_to_memory (struct tip_firmware_component *fw);
int tip_firmware_component_verify (struct tip_firmware_component *fw, struct hash_engine *hash,
	struct tip_primary_key_manifest *kmt, struct tip_secondary_key_manifest *skmt,
	uint8_t *digest_out, size_t digest_length);
int tip_firmware_component_get_next_img (struct tip_firmware_component *fw, struct spi_flash *flash,
	uint32_t offset);

/**
 * Error codes that can be generated when accessing a firmware component.
 */
enum {
	FIRMWARE_COMPONENT_VERIFY_FAILED = FIRMWARE_COMPONENT_ERROR (0x10),
	FIRMWARE_COMPONENT_BAD_SIGNATURE = FIRMWARE_COMPONENT_ERROR (0x11),
	FIRMWARE_COMPONENT_INVALID_SIGNATURE = FIRMWARE_COMPONENT_ERROR (0x12),
};


#endif /* TIP_FIRMWARE_COMPONENT_H_ */
