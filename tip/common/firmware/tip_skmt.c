// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "tip_skmt.h"
#include "tip_flash.h"
#include "platform_io.h"
#include "rot_memory_map.h"
#include "tip_rom_utils.h"


static int tip_skmt_is_allowed_func_empty (const struct key_manifest *manifest)
{
	return 0;
}

static int tip_skmt_revokes_old_manifest_func_empty (const struct key_manifest *manifest)
{
	return 0;
}

static int tip_skmt_update_revocation_func_empty (const struct key_manifest *manifest)
{
	return 0;
}

static int tip_skmt_verify_func_empty (const struct key_manifest *manifest,
	struct hash_engine *hash)
{
	return 0;
}

static const struct key_manifest_public_key *tip_skmt_get_root_key_func_empty (
	const struct key_manifest *manifest)
{
	return NULL;
}

static const struct key_manifest_public_key *tip_skmt_get_app_key_func_empty (
	const struct key_manifest *manifest)
{
	return NULL;
}

static const struct key_manifest_public_key *tip_skmt_get_manifest_key_empty (
	const struct key_manifest *manifest)
{
	return NULL;
}

/**
 * Initialize TIP secondary key manifest handler.
 *
 * @param skmt Secondary key manifest instance.
 * @param skmt_img Firmware component handler.
 * @param verified_copy_addr In-memory copy of SKMT that has been verified.
 * @return 0 if success or an error code.
 */
int tip_skmt_init (struct tip_secondary_key_manifest *skmt, struct tip_firmware_component *skmt_img,
	uint32_t verified_copy_addr)
{
	if (skmt == NULL || skmt_img == NULL) {
		return KEY_MANIFEST_INVALID_ARGUMENT;
	}

	memset (skmt, 0, sizeof (struct tip_secondary_key_manifest));

	/* Initialize base instance with empty APIs since TIP doesn't support key revocation for now.
	 * SKMT base APIs were not used in current current workflow. */
	skmt->base.is_allowed = tip_skmt_is_allowed_func_empty;
	skmt->base.revokes_old_manifest = tip_skmt_revokes_old_manifest_func_empty;
	skmt->base.update_revocation = tip_skmt_update_revocation_func_empty;
	skmt->base.verify = tip_skmt_verify_func_empty;
	skmt->base.get_app_key = tip_skmt_get_app_key_func_empty;
	skmt->base.get_root_key = tip_skmt_get_root_key_func_empty;
	skmt->base.get_manifest_key = tip_skmt_get_manifest_key_empty;

	skmt->ram_copy_addr = verified_copy_addr;
	skmt->fw = skmt_img;

	return 0;
}

/**
 * Copy verified secondary key manifest (without header) to TIP RAM.
 *
 * @param skmt TIP secondary key manifest handler.
 * @param addr Destination address.
 *
 * @return 0 if success or an error code.
 */
int tip_skmt_copy (struct tip_secondary_key_manifest *skmt, uint32_t addr)
{
	if (skmt == NULL || skmt->fw == NULL || skmt->fw->header_ram == NULL ||
		addr < TIP_RAM_BASE_ADDR || addr > (TIP_RAM_BASE_ADDR + TIP_RAM_MEMORY_SIZE)) {
		return KEY_MANIFEST_INVALID_ARGUMENT;
	}

	memcpy ((uint8_t *) addr, (uint8_t *) skmt->fw->header_ram + sizeof (HEADER_GENERAL_T),
		skmt->fw->header_ram->header.codeSize);

	skmt->ram_copy_addr = addr;
	platform_printf ("Copy SKMT to %#010lx" NEWLINE, addr);

	return 0;
}

/**
 * Parse the public keys inside SKMT.
 *
 * @param skmt TIP secondary key manifest handler.
 *
 * @return 0 if success or an error code.
 */
int tip_skmt_parse (struct tip_secondary_key_manifest *skmt)
{
	uint32_t addr;

	if (skmt == NULL || skmt->fw == NULL) {
		return KEY_MANIFEST_INVALID_ARGUMENT;
	}

	/* Boot flow: parse from valid in-memory copy. */
	if (skmt->ram_copy_addr) {
		addr = skmt->ram_copy_addr;
		platform_printf (KMAG "Parse SKMT from RAM (addr %#010lx)" NEWLINE KNRM, addr);
	} else {
		/* Update flow: parse from flash. */
		addr = (uint32_t) skmt->fw->header_flash + sizeof (HEADER_GENERAL_T);
		platform_printf (KMAG "Parse SKMT from FLASH (addr %#010lx)" NEWLINE KNRM, addr);
	}

	skmt->num_of_keys = 0;

	for (int key_ind = 0; key_ind < SKMT_MAX_KEY_NUM; ++key_ind) {
		switch (*(uint16_t *) addr) {
			case SKMT_KEY_ECC:
			case SKMT_KEY_RSA:
			case SKMT_KEY_AES:
			case SKMT_KEY_CERT:
				/* Save the parse results into run-time structure within SKMT. */
				skmt->keys[key_ind].skmt_type = *(uint16_t *) addr;
				skmt->keys[key_ind].skmt_key_size = *(uint16_t *) (addr + 2);
				skmt->keys[key_ind].skmt_key = (uint8_t *) (addr + 4);
				skmt->num_of_keys++;
				platform_printf (
					"SKMT key %d, addr %#010lx key_type %c, key_size 0x%lx, key = 0x%x-0x%x-0x%x..." NEWLINE,
					key_ind, addr, (char) (skmt->keys[key_ind].skmt_type >> 8),
					skmt->keys[key_ind].skmt_key_size, skmt->keys[key_ind].skmt_key[0],
					skmt->keys[key_ind].skmt_key[1], skmt->keys[key_ind].skmt_key[2]);

				addr += 4 + skmt->keys[key_ind].skmt_key_size;
				break;

			default:
				break;
		}
	}

	if (skmt->num_of_keys == 0) {
		return KEY_MANIFEST_INVALID_FORMAT;
	}

	return 0;
}

/**
 * Look up a key from the secondary key manifest based on the key index.
 *
 * @param skmt TIP secondary key manifest.
 * @param id Key index.
 * @param [out] key Output buffer to save the key.
 * @param [out] length Output Buffer length.
 *
 * @return 0 if sucess or an error code.
 */
int tip_skmt_key_lookup (struct tip_secondary_key_manifest *skmt, int id, uint8_t **key,
	size_t *length)
{
	if ((skmt == NULL) || (key == NULL) || (length == NULL) ||
		(skmt->num_of_keys <= id)) {
		return KEY_MANIFEST_INVALID_ARGUMENT;
	}

	*key = skmt->keys[id].skmt_key;
	*length = skmt->keys[id].skmt_key_size;

	return 0;
}
