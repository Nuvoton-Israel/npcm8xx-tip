// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "common/unused.h"
#include "tip_flash.h"
#include "tip_firmware_component.h"
#include "tip_rom_utils.h"
#include "tip_log.h"
#include "tip_utils.h"
#include "tip_key_manifest.h"
#include "tip_skmt.h"
#include "tip_security.h"
#include "tip_version.h"
#include "platform_io.h"


#define COMPARE_START_TAG(a, b) \
	((*(uint32_t *) a == *(uint32_t *) b) && (*((uint32_t *) a + 1) == *((uint32_t *) b + 1)))

/**
 * start tag of all possible images
 */
static const uint8_t kmt_header_tag[8]                = {0x5E, 0x4D, 0x3B, 0x2A, 0xE1, 0x54, 0xF2, 0x57};
static const uint8_t kmt_header_tag_crc[8]            = {0x5E, 0x4D, 0x3B, 0x2A, 0x1E, 0xAB, 0xF2, 0x57};
static const uint8_t tip_fw_l0_header_tag[8]          = {0x5E, 0x4D, 0x7A, 0x9B, 0xE1, 0x54, 0xF2, 0x57};
static const uint8_t tip_fw_l0_header_tag_crc[8]      = {0x5E, 0x4D, 0x7A, 0x9B, 0x1E, 0xAB, 0xF2, 0x57};
static const uint8_t skmt_header_tag[8]               = {0x73, 0x6B, 0x6D, 0x74, 0x50, 0x08, 0x0D, 0x0A};
static const uint8_t tip_fw_l1_header_tag[8]          = {0x0A, 0x54, 0x49, 0x50, 0x5F, 0x4C, 0x31, 0x0A};
static const uint8_t bb_header_tag[8]                 = {0x0A, 0x50, 0x08, 0x55, 0xAA, 0x54, 0x4F, 0x4F};
static const uint8_t uboot_header_tag[8]              = {0x0A, 0x55, 0x42, 0x4F, 0x4F, 0x54, 0x42, 0x4C};
static const uint8_t bl31_header_tag[8]               = {0x0A, 0x42, 0x4C, 0x33, 0x31, 0x4E, 0x50, 0x43};
static const uint8_t optee_header_tag[8]              = {0x0A, 0x54, 0x45, 0x45, 0x5F, 0x4E, 0x50, 0x43};

/**
 * SVN version handler.
 */
extern struct tip_version_handler tip_version;

/**
 *  Holder for keys that read from OTP.
 */
static uint8_t key_otp[0x60];

/**
 * Get TIP firmware component or image name.
 *
 * @param img_type Image type.
 *
 * @return FW component name.
 */
char *tip_firmware_component_get_name (IMG_TYPE_E img_type)
{
	switch (img_type) {
		case (IMG_KMT):
			return "KMT          ";
		case (IMG_TFT_L0):
			return "TFT_L0       ";
		case (IMG_SKMT):
			return "SKMT         ";
		case (IMG_TFT_L1):
			return "TFT_L1       ";
		case (IMG_BOOTBLOCK):
			return "BOOTBLOCK    ";
		case (IMG_BL31):
			return "BL31         ";
		case (IMG_OPTEE):
			return "OPTEE        ";
		case (IMG_UBOOT):
			return "UBOOT        ";
		case (IMG_COMBO0):
			return "COMBO0       ";
		case (IMG_COMBO1):
			return "COMBO1       ";
		case (IMG_LINUX_KERNEL):
			return "LINUX_KERNEL ";
		case (IMG_LINUX_DTS):
			return "LINUX_DTS    ";
		case (IMG_LINUX_FS):
			return "LINUX_FS     ";
		case (IMG_LINUX_OPENBMC):
			return "LINUX_OPENBMC";
		case (IMG_FULL):
			return "FULL         ";
		default:
			return "UNKNOWN ";
	}
}

/**
 * Get the public key used to verify the single FW component.
 *
 * @param fw TIP firmware component.
 * @param kmt Primary key manifest.
 * @param skmt Secondary key manifest.
 * @param ind Key index used to do lookup.
 * @param [out] pub_key Output buffer to store the public key.
 * @param [out] pub_key_length Length of the public key.
 *
 * @return 0 if success or an error code.
 */
static int tip_firmware_component_get_key (struct tip_firmware_component *fw,
	struct tip_primary_key_manifest *kmt, struct tip_secondary_key_manifest *skmt, int ind,
	uint8_t **pub_key, size_t *pub_key_length)
{
	int status;
	IMG_HEADER_T *ROM_HeaderPtr;

	if (fw == NULL || pub_key == NULL || pub_key_length == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	uint8_t valid_otp[2] = { 0x00, 0x00 };

	/* Boot flow: use KMT\SKMT index which is already loaded.
	 * For components that are loaded by ROM, the pointer is cast to IMG_HEADER_T. */
	if (fw->header_ram) {
		ROM_HeaderPtr = (IMG_HEADER_T *) fw->header_ram;
	} else {
		/* update flow: use KMT\SKMT index from flash. */
		ROM_HeaderPtr = (IMG_HEADER_T *) fw->header_flash;
	}

	fw->key_index = ROM_HeaderPtr->header.key_index & 0x000000FF;

	platform_printf ("\tGet key for (%#010lx) %s:  \t", (uint32_t) ROM_HeaderPtr,
		tip_firmware_component_get_name (fw->img_type));

	if (fw->img_type == IMG_KMT) {
		platform_printf ("\tGet OTP key (%d)\t", fw->key_index);

		FUSE_WRPR_get (oPKnVAL_PROPERTY (fw->key_index), valid_otp);
		if (valid_otp[0] != 0xAA) {
			platform_printf (KRED "\nOTP key %d is invalid" NEWLINE, fw->key_index);
			if (TIP_SECBOOT_IS_ACTIVE ()) {
				return -1;
			}
		}
		FUSE_WRPR_get (oPKn_PROPERTY (fw->key_index), key_otp);
		*pub_key = key_otp;

	} else if (fw->img_type == IMG_TFT_L0 || fw->img_type == IMG_SKMT) {
		if (kmt == NULL || kmt->fw == NULL) {
			platform_printf (KRED "\tKMT not found\t" KNRM);
			return -1;
		}

		if (fw->header_ram) {
			if (!kmt->ram_copy_valid) {
				platform_printf (KRED "Error:RAM KMT invalid" NEWLINE KNRM);
				return -1;
			}
			platform_printf (KMAG "\tGet KMT key (%d) from RAM\t" KNRM, fw->key_index);

			/* NTIL: if key_manifest had index, this would have been nicer... */
			*pub_key = (uint8_t *) kmt->fw->header_ram + sizeof (IMG_HEADER_T) +
					   fw->key_index * 128;
		} else {
			*pub_key = (uint8_t *) kmt->fw->header_flash + sizeof (IMG_HEADER_T) +
					   fw->key_index * 128;
			platform_printf (KMAG "\tGet KMT key (%d) from FLASH\t" KNRM, fw->key_index);
		}

	} else {
		/* SKMT keys: */
		platform_printf ("\tGet SKMT key (%d)\t", fw->key_index);
		status = tip_skmt_key_lookup (skmt, fw->key_index, pub_key, pub_key_length);
		if (status != 0) {
			platform_printf (KRED "SKMT key lookup failed" NEWLINE KNRM);
			return status;
		}
	}

	platform_printf ("public key %d is 0x%x-0x%x-0x%x-..." NEWLINE, fw->key_index,
		(*pub_key)[0], (*pub_key)[1], (*pub_key)[2]);

	return 0;
}

/**
 * Get current security version number (SVN) from the header. Minimum SVN is on OTP
 *
 * @param fw TIP firmware component.
 *
 * @return int Version number
 */
static int tip_firmware_component_get_svn_from_header (struct tip_firmware_component *fw)
{
	uint16_t version = 0;

	if (fw == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	/* If image is loaded to RAM, get from RAM header. Otherwise from flash header. */
	if (fw->header_ram) {
		version = fw->header_ram->header.version;
	} else if (fw->header_flash) {
		version = fw->header_flash->header.version;
	}

	return version;
}

/**
 *  Scan the flash to find the component by searching for start tag. Used by boot flow.
 *
 * @param fw TIP firmware component.
 * @param flash The flash to scan.
 * @param offset The flash offset to start scanning. If it's less than 0, skip scanning.
 *
 * @return 0 if success or an error code.
 */
static int tip_firmware_component_scan_flash (struct tip_firmware_component *fw,
	struct spi_flash *flash, int offset)
{
	struct tip_flash_master *tip_fl;
	const uint8_t *tag = tip_fw_l1_header_tag;
	uint32_t dst_addr;
	uint32_t scan_from;
	uint32_t scan_to;

	if (fw == NULL || flash == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	/* Skips scanning if offset is negative value. */
	if (offset < 0) {
		return 0;
	}

	switch (fw->img_type) {
		case IMG_KMT:
			tag = kmt_header_tag;
			break;
		case IMG_SKMT:
			tag = skmt_header_tag;
			break;
		case IMG_TFT_L0:
			tag = tip_fw_l0_header_tag;
			break;
		case IMG_TFT_L1:
			tag = tip_fw_l1_header_tag;
			break;
		case IMG_BOOTBLOCK:
			tag = bb_header_tag;
			break;
		case IMG_BL31:
			tag = bl31_header_tag;
			break;
		case IMG_OPTEE:
			tag = optee_header_tag;
			break;
		case IMG_UBOOT:
			tag = uboot_header_tag;
			break;
		default:
			return -1;
	}

	tip_fl = (struct tip_flash_master *) flash->spi;

	if (tip_flash_reconfig_address_mode (tip_fl->fiu, tip_fl->cs)) {
		platform_printf (KRED "FIU%d CS%d: flash not found" NEWLINE KNRM, tip_fl->fiu, tip_fl->cs);
		return -1;
	}

	/* check overflow: */
	if (offset >= tip_fl->size) {
		offset = 0;
	}

	scan_from = tip_fl->base_addr + offset;
	scan_to = tip_fl->base_addr + tip_fl->size;

	platform_printf ("tag %c%c%c. scan from %#010lx to %#010lx" NEWLINE, tag[1], tag[2], tag[3],
		scan_from, scan_to);

	/* scan the flash, search for tags. */
	for (uint32_t addr = scan_from; addr < scan_to; addr += 0x1000) {
		if (COMPARE_START_TAG (addr, tag)) {
			fw->header_flash = (HEADER_GENERAL_T *) addr;
			dst_addr = fw->header_flash->header.destAddr;
			fw->base.start_addr = addr - tip_fl->base_addr;

			if (fw->img_type == IMG_KMT || fw->img_type == IMG_TFT_L0) {
				TIP_HEADER_STRUCT_T *rom_header;
				rom_header = (TIP_HEADER_STRUCT_T *) fw->header_flash;
				fw->size = rom_header->img_length + sizeof (TIP_HEADER_STRUCT_T);
				fw->key_index = rom_header->key_index;
			} else {
				fw->size = fw->header_flash->header.codeSize + sizeof (HEADER_GENERAL_T);
				fw->key_index = fw->header_flash->header.KeyIndex;
			}

			platform_printf (KGRN "%s %s found addr %#010lx dst %#010lx " NEWLINE KNRM, __func__,
				tip_firmware_component_get_name (fw->img_type), addr, dst_addr);

			return 0;
		}
	}

	return -1;
}

/**
 * Initialize TIP firmware component handler, search it on flash.
 *
 * @param fw Firmware component handler.
 * @param flash The flash to scan from.
 * @param img_type  L0\L1\SKMT\KMT\BOOTBLOCK\BL31\OPTEE\UBOOT.
 * @param start_offset Start offset in flash to start the scan.
 *
 * @return 0 if success or an error code.
 */
int tip_firmware_component_init (struct tip_firmware_component *fw, struct spi_flash *flash,
	IMG_TYPE_E img_type, int start_offset)
{
	int status;

	if (fw == NULL || flash == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	memset (fw, 0, sizeof (struct tip_firmware_component));

	fw->base.flash = &flash->base;
	fw->img_type = img_type;
	fw->key_index = -1;
	fw->size = 0;

	/* scan for a specific image, if we know what type it is */
	if (img_type != IMG_UNKNOWN && start_offset >= 0) {
		status = tip_firmware_component_scan_flash (fw, flash, start_offset);
		if (status != 0) {
			platform_printf (KRED "no image found on flash" NEWLINE KNRM);
			return status;
		}
	}

	return 0;
}

/**
 * Verify single firmware component against the signature on the header with the public key on
 * otp\kmt\skmt.
 *
 * @param fw Firmware component handler.
 * @param hash Hashing engine to utilize.
 * @param kmt Key manifest.
 * @param skmt Secondary key manifest.
 * @param [out] digest_out Optional out buffer for the digest.
 * @param digest_length Length of the digest.
 *
 * @return 0 if success or an error code.
 */
int tip_firmware_component_verify (struct tip_firmware_component *fw, struct hash_engine *hash,
	struct tip_primary_key_manifest *kmt, struct tip_secondary_key_manifest *skmt,
	uint8_t *digest_out, size_t digest_length)
{
	/* TODO: Add an interface to NCL hardware engine. Currently hardware hash engine is utilized but
	 * without an option to pass it explicity.*/
	UNUSED (hash);

	DEFS_STATUS hal_status = DEFS_STATUS_OK;
	SECURED_BOOLEAN_T sigOK = SECURED_FALSE;
	int status;
	HEADER_GENERAL_T *header;
	uint8_t *imageAddr;
	uint32_t imageLen;
	uint8_t *pub_key;
	size_t pub_key_len;
	uint8_t digest_buf[SHA512_HASH_LENGTH] = { 0 };

	if (fw == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	char *fw_name = tip_firmware_component_get_name (fw->img_type);

	/* if the image is not loaded to RAM, verify directly on flash */
	header = fw->header_ram ? fw->header_ram : fw->header_flash;
	imageAddr = (uint8_t *) header->header.reservedSigned;

	/* signed area starts right after the signature inside the header */
	imageLen = fw->size - 0x70;

	hal_status = tip_image_hash (header, imageAddr, imageLen, digest_buf);
	if (hal_status == DEFS_STATUS_INVALID_DATA_FIELD)
		return FIRMWARE_COMPONENT_INVALID_SIGNATURE;

	if (hal_status != DEFS_STATUS_OK) {
		platform_printf (KRED "hash image fail" NEWLINE KNRM);
		return FIRMWARE_COMPONENT_VERIFY_FAILED;
	}

	/* Get key for the image. */
	status = tip_firmware_component_get_key (fw, kmt, skmt, fw->key_index, &pub_key, &pub_key_len);
	if (status != 0) {
		platform_printf ("fail key, img_type %s " NEWLINE, fw_name);
		return status;
	}

	hal_status = tip_image_auth_ECC (header, imageAddr, imageLen, pub_key, digest_buf, &sigOK);

	/* Erase after use. Use otp key only once */
	if (pub_key == key_otp) {
		memset (key_otp, 0, sizeof (key_otp));
	}

	if (hal_status != DEFS_STATUS_OK) {
		platform_printf (KRED "Error in image, can't authenticate" NEWLINE KNRM);
		return FIRMWARE_COMPONENT_VERIFY_FAILED;
	}

	if (sigOK == SECURED_TRUE) {
		platform_printf (KGRN "%s key %d, SIG PASS" NEWLINE KNRM, fw_name, fw->key_index);

		/* Copy the digest to output buffer that will be used for attestation. */
		if (digest_out != NULL && (digest_length >= sizeof (digest_buf))) {
			memcpy (digest_out, digest_buf, sizeof (digest_buf));
		}

		/* Check SVN against the minimal SVN on OTP. */
		uint16_t version = tip_firmware_component_get_svn_from_header (fw);
		status = tip_version_set_and_check (&tip_version, version, fw->img_type);
		if (status != 0) {
			return FIRMWARE_COMPONENT_VERIFY_FAILED;
		}

		return 0;
	}

	platform_printf (KRED "%s key %d, SIG FAIL" NEWLINE KNRM, fw_name, fw->key_index);
	return FIRMWARE_COMPONENT_BAD_SIGNATURE;
}

/**
 * Load firmware component from flash to memory.
 *
 * @param fw Firmware component handler.
 *
 * @return 0 if success or an error code.
 */
int tip_firmware_component_load_to_memory (struct tip_firmware_component *fw)
{
	uint32_t src_flash_addr = 0;
	uint32_t img_size = -1;
	uint32_t dst_addr = -1;
	uint32_t addr_min = 0;
	uint32_t addr_max = 0xFFFFFFFF;
	uint32_t size_max = _2GB_;

	/* image that is loaded below 16M must be accessed via TIP DRMa window */
	bool dram_window = false;

	if (fw == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	/* Image is already loaded into memory. */
	if (fw->header_ram) {
		return 0;
	}

	dst_addr = fw->header_flash->header.destAddr;
	if (dst_addr < _16MB_) {
		dram_window = true;
	}

	switch (fw->img_type) {
		case IMG_KMT:
		case IMG_SKMT:
		case IMG_TFT_L0:
		case IMG_TFT_L1:
			addr_min = TIP_RAM_BASE_ADDR;
			addr_max = TIP_RAM_BASE_ADDR + TIP_RAM_MEMORY_SIZE - sizeof (TIP_LOG_Arr_T) -
					   sizeof (BOOT_LOG_TABLE_T);
			dram_window = false;
			break;

		case IMG_BOOTBLOCK:
			addr_min = RAM2_BASE_ADDR;
			addr_max = RAM2_BASE_ADDR + RAM2_MEMORY_SIZE;
			break;

		case IMG_BL31:
			addr_min = 0;
			addr_max = RAM2_BASE_ADDR + RAM2_MEMORY_SIZE;
			break;

		case IMG_OPTEE:
			addr_min = 0;
			addr_max = SPI0CS0_BASE_ADDR;
			break;

		case IMG_UBOOT:
			addr_min = 0x7E00;
			addr_max = SPI0CS0_BASE_ADDR;
			break;

		default:
			platform_printf ("%s: Invalid image type" NEWLINE, __func__);
			return -1;
	}

	size_max = addr_max - addr_min;

	img_size = fw->header_flash->header.codeSize + sizeof (HEADER_GENERAL_T);
	src_flash_addr = (uint32_t) (uint8_t *) fw->header_flash;

	if ((img_size < sizeof (HEADER_GENERAL_T)) || (img_size > size_max)) {
		platform_printf (KRED "invalid image size %#010lx" NEWLINE KNRM, img_size);
		return -1;
	}

	if ((dst_addr == 0xFFFFFFFF) || (dst_addr == 0) || (dst_addr < addr_min) ||
		(dst_addr + img_size) > addr_max) {
		platform_printf (KRED "image out of range  %#010lx : %#010lx" NEWLINE KNRM, addr_min,
			addr_max);
		return -1;
	}

	/* bl31 needs its RAM to be cleared in advance if it is loaded to RAM2.*/
	if ((fw->img_type == IMG_BL31) && (dst_addr >= RAM2_BASE_ADDR)){
		platform_printf (KCYN
			"Clear RAM2 to leave some space for BL31 from  %#010lx to  %#010lx" NEWLINE,
			RAM2_BASE_ADDR, 0xfffeef00);
		tip_memset (RAM2_BASE_ADDR, 0, 0xfffeef00 - RAM2_BASE_ADDR, false);
	}

	platform_printf (KMAG NEWLINE "TIP FW: copy fw %#010lx size %#010lx to %#010lx" NEWLINE KNRM,
		src_flash_addr, img_size, dst_addr);

	/* copy the image from flash to RAM */
	tip_memcpy (dst_addr, src_flash_addr, img_size, dram_window, true);

	/* Set header_ram until it is copied to RAM.*/
	fw->header_ram = (HEADER_GENERAL_T *) dst_addr;

	/* reset DRAM window back to start of image, so that header_ram points to header via window */
	if (dram_window == true) {
		uint32_t win_offset = 0;
		if (dst_addr % _64KB_ >  0) {
			win_offset = dst_addr % _64KB_;
		} 
		fw->header_ram = (HEADER_GENERAL_T *) (TIP_DRAM_WIN1_BASE_ADDR + win_offset);
	}

	return 0;
}

/**
 *  Find the firmware component next to current one.
 *
 *  Only used by tip_combo_image in order to find the next component during update flow.
 *  Unlike initial boot, when tip_combo checks the new image to update
 *  which it got from BMC, it doesn't "know" what images are inside.
 *  So it scans them searching for any type of valid image, and return a pointer to
 *  the next image.
 *
 * @param fw Firmware component handler.
 * @param flash Flash handler (can be virtual flash).
 * @param offset Offset in flash.
 *
 * @return 0 if success or an error code.
 */
int tip_firmware_component_get_next_img (struct tip_firmware_component *fw, struct spi_flash *flash,
	uint32_t offset)
{
	struct tip_flash_master *tip_fl;
	uint32_t scan_from;
	uint32_t scan_to;
	/* pointers to start tags. kmt and L0 have two options */
	const char *start_tags[10] = {
		kmt_header_tag,
		kmt_header_tag_crc,
		tip_fw_l0_header_tag,
		tip_fw_l0_header_tag_crc,
		skmt_header_tag,
		tip_fw_l1_header_tag,
		bb_header_tag,
		bl31_header_tag,
		optee_header_tag,
		uboot_header_tag
	};

	/* image types, respectively to start_tags */
	IMG_TYPE_E types[10] = {
		IMG_KMT,
		IMG_KMT,
		IMG_TFT_L0,
		IMG_TFT_L0,
		IMG_SKMT,
		IMG_TFT_L1,
		IMG_BOOTBLOCK,
		IMG_BL31,
		IMG_OPTEE,
		IMG_UBOOT
	};

	if (fw == NULL || flash == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	tip_fl = (struct tip_flash_master *) flash->spi;
	scan_from = tip_fl->base_addr + ROUND_UP (offset, 0x1000);
	scan_to = tip_fl->base_addr + tip_fl->size;

	fw->img_type = IMG_UNKNOWN;

	/* Scan flash in steps of 4KB. Compare to each possible start tag until an image is found. */
	for (uint32_t addr = scan_from; addr < scan_to; addr += 0x1000) {
		/* check each possible start tag */
		for (int i = 0; i < 10; ++i) {
			if (COMPARE_START_TAG (addr, start_tags[i])) {
				fw->header_flash = (HEADER_GENERAL_T *) addr;
				fw->img_type = types[i];
				fw->key_index = fw->header_flash->header.KeyIndex;
				fw->base.start_addr = addr - tip_fl->base_addr;

				if (fw->img_type == IMG_KMT || fw->img_type == IMG_TFT_L0) {
					IMG_HEADER_T *rom_header = (IMG_HEADER_T *) fw->header_flash;
					fw->size = rom_header->header.img_length + sizeof (IMG_HEADER_T);
				} else {
					fw->size = fw->header_flash->header.codeSize + sizeof (HEADER_GENERAL_T);
				}

				return 0;
			}
		}
	}

	return -1;
}
