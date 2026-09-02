// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <string.h>
#include "tip_key_manifest.h"
#include "tip_boot.h"
#include "platform_io.h"
#include "tip_rom_utils.h"

/**
 * Verify that the key manifest is valid.  A valid manifest is one that has a good signature
 * with a validated key.
 *
 * Verification of the key manifest is not guaranteed to be reentrant.
 *
 * @param manifest The manifest to validate.
 * @param hash The hash engine to use for validation.
 *
 * @return 0 if the manifest is valid or an error code.
 */
static int tip_key_manifest_verify_func (const struct key_manifest *manifest,
	struct hash_engine *hash)
{
	int status;
	struct tip_primary_key_manifest *kmt = (struct tip_primary_key_manifest *) manifest;

	if (kmt == NULL || kmt->fw == NULL || hash == NULL) {
		return KEY_MANIFEST_INVALID_ARGUMENT;
	}

	/* Verify KMT like any other binary. */
	status = tip_firmware_component_verify (kmt->fw, hash, NULL, NULL, NULL, 0);

	return status;
}

static int tip_key_manifest_is_allowed_func_empty (const struct key_manifest *manifest)
{
	return 0;
}

static int tip_revokes_old_manifest_func_empty (const struct key_manifest *manifest)
{
	return 0;
}

static int tip_update_revocation_func_empty (const struct key_manifest *manifest)
{
	return 0;
}

static const struct key_manifest_public_key *tip_get_app_key_func_empty (
	const struct key_manifest *manifest)
{
	return NULL;
}

static const struct key_manifest_public_key *tip_get_manifest_key_func_empty (
	const struct key_manifest *manifest)
{
	return NULL;
}

/**
 * Initialize TIP primary key manifest handler.
 *
 * @param kmt TIP primary key manifest handler.
 * @param kmt_img Firmware component handler of kmt.
 *
 * @return 0 if success or an error code.
 */
int tip_key_manifest_init (struct tip_primary_key_manifest *kmt,
	struct tip_firmware_component *kmt_img)
{
	if (kmt == NULL || kmt_img == NULL) {
		return KEY_MANIFEST_INVALID_ARGUMENT;
	}

	memset (kmt, 0, sizeof (struct tip_primary_key_manifest));

	kmt->base.verify = tip_key_manifest_verify_func;
	/* Initialize base with empty APIs since TIP doesn't support key revocation for now.
	 * get_app_key and get_manifest_key API were not used in current workflow. */
	kmt->base.is_allowed = tip_key_manifest_is_allowed_func_empty;
	kmt->base.revokes_old_manifest = tip_revokes_old_manifest_func_empty;
	kmt->base.update_revocation = tip_update_revocation_func_empty;
	kmt->base.get_app_key = tip_get_app_key_func_empty;
	kmt->base.get_manifest_key = tip_get_manifest_key_func_empty;

	kmt->fw = kmt_img;

	return 0;
}
	
static int tip_key_manifest_validate_key (uint8_t **data_kmt, uint16_t key_size, int num_of_keys, const char *key_type, uint16_t start_from)
{
	for (int kmt_key_cnt = 0; kmt_key_cnt < num_of_keys; ++kmt_key_cnt) {
		*data_kmt += key_size;

        uint16_t valid_kmt = *(uint16_t *)(*data_kmt + (key_size - 2));
        if (valid_kmt != 0x3A3A) {
            platform_printf (KRED "%s public key %d invalid" NEWLINE KNRM, key_type, kmt_key_cnt);
            return KEY_MANIFEST_WEAK_KEY;
        }

        platform_printf ("%s public key %d is 0x%x-0x%x-0x%x-..., key is valid" NEWLINE,
                        key_type, kmt_key_cnt, (*data_kmt)[start_from], (*data_kmt)[start_from + 1], (*data_kmt)[start_from + 2]);

	}

	return 0;
}

/**
 * Parse the key manifest that was verified by TIP ROM and relocate to a different RAM region.
 * If function is called during flash update, TIP_FW will not copy and only parse
 * the KMT on flash.
 *
 * @param kmt Key manifest
 * @param verified_kmt  RAM address storing ROM verified KMT.
 * @param kmt_copy_addr Destination RAM address to copy KMT.
 * @param copy_to_ram   true\false to copy image.
 *
 * @return 0 if the manifest is parsed and copied successfully or an error code.
 */
 int tip_key_manifest_parse_and_copy (struct tip_primary_key_manifest *kmt,
	const uint8_t *verified_kmt, uint32_t kmt_copy_addr, bool copy_to_ram)
{
	IMG_HEADER_T *KMT_HeaderRamPtr;
	uint8_t *data_kmt;
	unsigned int num_of_ecc_keys = 0;
	unsigned int num_of_lms_keys = 0;
	int status;

	if (kmt == NULL || verified_kmt == NULL) {
		return KEY_MANIFEST_INVALID_ARGUMENT;
	}

	if ((copy_to_ram == true) && ((kmt_copy_addr < TIP_RAM_BASE_ADDR) ||
		(kmt_copy_addr > TIP_RAM_BASE_ADDR + TIP_RAM_MEMORY_SIZE))) {
		return KEY_MANIFEST_INVALID_ARGUMENT;
	}

	KMT_HeaderRamPtr = (IMG_HEADER_T *) verified_kmt;

	kmt->num_of_kmt_keys = KMT_HeaderRamPtr->header.img_length / ECC_KEY_SIZE;
	num_of_ecc_keys = kmt->num_of_kmt_keys;

#ifdef LMS_ENABLE
	{
		uint32_t LMS_Start_offset = (uint32_t)(KMT_HeaderRamPtr->header.LMS_KMO);
		if (LMS_Start_offset > 0) {
			if (LMS_Start_offset != (uint32_t) (KMT_HeaderRamPtr->header.img_length * ECC_KEY_SIZE /
												(ECC_KEY_SIZE + LMS_KEY_SIZE)))
				return KEY_MANIFEST_INVALID_ARGUMENT;

			/* Calculate the number of keys using bitwise shift */
			num_of_ecc_keys = LMS_Start_offset / ECC_KEY_SIZE;
			num_of_lms_keys = (KMT_HeaderRamPtr->header.img_length - LMS_Start_offset) / LMS_KEY_SIZE;
			/* Calculate the total number of keys */
			kmt->num_of_kmt_keys = num_of_ecc_keys + num_of_lms_keys;
			platform_printf ("LMS offset %x, %d ECC keys, %d LMS keys", NEWLINE,
				LMS_Start_offset, num_of_ecc_keys, num_of_lms_keys);
		
		} 
	}
#endif

	platform_printf ("KMT num_of_kmt_keys is %d" NEWLINE, kmt->num_of_kmt_keys);
	if (kmt->num_of_kmt_keys > KMT_MAX_KEY_NUM) { 
		platform_printf (KRED "ERROR: only %d KMT keys are allowed, not %d" NEWLINE KNRM,
			KMT_MAX_KEY_NUM, kmt->num_of_kmt_keys);
		
		return KEY_MANIFEST_INVALID_FORMAT;
	}
	

	if (num_of_ecc_keys > 0) {
		data_kmt = (uint8_t *) KMT_HeaderRamPtr + sizeof (IMG_HEADER_T) - ECC_KEY_SIZE;
		/* Validate ECC keys */
		status = tip_key_manifest_validate_key (&data_kmt, ECC_KEY_SIZE, num_of_ecc_keys, "KMT ECC", 0);
		if (status) {
			return KEY_MANIFEST_WEAK_KEY;
		}
	}

#ifdef LMS_ENABLE
	if (num_of_lms_keys > 0) {
		/* Move data_kmt back to the start of LMS keys */
		data_kmt += ECC_KEY_SIZE - LMS_KEY_SIZE;
		/* Validate LMS keys */
		status = tip_key_manifest_validate_key (&data_kmt, LMS_KEY_SIZE, num_of_lms_keys, "KMT LMS", 8);
		if (status) {
			return KEY_MANIFEST_WEAK_KEY;
		}
	}
#endif

	if (copy_to_ram == true) {
		memcpy ((uint8_t *) kmt_copy_addr, verified_kmt, 256 + sizeof (IMG_HEADER_T) + (LMS_KEY_SIZE * num_of_lms_keys) );

		kmt->fw->header_ram = (HEADER_GENERAL_T *) kmt_copy_addr;
		kmt->ram_copy_valid = true;

		platform_printf ("Copy KMT to %#010lx" NEWLINE, kmt_copy_addr);
	}

	return 0;
}

/**
 * Invalidate keys in OTP according to key_invalid mask value from Kmt_and_header.
 * FUSTRAP2 is the OTP value in which TIP needs to set the key_invalid mask
 *
 * @return 0 if success or an error code.
 */
int tip_invalidate_otp_keys (void)
{
	/* verify that the required key(s) to be invalidate are legal
	 * verify that its not the key that was booted with
	 */
	uint16_t last_key = READ_REG_FIELD (LASTKEY, LASTKEY_LAST_KEY);
	uint32_t key_invalid = REG_READ (TIP_SCR2);
	if (key_invalid >= (1 << last_key)) {
		platform_printf (KRED "one of the required keys is illegal when last key is:  "
							  "%d and key_invalid =0x%x" NEWLINE KNRM,
			last_key, key_invalid);
		return KEY_MANIFEST_INVALID_ARGUMENT;
	}
	platform_printf (KGRN
		"used key is key #%d, mask to invalidate other key(s) is 0x%x" NEWLINE KNRM,
		last_key, key_invalid);
#if FUSE_OTP || TIP_DUMMY_OTP_ON_RAM
	/* enable FCFG to fuse to OTP */
	uint8_t block = FUSE_WRPR_PROP_BLOCK (FUSTRAP2_PROPERTY) / 8;
	uint8_t fcfg_val = READ_REG_FIELD (FCFG0_7 (block), FCFG0_7_FPRGDIS);
	SET_REG_FIELD (FCFG0_7 (block), FCFG0_7_FPRGDIS, fcfg_val & 0xFE);
	FUSE_WRPR_set (FUSTRAP2_PROPERTY, (uint8_t *) &key_invalid);
	/* disable FCFG once done */
	SET_REG_FIELD (FCFG0_7 (block), FCFG0_7_FPRGDIS, fcfg_val);
#endif
	return 0;
}
