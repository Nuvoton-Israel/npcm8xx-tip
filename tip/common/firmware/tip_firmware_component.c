// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
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
#include "tip_aes_ncl.h"
#include "rot_memory_map.h"
#include "tip_virtual_flash.h"


#define COMPARE_START_TAG(a, b) \
	((*(uint32_t*) a == *(uint32_t*) b) && (*((uint32_t*) a + 1) == *((uint32_t*) b + 1)))

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
 * TIP L1 header handler for flag SystemControlFlags (control in IGPS, TipFwAndHeader_L1.xml)
 */
struct tip_L1_system_control tip_L1_sys_ctrl;
struct tip_L0_system_control tip_L0_sys_ctrl;

/**
 * OTP data to pull once
 */
extern uint8_t fustrap1_lms_enabeld;

/**
 * SVN version handler.
 */
extern struct tip_version_handler *tip_version;

/**
 * Virtual flash as staging area for FW update
 */
extern struct spi_flash virtual_flash;

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
		case (IMG_COMBO2):
			return "COMBO_ALL    ";
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
 * Get TIP firmware component index.
 *
 * @param img_type Image type.
 *
 * @return component index on success or an error code.
 */
int tip_firmware_component_get_index (IMG_TYPE_E img_type)
{
	int index = 0;

	if (img_type < IMG_KMT || img_type > IMG_UBOOT) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	while ((img_type & 0x1) == 0) {
		img_type >>= 1;
		index += 1;
	}

	return index;
}

/**
This function checks if the LMS force value is set on the LMS key valid of the OTP for the selected key.
**/
bool tip_is_kmt_and_LMS_is_forced_in_otp (struct tip_firmware_component *fw)
{
	if ( fw->img_type == IMG_KMT ) {
			uint8_t valid_otp = 0;
			HEADER_GENERAL_T * header = fw->header_ram ? fw->header_ram : fw->header_flash;
			FUSE_WRPR_get (oLMS_PKnVAL_PROPERTY (header->header.KeyIndex_lms), &valid_otp);
			if (valid_otp == 0xAA)
				return TRUE;
			else
				return FALSE;
	}
	return FALSE;
}

/**
 * Get public key count number per image type
 * For KMT image there are 9 public keys.
 * for L0  - according to KMT size.
 * for all the rest according to SKMT size.
 *
 * @param fw TIP firmware component.
 * @param kmt Primary key manifest.
 * @param skmt Secondary key manifest.
 *
 * @return 0 if success or an error code.
 */
static int tip_firmware_component_get_key_count (struct tip_firmware_component *fw,
	struct tip_primary_key_manifest *kmt, struct tip_secondary_key_manifest *skmt)
{
	switch (fw->img_type) {
		case IMG_KMT:
			/* KMT is signed with OTP keys */
			return FUSE_WRAPPER_NUM_OF_ECC_KEYS;

		case IMG_TFT_L0:
		case IMG_SKMT:
			/* L0 signed with a KMT key */
			return MIN (kmt->num_of_kmt_keys, KMT_MAX_KEY_NUM);

		default:
			/* All other image types signed with SKMT keys */
			return MIN (skmt->num_of_keys, SKMT_MAX_KEY_NUM);
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
 * @param is_ECC_key type of request key is ECC.
 * @param is_LMS_key type of requested key is LMS
 *
 * @return 0 if success or an error code.
 */
static int tip_firmware_component_get_key (struct tip_firmware_component *fw,
	struct tip_primary_key_manifest *kmt, struct tip_secondary_key_manifest *skmt, int ind,
	uint8_t **pub_key, int pub_key_length, bool is_ECC_key, bool is_LMS_key)
{
	int status;
	IMG_HEADER_T *ROM_HeaderPtr;
	size_t skmt_pub_key_length;
	int offset_to_add_in_KMT = 0, index_to_read_key = 0;
	UINT32 i;
	UINT32 key_actual_length;
	bool key_all_zeros, key_all_ones;

	if (fw == NULL || pub_key == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	uint8_t valid_otp = 0;
	uint8_t fustrap2[4];

	/* Boot flow: use KMT\SKMT index which is already loaded.
	 * For components that are loaded by ROM, the pointer is cast to IMG_HEADER_T. */
	if (fw->header_ram) {
		ROM_HeaderPtr = (IMG_HEADER_T *) fw->header_ram;
	}
	else {
		/* update flow: use KMT\SKMT index from flash. */
		ROM_HeaderPtr = (IMG_HEADER_T *) fw->header_flash;
	}

#ifndef LMS_ENABLE
	is_ECC_key = true;
	is_LMS_key = false;
#endif

	fw->key_index = ind & 0x000000FF;

	platform_printf ("\tGet key for (%#010lx) %s:  \t", (uint32_t) ROM_HeaderPtr,
		tip_firmware_component_get_name (fw->img_type));

	/* Handle OTP keys */
	if (fw->img_type == IMG_KMT) {
		platform_printf ("\tGet OTP key (%d)\t", ind);

		/* check the valid byte */
		if (is_ECC_key) {
			FUSE_WRPR_get (oPKnVAL_PROPERTY (ind), &valid_otp);
			FUSE_WRPR_get (oPKn_PROPERTY (ind), key_otp);
		}
#ifdef LMS_ENABLE
		else if (is_LMS_key) {
			FUSE_WRPR_get (oLMS_PKnVAL_PROPERTY (ind), &valid_otp);
			FUSE_WRPR_get (oLMS_PKn_PROPERTY (ind), key_otp);
			index_to_read_key = 8;
		}
#endif
		if (is_ECC_key){
			if (valid_otp != 0xAA) {
				platform_printf (KRED "OTP key %d is invalid" NEWLINE KNRM, ind);
				if (TIP_SECBOOT_IS_ACTIVE ()) {
					platform_printf (KRED "Secure boot enabled but OTP key invalid" NEWLINE KNRM);
					return KEY_MANIFEST_REVOKED;
				}
			}

			FUSE_WRPR_get (FUSTRAP2_PROPERTY, fustrap2);
			if (READ_VAR_BIT(*(uint32_t *)fustrap2, ind) == 1) {
				platform_printf ("OTP key%d is invalid. fustrap2 = %#010lx" NEWLINE, ind, (uint32_t *)fustrap2 );
				if (TIP_SECBOOT_IS_ACTIVE ()) {
					return -1;
				}
			}
		}

		*pub_key = key_otp;
		platform_printf ("public key %d is 0x%x-0x%x-0x%x-..." NEWLINE, ind,
			(*pub_key)[index_to_read_key], (*pub_key)[index_to_read_key + 1],
			(*pub_key)[index_to_read_key + 2]);
	}

	/* Handle KMT keys */
	else if (fw->img_type == IMG_TFT_L0 || fw->img_type == IMG_SKMT) {
		if (kmt == NULL || kmt->fw == NULL) {
			platform_printf (KRED "\tKMT not found\t" KNRM);
			return -1;
		}
		/* the LMS keys are ordered after the ECC keys, and have the same amount of keys.	*/
		if (is_ECC_key) {
			offset_to_add_in_KMT = ind * ECC_KEY_SIZE;
		}
#ifdef LMS_ENABLE
		else if (is_LMS_key) {
			int kmt_keys_per_algo = (tip_firmware_component_get_key_count (fw, kmt, skmt)) / 2;
			offset_to_add_in_KMT =
				(kmt_keys_per_algo * ECC_KEY_SIZE) + (ind * LMS_KEY_SIZE);
			index_to_read_key = 8;
		}
#endif
		if (fw->header_ram) {
			if (!kmt->ram_copy_valid) {
				platform_printf (KRED "Error:RAM KMT invalid" NEWLINE KNRM);
				return KEY_MANIFEST_UNSUPPORTED_KEY;
			}
			platform_printf (KMAG "\tGet KMT key (%d) from RAM\t" KNRM, ind);

			/* NTIL: if key_manifest had index, this would have been nicer... */
			*pub_key =
				(uint8_t *) kmt->fw->header_ram + sizeof (IMG_HEADER_T) + offset_to_add_in_KMT;
		}
		else {
			platform_printf (KMAG "\tGet KMT key (%d) from FLASH\t" KNRM, ind);
			*pub_key =
				(uint8_t *) kmt->fw->header_flash + sizeof (IMG_HEADER_T) + offset_to_add_in_KMT;
		}
		platform_printf ("public key %d is 0x%x-0x%x-0x%x-..." NEWLINE NEWLINE, ind,
			(*pub_key)[index_to_read_key], (*pub_key)[index_to_read_key + 1],
			(*pub_key)[index_to_read_key + 2]);
	}

	/* Handle SKMT keys */
	else {
		platform_printf ("\tGet SKMT key (%d)\t", ind);
		status = tip_skmt_key_lookup (skmt, ind, pub_key, &skmt_pub_key_length);
		if (status != 0) {
			platform_printf ("SKMT key %d not found" NEWLINE KNRM, ind);
			return status;
		}
	}

	/* Check the case the key is valid, but it's all zeros or all 1s. */
	key_actual_length = is_LMS_key ? oLMS_PKn_LENGTH : (is_ECC_key ? oPKn_LENGTH : 0);
	key_all_zeros = true;
	key_all_ones = true;

	for (i = 0; i < key_actual_length; i++) {
		if ((*pub_key)[i] != 0) {
			key_all_zeros = false;
		}
		if ((*pub_key)[i] != 0xFF) {
			key_all_ones = false;
		}
		/* The key is not all zeros or ones - so key is valid. */
		if ((key_all_zeros == false) && (key_all_ones == false)) {
			return 0;
		}
	}

	platform_printf (KRED "key is empty" NEWLINE KNRM, ind);
	return KEY_MANIFEST_UNSUPPORTED_KEY;
}

/**
 *  Scan the flash to find the component by searching for start tag. Used by boot flow.
 *
 * @param fw TIP firmware component.
 * @param flash The flash to scan.
 * @param offset The flash offset to start scanning.
 *
 * @return 0 if success or an error code.
 */
static int tip_firmware_component_scan_flash (struct tip_firmware_component *fw,
	struct spi_flash *flash, uint32_t offset)
{
	struct tip_flash_master *tip_fl;
	const uint8_t *tag = tip_fw_l1_header_tag;
	uint32_t dst_addr;
	uint32_t scan_from;
	uint32_t scan_to;
	int status;

	if (fw == NULL || flash == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
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
			return FIRMWARE_COMPONENT_BAD_HEADER;
	}

	tip_fl = (struct tip_flash_master *) flash->spi;

	status = tip_flash_reconfig_address_mode (tip_fl->fiu, tip_fl->cs);
	if (status) {
		platform_printf (KRED "FIU%d CS%d: flash not found" NEWLINE KNRM, tip_fl->fiu, tip_fl->cs);
		return status;
	}

	/* check overflow */
	if (offset >= tip_fl->size) {
		offset = 0;
	}

	offset = ROUND_UP (offset, 0x1000);

	scan_from = tip_fl->base_addr + offset;
	scan_to = tip_fl->base_addr + tip_fl->size;

	platform_printf ("scan from %#010lx to %#010lx" NEWLINE, scan_from, scan_to);

	/* scan the flash, search for tags. */
	for (uint32_t addr = scan_from; addr < scan_to; addr += 0x1000) {
		if (COMPARE_START_TAG (addr, tag)) {
			fw->header_flash = (HEADER_GENERAL_T *) addr;
			fw->base.start_addr = addr - tip_fl->base_addr;

			if (fw->img_type == IMG_KMT || fw->img_type == IMG_TFT_L0) {
				TIP_HEADER_STRUCT_T *rom_header;
				rom_header = (TIP_HEADER_STRUCT_T *) fw->header_flash;
				fw->size = rom_header->img_length + sizeof (TIP_HEADER_STRUCT_T);
				fw->key_index = rom_header->key_index;
				dst_addr = rom_header->load_start_addr;
			}
			else {
				fw->size = fw->header_flash->header.codeSize + sizeof (HEADER_GENERAL_T);
				fw->key_index = fw->header_flash->header.KeyIndex;
				dst_addr = fw->header_flash->header.destAddr;
			}
#ifdef LMS_ENABLE
			/* after verification with ECC key, moving to verification with LMS key */
			if (fw->header_flash->header.enableLMS == 0x01 || fustrap1_lms_enabeld ||
				tip_is_kmt_and_LMS_is_forced_in_otp(fw)) {
				fw->is_lms_sig_exists = true;
				fw->size += LMS_SIG_SIZE;
			} else {
				fw->is_lms_sig_exists = false;
			}
#endif

			platform_printf (KGRN "%s found addr %#010lx dst %#010lx size %#010lx" NEWLINE KNRM,
				tip_firmware_component_get_name (fw->img_type), addr, dst_addr, fw->size);

			return 0;
		}
	}

	return FIRMWARE_COMPONENT_BAD_HEADER;
}

/**
 * Initialize TIP firmware component handler, search it on flash.
 *
 * @param fw Firmware component handler.
 * @param flash The flash to scan from.
 * @param img_type  L0\L1\SKMT\KMT\BOOTBLOCK\BL31\OPTEE\UBOOT.
 * @param start_offset Start offset in flash to start the scan.
 * @param scan_flash true to search for the image on flash, false to skip search
 *
 * @return 0 if success or an error code.
 */
int tip_firmware_component_init (struct tip_firmware_component *fw, struct spi_flash *flash,
	IMG_TYPE_E img_type, uint32_t start_offset, bool scan_flash)
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

	if ((img_type != IMG_UNKNOWN) && (scan_flash == true)) {
		status = tip_firmware_component_scan_flash (fw, flash, start_offset);
		if (status != 0) {
			platform_printf (KRED "no image found on flash" NEWLINE KNRM);
			return status;
		}
	}

	return 0;
}

#ifdef TIP_SUPPORT_ENCRYPTED_IMAGE
/**
 * Enc\Dec single firmware component using an OTP key and IV from header.
 *
 * @param fw Firmware component handler.
 * @param flash - virual flash to be used as encryption staging area.
 * @aes_engine engine for AES op.
 * @key_ind - OTP key index, out 6 keys.
 * @param b_enc during update need to encrypt. during boot need to decrypt
 *
 * @return 0 if success or an error code.
 */
static int tip_firmware_component_aes (struct tip_firmware_component *fw,
	struct tip_flash_master_virtual *flash, uint8_t key_ind, bool b_enc)
{
	struct tip_aes_ncl_engine fw_enc_dec_aes;
	TIP_HEADER_STRUCT_T *tip_rom_header;
	HEADER_GENERAL_T *tip_rom_header2;
	uint8_t aes_valid = 0;
	uint32_t *data_src;
	uint32_t *data_dst;
	uint32_t fw_len;
	uint32_t header_size;
	uint32_t *iv;
	int status;

	if (fw == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

	/* public key stores are not encrypted */
	if ((fw->img_type == IMG_KMT) || (fw->img_type == IMG_SKMT))
		return 0;

	if (fw->header_ram) {
		tip_rom_header = (TIP_HEADER_STRUCT_T *) fw->header_ram;
	}
	else {
		tip_rom_header = (TIP_HEADER_STRUCT_T *) fw->header_flash;
	}

	/* check if encrypt flag is set to 0x03 */
	if (tip_rom_header->tft_enc_ctl == 0x03) {
		if (fw->img_type <= IMG_TFT_L0) {
			header_size = sizeof (IMG_HEADER_T);
			data_src = (uint32_t*) ((void *) tip_rom_header + header_size);
			fw_len = (uint32_t) tip_rom_header->img_length;
			iv = (uint32_t *) tip_rom_header->aes_cbc_iv;
		}
		else {
			header_size = sizeof (HEADER_GENERAL_T);
			tip_rom_header2 = (HEADER_GENERAL_T *) tip_rom_header;
			data_src = (uint32_t*) ((void *) tip_rom_header2 + header_size);
			fw_len = (uint32_t) tip_rom_header2->header.codeSize;
			iv = (uint32_t*) tip_rom_header2->header.aes_cbc_iv;
		}

		/* copy in place, unless the image is on flash */
		data_dst = data_src;

		/* connot decrypt in-place on flash, need to copy to RAM staging area first.
		 * Copy the header directly, and then decrypt on the fly for the rest of the image
		 */
		fw->header_decrypt_staging = NULL;

		if ((fw->header_ram == NULL) && (b_enc == false)) {
			tip_memcpy (TIP_VIRTUAL_FLASH_BASE_ADDR, (uint32_t) tip_rom_header, header_size, false,
				true);
			data_src = (uint32_t*) ((uint32_t) tip_rom_header + header_size);
			data_dst = (uint32_t*) (TIP_VIRTUAL_FLASH_BASE_ADDR + header_size);
			fw->header_decrypt_staging = (HEADER_GENERAL_T *) TIP_VIRTUAL_FLASH_BASE_ADDR;
		}

		platform_printf (KMAG "%s AES %s addr %010lx src %010lx dst %010lx len %010lx" NEWLINE KNRM,
			tip_firmware_component_get_name (fw->img_type), (b_enc == true) ? "ENC" : "DEC",
			(uint32_t) tip_rom_header, (uint32_t) data_src, data_dst, fw_len);

		FUSE_WRPR_get (TIP_AES_KEY0_VALID_PROPERTY, &aes_valid);
		if (aes_valid != 0xAA) {
			platform_printf (KRED "Missing AES key on device" KNRM NEWLINE);
			return AES_GCM_ENGINE_NO_KEY;
		}

		/* init the engine and the key values */
		status = tip_aes_ncl_init (&fw_enc_dec_aes);
		if (status != 0) {
			goto aes_done;
		}

		status = fw_enc_dec_aes.set_mode (&fw_enc_dec_aes, NCL_AES_MODE_CBC);
		if (status != 0) {
			goto aes_done;
		}

		status = fw_enc_dec_aes.select_key (&fw_enc_dec_aes, key_ind);
		if (status != 0) {
			goto aes_done;
		}

		if (b_enc == true) {
			status = fw_enc_dec_aes.base.encrypt_data (&(fw_enc_dec_aes.base), (uint8_t*) data_src,
				fw_len, (uint8_t*) iv, NCL_AES_KEY_SIZE_256, (uint8_t*) data_dst, fw_len, NULL,
				0);
		}
		else {
			status = fw_enc_dec_aes.base.decrypt_data (&(fw_enc_dec_aes.base), (uint8_t *) data_src,
				fw_len, NULL, (uint8_t *) iv, NCL_AES_KEY_SIZE_256, (uint8_t *) data_dst, fw_len);
		}

aes_done:
		tip_aes_ncl_release (&fw_enc_dec_aes);
		return status;
	}

	return 0;
}
#endif /* TIP_SUPPORT_ENCRYPTED_IMAGE */

static int tip_firmware_component_verify_ecc_or_lms_sig (struct tip_firmware_component *fw,
	struct tip_primary_key_manifest *kmt, struct tip_secondary_key_manifest *skmt,
	uint8_t *digest_out, size_t digest_length, bool isECC, bool isLMS, HEADER_GENERAL_T *header,
	uint8_t *imageAddr, uint32_t imageLen)
{
	/* status fail until proven otherwise */
	int status = FIRMWARE_COMPONENT_VERIFY_FAILED;
	SECURED_BOOLEAN_T sigOK = SECURED_FALSE;
	uint8_t *pub_key;
	uint8_t key_index, key_index_min, key_index_max;
	uint32_t key_mask;
	char *fw_name = tip_firmware_component_get_name (fw->img_type);
	uint32_t mask_from_header;
	uint32_t index_from_header;
	char *name;
	uint8_t hash_alg_ecc = (digest_length == SHA384_HASH_LENGTH ? HASH_TYPE_SHA384 : HASH_TYPE_SHA512);
#ifdef LMS_ENABLE
	uint8_t hash_alg_lms = (digest_length == SHA384_HASH_LENGTH ? SHA384_HASH_LENGTH : SHA512_HASH_LENGTH);
#endif

	/* This function handles either ECC or LMS. It is called up to twice */
	if (isECC == isLMS) {
		return FIRMWARE_COMPONENT_BAD_HEADER;
	}

	if (isECC == true) {
		mask_from_header = header->header.key_mask_select;
		index_from_header = header->header.KeyIndex;
		name = "ECC";
	}

#ifdef LMS_ENABLE
	if (isLMS == true) {
		mask_from_header = header->header.key_mask_select_lms;
		index_from_header = header->header.KeyIndex_lms;
		name = "LMS";
	}
#endif

	/* Get key for the image. */
	if (mask_from_header != 0) {
		key_index_min = 0;
		key_index_max = tip_firmware_component_get_key_count (fw, kmt, skmt);
		key_mask = mask_from_header;
	}
	else {
		key_index = index_from_header;
		key_index_min = key_index;
		key_index_max = key_index + 1;
		key_mask = 1L << key_index;
	}

	for (key_index = key_index_min; key_index < key_index_max; key_index++) {
		int key_size;
		if ((key_mask & (0x01 << key_index)) == 0) {
			continue;
		}

		key_size = isECC ? ECC_KEY_SIZE : (isLMS ? LMS_KEY_SIZE : 0);
		status = tip_firmware_component_get_key (fw, kmt, skmt, key_index, &pub_key, key_size, isECC, isLMS);
		if (status != 0) {
			continue;
		}

		/* perform ECC verification */
		if (isECC) {
			status = tip_image_auth_ECC (header, imageAddr, imageLen, pub_key, digest_out, &sigOK, hash_alg_ecc);

			/* Erase after use. Use otp key only once */
			if (pub_key == key_otp) {
				memset (key_otp, 0, sizeof (key_otp));
			}

			if (sigOK == SECURED_TRUE) {
				break;
			}
		
			/* L1 must be verified with first SKMT key.
			* This will prevent usage of keys which are not dedicated for TIP.
			*/
			if (fw->img_type == IMG_TFT_L1) {
				key_index = 0;
				key_mask = 1;
			}

			platform_printf ("try key %d mask %#010lx" NEWLINE, key_index, key_mask);
		}
#ifdef LMS_ENABLE
		/* perform LMS verification */
		else if (isLMS) {
			status = tip_image_auth_LMS (header, imageAddr, imageLen, pub_key, digest_out, &sigOK, hash_alg_lms);
			/* Erase after use. Use otp key only once */
			if (pub_key == key_otp) {
				memset (key_otp, 0, sizeof (key_otp));
			}

			if (sigOK == SECURED_TRUE) {
				break;
			}
		}
#endif
	}

	if (sigOK == SECURED_TRUE) {
		platform_printf (KGRN "%s key %d %s SIG PASS" NEWLINE KNRM, fw_name, fw->key_index, name);
	}
	else {
		platform_printf (KRED "%s key %d %s SIG FAIL, status %x" NEWLINE KNRM, fw_name, fw->key_index, name, status);
		return FIRMWARE_COMPONENT_BAD_SIGNATURE;
	}

	return status;
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
int tip_firmware_component_verify (struct tip_firmware_component *fw,
	const struct hash_engine *hash, struct tip_primary_key_manifest *kmt,
	struct tip_secondary_key_manifest *skmt, uint8_t *digest_out, size_t digest_length)
{
	/* TODO: Add an interface to NCL hardware engine. Currently hardware hash engine is utilized but
	 * without an option to pass it explicity.*/
	UNUSED (hash);
	DEFS_STATUS hal_status = DEFS_STATUS_OK;
	/* status fail until proven otherwise */
	int status = FIRMWARE_COMPONENT_VERIFY_FAILED;
	HEADER_GENERAL_T *header;
	uint8_t *imageAddr;
	uint32_t imageLen;
	uint8_t digest_buf[SHA512_HASH_LENGTH] = {0};
	uint16_t version;
	uint16_t next_version;
	uint8_t hash_alg = (digest_length == SHA384_HASH_LENGTH ? HASH_TYPE_SHA384 : HASH_TYPE_SHA512);

#ifdef TIP_SUPPORT_ENCRYPTED_IMAGE
	/* b_enc: true during image update, false during boot */
	bool b_enc = false;
	uint32_t spi, fiu, cs, offset;
#endif /* TIP_SUPPORT_ENCRYPTED_IMAGE */

	if (fw == NULL) {
		return FIRMWARE_COMPONENT_INVALID_ARGUMENT;
	}

#ifdef TIP_SUPPORT_ENCRYPTED_IMAGE
	if (tip_L1_sys_ctrl.tip_support_encryption) {
		/* check if need to enc\dec image. During boot: decrypt, during update : encrypt */
		tip_flash_get_index_of_currently_running_image (&spi, &fiu, &cs, &offset);
		if ((spi == 0) && (offset == 0)) {
			b_enc = true;
		}

		/* if verfication is required during boot : decrypt before verify key */
		if (b_enc == false) {
			status = tip_firmware_component_aes (fw, (struct tip_flash_master_virtual *) &virtual_flash,
				TIP_PER_DEVICE_DECRYPT_KEY, b_enc);
			if (status != 0) {
				platform_printf (KRED "AES decryption failed" KNRM NEWLINE);
				return FIRMWARE_COMPONENT_VERIFY_FAILED;
			}
		}
	}
#endif /* TIP_SUPPORT_ENCRYPTED_IMAGE */

	/* if the image is not loaded to RAM, verify directly on flash */
	header = fw->header_ram ? fw->header_ram : fw->header_flash;

	/* if the image is after decryption on-the-fly use the to staging area */
	if (fw->header_decrypt_staging != NULL) {
		header = fw->header_decrypt_staging;
	}

	imageAddr = (uint8_t*) header->header.reservedSigned;

	/* signed area starts right after the signature inside the header */
	imageLen = fw->size - 0x70;
	if (fw->is_lms_sig_exists){
		imageLen -=  LMS_SIG_SIZE;
	}

	hal_status = tip_image_hash (header, imageAddr, imageLen, digest_buf, hash_alg);

	if (hal_status == DEFS_STATUS_INVALID_DATA_FIELD)
		return FIRMWARE_COMPONENT_INVALID_SIGNATURE;

	if (hal_status != DEFS_STATUS_OK) {
		platform_printf (KRED "hash image fail" NEWLINE KNRM);
		return FIRMWARE_COMPONENT_VERIFY_FAILED;
	}

	/* Copy the digest to output buffer that will be used for attestation. */
	if (digest_out != NULL && (digest_length >= sizeof (digest_buf))) {
		memcpy (digest_out, digest_buf, sizeof (digest_buf));
	}

	status = tip_firmware_component_verify_ecc_or_lms_sig (fw, kmt, skmt, digest_buf, digest_length, true,
		false, header, imageAddr, imageLen);

	if (status != 0) {
		return FIRMWARE_COMPONENT_VERIFY_FAILED;
	}

#ifdef LMS_ENABLE
	{
		/* after verification with ECC key, moving to verification with LMS key */
		if (header->header.enableLMS == 0x01 || fustrap1_lms_enabeld ||
			tip_is_kmt_and_LMS_is_forced_in_otp(fw)) {
			status = tip_firmware_component_verify_ecc_or_lms_sig (fw, kmt, skmt, digest_buf,
				digest_length, false, true, header, imageAddr, imageLen);
			if (status != 0) {
				return FIRMWARE_COMPONENT_VERIFY_FAILED;
			}
		}
	}
#endif

#ifdef TIP_SUPPORT_ENCRYPTED_IMAGE
	/* check if need to enc\dec image. During boot: decrypt, during update : encrypt */
	if (b_enc == true) {
		status = tip_firmware_component_aes (fw, (struct tip_flash_master_virtual *) &virtual_flash,
			TIP_PER_DEVICE_DECRYPT_KEY, b_enc);
		if (status != 0) {
			platform_printf (KRED "AES encryption failed" KNRM NEWLINE);
			return FIRMWARE_COMPONENT_VERIFY_FAILED;
		}
	}
#endif /* TIP_SUPPORT_ENCRYPTED_IMAGE */

	/* Check SVN against the minimal SVN on OTP. */
	if (fw->header_ram) {
		version = fw->header_ram->header.version;
		next_version = fw->header_ram->header.next_version;
	}
	else if (fw->header_flash) {
		version = fw->header_flash->header.version;
		next_version = fw->header_flash->header.next_version;
	}

	status = tip_version_check (tip_version, version, next_version, fw->img_type);
	if (status != 0) {
		return FIRMWARE_COMPONENT_VERIFY_FAILED;
	}

	status = tip_version_set (tip_version, version, next_version, fw->img_type);

	return status;
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
			return FIRMWARE_COMPONENT_BAD_HEADER;
	}

	size_max = addr_max - addr_min;

	img_size = fw->header_flash->header.codeSize + sizeof (HEADER_GENERAL_T);
	src_flash_addr = (uint32_t) (uint8_t*) fw->header_flash;

	if ((img_size < sizeof (HEADER_GENERAL_T)) || (img_size > size_max)) {
		platform_printf (KRED "invalid image size %d, fw->header_flash->header.codeSize is %d + "
							  "sizeof (HEADER_GENERAL_T) %d  " NEWLINE KNRM,
			img_size, fw->header_flash->header.codeSize, sizeof (HEADER_GENERAL_T));
		return FIRMWARE_COMPONENT_NO_LOAD_ADDRESS;
	}

	if ((dst_addr == 0xFFFFFFFF) || (dst_addr == 0) || (dst_addr < addr_min) ||
		(dst_addr + img_size) > addr_max) {
		platform_printf (KRED "image out of range  %#010lx : %#010lx" NEWLINE KNRM, addr_min,
			addr_max);
		return FIRMWARE_COMPONENT_TOO_LARGE;
	}

	/* bl31 needs its RAM to be cleared in advance if it is loaded to RAM2.*/
	if ((fw->img_type == IMG_BL31) && (dst_addr >= RAM2_BASE_ADDR)) {
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

#ifdef LMS_ENABLE
	/* in case the image contains a signature of LMS, need to copy it from the footer of the flash
	 * to the footer of the RAM */
	if (fw->header_ram->header.enableLMS == 0x01 || fustrap1_lms_enabeld ||
		tip_is_kmt_and_LMS_is_forced_in_otp(fw)) {
		tip_memcpy (dst_addr + img_size, src_flash_addr + img_size, LMS_SIG_SIZE,
			dram_window, true);
	}
#endif

	/* reset DRAM window back to start of image, so that header_ram points to header via window */
	if (dram_window == true) {
		uint32_t win_offset = 0;
		if (dst_addr % _64KB_ > 0) {
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
	const char *start_tags[10] = { kmt_header_tag, kmt_header_tag_crc, tip_fw_l0_header_tag,
		tip_fw_l0_header_tag_crc, skmt_header_tag, tip_fw_l1_header_tag, bb_header_tag,
		bl31_header_tag, optee_header_tag, uboot_header_tag };

	/* image types, respectively to start_tags */
	IMG_TYPE_E types[10] = { IMG_KMT, IMG_KMT, IMG_TFT_L0, IMG_TFT_L0, IMG_SKMT, IMG_TFT_L1,
		IMG_BOOTBLOCK, IMG_BL31, IMG_OPTEE, IMG_UBOOT };

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
				}
				else {
					fw->size = fw->header_flash->header.codeSize + sizeof (HEADER_GENERAL_T);
				}
#ifdef LMS_ENABLE
				/* after verification with ECC key, moving to verification with LMS key */
				if (fw->header_flash->header.enableLMS == 0x01 || fustrap1_lms_enabeld ||
					tip_is_kmt_and_LMS_is_forced_in_otp(fw)) {
					fw->is_lms_sig_exists = true;
					fw->size += LMS_SIG_SIZE;
				} else {
					fw->is_lms_sig_exists = false;
				}

#endif
				return 0;
			}
		}
	}

	return -1;
}

/**
 *  Check if there is an external register (hardening) table.
 *
 * @param fw Firmware component handler.
 *
 * @return true is register table exists, false if no table.
 */
bool tip_firmware_component_check_external_reg_table (struct tip_firmware_component *fw)
{
	if (fw == NULL) {
		return false;
	}
	
	if (fw->header_ram == NULL) {
		return false;
	}

	/* check if there is a table. If not: nothing to do. */
	if ((fw->header_ram->header.regs_offset == 0xFFFFFFFF) || (fw->header_ram->header.regs_offset == 0)) {
		return false; 
	}

	return true;
}

/**
 *  Load external register (hardening) table.
 *
 *  Each FW component has an optional regiser table appended to its tail.
 *  The table is written by IGPS user.
 *  This function checks if there is such a table. If there is TIP_FW parse
 *  and executes the table, one line at a time.
 *  Table execution is done after loading the image, but before it starts to execute.
 *
 * @param fw Firmware component handler.
 * @param reset Last reset type. Value match TIP_SCR1 register content.
 *
 * @return 0 if success or an error code.
 */
int tip_firmware_component_load_external_reg_table (struct tip_firmware_component *fw,
	uint16_t reset)
{
	uint32_t regs_offset;
	uint32_t reg_addr, reg_value, reg_mask, reg_delay_us, reg_before;
	uint32_t reg_cnt_total;
	uint16_t reg_reset;
	uint8_t reg_size, reg_delay;

	/* 4 bytes each: addr, mask, value, reset, delay, size */
	const uint32_t reg_line_size = 16;

	regs_offset =
		(uint32_t) fw->header_ram + fw->header_ram->header.regs_offset + sizeof (HEADER_GENERAL_T);

	reg_cnt_total =
		(fw->header_ram->header.codeSize - fw->header_ram->header.regs_offset) / reg_line_size;

	if (reg_cnt_total > TIP_FIRMWARE_MAX_HARDENING_REGISTERS_COUNT) {
		return FIRMWARE_COMPONENT_TOO_LARGE;
	}

	platform_printf (KMAG "parse %#010lx %d regs" NEWLINE KNRM, regs_offset, reg_cnt_total);

	for (int reg_cnt = 0; reg_cnt < reg_cnt_total; reg_cnt++) {
		reg_addr = *(uint32_t*) (regs_offset);
		reg_value = *(uint32_t*) (regs_offset + 0x04);
		reg_mask = *(uint32_t*) (regs_offset + 0x08);
		reg_size = *(uint8_t*) (regs_offset + 0x0C);
		reg_delay = *(uint8_t*) (regs_offset + 0x0D);
		reg_reset = *(uint16_t*) (regs_offset + 0x0E);
		regs_offset += reg_line_size;

		reg_delay_us = pow (2, reg_delay) - 1;

		if ((reg_reset & reset) == 0) {
			continue;
		}

		reg_before = *(uint32_t*) reg_addr;

		switch (reg_size) {
			case 0x02:
				if (reg_mask == 0xFFFFFFFF) {
					*(uint32_t*) reg_addr = reg_value;
				}
				else {
					*(uint32_t*) reg_addr =
						(*(uint32_t*) reg_addr & ~reg_mask) | (reg_value & reg_mask);
				}
				break;
			case 0x01:
				reg_mask &= 0x0000FFFF;
				reg_value &= 0x0000FFFF;
				if (reg_mask == 0xFFFF) {
					*(uint16_t*) reg_addr = (uint16_t) reg_value;
				}
				else {
					*(uint16_t*) reg_addr = (*(uint16_t*) reg_addr & (uint16_t) ~reg_mask) |
						((uint16_t) reg_value & (uint16_t) reg_mask);
				}
				break;
			case 0x00:
			default:
				reg_mask &= 0x000000FF;
				reg_value &= 0x000000FF;
				if (reg_mask == 0xFF) {
					*(uint8_t*) reg_addr = (uint8_t) reg_value;
				}
				else {
					*(uint8_t*) reg_addr = (*(uint8_t*) reg_addr & (uint8_t) ~reg_mask) |
						((uint8_t) reg_value & (uint8_t) reg_mask);
				}
				break;
		}

		platform_printf ("[%d] addr %#010lx val %#010lx msk %#010lx rst %#05x "
						 "%dus %#010lx->%#010lx" NEWLINE,
			reg_cnt, reg_addr, reg_value, reg_mask, reg_reset, reg_delay_us, reg_before,
			*(uint32_t*) reg_addr);

		if (reg_delay_us != 0) {
			CLK_Delay_MicroSec (reg_delay_us);
		}
	}

	return 0;
}
