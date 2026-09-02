// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <string.h>
#include "tip_image_combo.h"
#include "rot_memory_map.h"
#include "tip_rom_utils.h"
#include "platform_io.h"


/**
 * Secondary key manifest used during boot flow.
 */
extern struct tip_secondary_key_manifest tip_skmt;


static int tip_image_combo_load_func (const struct firmware_image *fw, const struct flash *flash,
	uint32_t base_addr)
{
	int status;
	struct tip_image_combo *fw_combo = (struct tip_image_combo *) fw;
	uint32_t start_offset;
	uint32_t bmc_timestamp;
	bool bmc_timestamp_equal = false;

	if (fw_combo == NULL || flash == NULL) {
		return FIRMWARE_IMAGE_INVALID_ARGUMENT;
	}

	/* Sanitize the fields of tip_img_combo except the API. */
	memset (fw_combo->img, 0,
		sizeof (struct tip_firmware_component) * TIP_COMBO_MAX_COMPONENT_COUNT);
	memset (&fw_combo->kmt, 0, sizeof (struct tip_primary_key_manifest));
	memset (&fw_combo->skmt, 0, sizeof (struct tip_secondary_key_manifest));
	fw_combo->img_type = IMG_UNKNOWN;

	start_offset = base_addr;

	for (int i = 0; i < TIP_COMBO_MAX_COMPONENT_COUNT; ++i) {
		struct tip_firmware_component *component = &fw_combo->img[i];
		status = tip_firmware_component_init (component, (struct spi_flash *) flash, IMG_UNKNOWN,
			start_offset, true);
		if (status != 0) {
			return status;
		}

		status = tip_firmware_component_get_next_img (component, (struct spi_flash *) flash,
			start_offset);
		if (status != 0) {
			break;
		}

		platform_printf ("%s found %s  addr %#010lx size %#010lx" NEWLINE, __func__,
			tip_firmware_component_get_name (component->img_type),
			(uint32_t) component->header_flash, component->size);

		fw_combo->img_type |= component->img_type;

		/* Update the start offset in flash to scan next img. */
		start_offset = component->base.start_addr + component->base.offset +
					   ROUND_UP (component->size, 0x1000);

		switch (component->img_type) {
			case IMG_KMT:
				status = tip_key_manifest_init (&fw_combo->kmt, component);
				if (status != 0) {
					return status;
				}

				status = tip_key_manifest_parse_and_copy (&fw_combo->kmt,
					(const uint8_t *) component->header_flash, 0, false);
				if (status != 0) {
					return status;
				}
				
				break;

			case IMG_SKMT:
				status = tip_skmt_init (&fw_combo->skmt, component, 0);
				if (status != 0) {
					return status;
				}
				status = tip_skmt_parse (&fw_combo->skmt);
				if (status != 0) {
					return status;
				}
				break;

			case IMG_BOOTBLOCK:
				bmc_timestamp = component->header_flash->header.timestamp;
				bmc_timestamp_equal = true;
				break;

			case IMG_BL31:
			case IMG_OPTEE:
			case IMG_UBOOT:
				if (component->header_flash->header.timestamp != bmc_timestamp) {
					bmc_timestamp_equal = false;
				}
				break;

			default:
				break;
		}
	}

	switch (fw_combo->img_type) {
		case IMG_COMBO0:
			break;

		case IMG_COMBO1: {
			if (!bmc_timestamp_equal) {
				platform_printf (KRED "Combo 1 timestamp check failed" NEWLINE KNRM);
				return FIRMWARE_IMAGE_INVALID_FORMAT;
			}

			platform_printf ("Combo 1 timestamp check OK %#010lx" NEWLINE, bmc_timestamp);

			/* Use current SKMT for Combo1 verify. Copy it to combo structure. */
			memcpy (&fw_combo->skmt, &tip_skmt, sizeof (struct tip_secondary_key_manifest));
			break;
		}

		case IMG_COMBO2: {
			if (!bmc_timestamp_equal) {
				platform_printf (KRED "Combo 2 BMC timestamp check failed" NEWLINE KNRM);
				return FIRMWARE_IMAGE_INVALID_FORMAT;
			}
			break;
		}

		default:
			platform_printf (KRED "Error: update image format invalid. img_type %#010lx"
				NEWLINE KNRM, fw_combo->img_type);
			return FIRMWARE_IMAGE_INVALID_FORMAT;
	}

	return 0;
}

static int tip_image_combo_verify_func (const struct firmware_image *fw, struct hash_engine *hash)
{
	int status = 0;
	struct tip_image_combo *fw_combo = (struct tip_image_combo *) fw;

	if (fw_combo == NULL || hash == NULL) {
		return FIRMWARE_IMAGE_INVALID_ARGUMENT;
	}

	switch (fw_combo->img_type) {
		case IMG_COMBO0:
		case IMG_COMBO1:
		case IMG_COMBO2:
			break;

		default:
			return FIRMWARE_IMAGE_INVALID_FORMAT;
	}

	for (int i = 0; i < TIP_COMBO_MAX_COMPONENT_COUNT; ++i) {
		/* Combo 0 and Combo 1 only have 4 valid FW components. Others has been filled with IMG_UNKNOWN type
		 * when calling load API. Skip verification on UNKNOW components. */
		if (fw_combo->img[i].img_type == IMG_UNKNOWN) {
			continue;
		}
		/* Combo structure carries the correct KMT and SKMT after calling load API.
		 * Use them directly here.*/
		status = tip_firmware_component_verify (&fw_combo->img[i], hash, &fw_combo->kmt,
			&fw_combo->skmt, NULL, 0);
		if (status != 0 && TIP_SECBOOT_IS_ACTIVE ()) {
			return status;
		}
	}

	return status;
}

static int tip_image_combo_get_image_size_func (const struct firmware_image *fw)
{
	struct tip_image_combo *fw_combo = (struct tip_image_combo *) fw;
	uint32_t size = 0;

	if (fw_combo == NULL) {
		return FIRMWARE_IMAGE_INVALID_ARGUMENT;
	}

	switch (fw_combo->img_type) {
		case IMG_COMBO0:
		case IMG_COMBO1:
		case IMG_COMBO2:
			break;

		default:
			return FIRMWARE_IMAGE_INVALID_FORMAT;
	}

	for (int i = 0; i < TIP_COMBO_MAX_COMPONENT_COUNT; ++i) {
		if (fw_combo->img[i].img_type == IMG_UNKNOWN) {
			continue;
		}
		/* In Combo 2 Bootblock starts from a fixed offset 512KB. The padding should be counted in
		 * the final image size. Adjust the calcuated Combo 0 size up to ROT_COMBO0_MAX_SIZE. */
		if (fw_combo->img[i].img_type == IMG_BOOTBLOCK && size > 0) {
			size = ROT_COMBO0_MAX_SIZE;
		}

		size += ROUND_UP (fw_combo->img[i].size, 0x1000);
	}

	return size;
}

static const struct key_manifest *tip_image_combo_get_key_manifest_func (
	const struct firmware_image *fw)
{
	struct tip_image_combo *fw_combo = (struct tip_image_combo *) fw;

	if (fw_combo == NULL) {
		return NULL;
	}

	return (fw_combo->img_type == IMG_COMBO0 ? &fw_combo->kmt.base : &fw_combo->skmt.base);
}

static const struct firmware_header *tip_image_combo_get_firmware_header_empty_func (
	const struct firmware_image *fw)
{
	return NULL;
}

/**
 * Initialize image combo handler (base instance only).
 * Other fields in combo structure will be initialized during load API call.
 *
 * @param fw_combo Image combo handler.
 *
 * @return 0 if success or an error code.
 */
int tip_image_combo_init (struct tip_image_combo *fw_combo)
{
	if (fw_combo == NULL) {
		return FIRMWARE_IMAGE_INVALID_ARGUMENT;
	}

	memset (fw_combo, 0, sizeof (struct tip_image_combo));

	fw_combo->img_type = IMG_UNKNOWN;

	fw_combo->base.load = tip_image_combo_load_func;
	fw_combo->base.verify = tip_image_combo_verify_func;
	fw_combo->base.get_image_size = tip_image_combo_get_image_size_func;
	fw_combo->base.get_key_manifest = tip_image_combo_get_key_manifest_func;
	fw_combo->base.get_firmware_header = tip_image_combo_get_firmware_header_empty_func;

	return 0;
}
