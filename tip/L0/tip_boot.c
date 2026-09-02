// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "platform_api.h"
#include "platform_io.h"
#include "tip_boot.h"
#include "tip_log.h"
#include "tip_rom_utils.h"
#include "serial_printf/serial_printf.h"
#include "tip_utils.h"
#include "rot_memory_map.h"
#include "tip_firmware_component.h"
#include "tip_flash.h"
#include "tip_skmt.h"
#include "tip_security.h"
#include "tip_cmd_device.h"
#include "tip_version.h"
#include "tip_key_manifest.h"
#include "boot_logging.h"
#include "crypto/x509_mbedtls.h"
#include "crypto/base64_mbedtls.h"
#include "riot/riot_core_common.h"
#include "riot/riot_keys.h"
#include "logging/logging_flash.h"
#include "logging/debug_log.h"
#include "tip_hash_ncl.h"
#include "crypto/ecc_ecc_hw.h"
#include "tip_ecc_hw_ncl.h"


/**
 * Version of RIoT Core. This must be incremented with every change to the bootloader.
 */
static const char *version = "0.6.2";

/* Stuff from TIP_ROM */
static BOOT_LOG_TABLE_T BOOT_log_table __attribute__ ((section (".boot_log")));

/**
 * Handler for the main flash.
 */
static struct spi_flash *main_flash;

/**
 * Handler for the recovery flash.
 */
static struct spi_flash *recovery_flash;

/**
 * Handler for the active flash.
 *
 * Active flash is where the running fw image is loaded from.
 * It can be either main flash or recovery flash.
 */
static struct spi_flash *active_flash;

/**
 * Flag indicating the system booted from the recovery flash.
 */
static bool recovery_boot;

/**
 * Offset of recovery image location.
 */
static uint32_t recovery_flash_start_offset;

/**
 * Primary key manifest.
 */
static struct tip_primary_key_manifest tip_kmt;

/**
 * Secondary key manifest.
 */
static struct tip_secondary_key_manifest tip_skmt;

/**
 * SVN version handler.
 */
struct tip_version_handler tip_version;

/**
 * The command handler for device operations.
 */
static struct tip_cmd_device tip_cmd_device;

/**
 * Variable context for the debug log.
 */
static struct logging_flash_state debug_log_context;

/**
 * Flash logger for storing boot debug messages.
 */
static struct logging_flash debug_logger;

/**
 * Engine for system_hash operations.
 */
struct tip_hash_ncl_engine system_hash;

/**
 * Engine for ECC operations.
 */
static struct tip_ecc_hw_ncl_engine hw_ecc;

/**
 * HW Engine for ECC operations.
 */
static struct ecc_engine_ecc_hw ecc;

/**
 * Engine for X.509 certificate operations.
 */
static struct x509_engine_mbedtls x509;

/**
 * Engine for base64 encoding.
 */
static struct base64_engine_mbedtls base64;

/**
 * Variable context for RIoT Core.
 */
static struct riot_core_common_state riot_context;

/**
 * RIoT Core implementation.
 */
static struct riot_core_common riot;

/**
 * TCB info for RIoT Core.
 */
static struct x509_dice_tcbinfo riot_tcb;

/**
 * UEID info for RIoT Core.
 */
static struct x509_dice_ueid riot_ueid;

/**
 * TCB info for the main image.
 */
static struct x509_dice_tcbinfo alias_tcb;

/**
 * The UUID for the device.
 */
static uint8_t UUID[TIP_UID_LEN];

/**
 * Shared attestation information for the main application.
 */
static struct riot_shared_attestation *attestation = (struct riot_shared_attestation *)
	RIOT_SHARED_ADDRESS;

/**
 * The location where the CDI is stored by the ROM.
 */
static uint8_t *DICE_CDI;

/**
 * KMT copy in RAM pointer
 */
static const uint8_t *verified_manifest = (uint8_t *) MANIFEST_KEY_ADDRESS;

/**
 * TIP fw L1 start address
 */
static uint32_t cerberus_fw_start = 0x20000;

/**
 * Compute the ptr to DICE CDI data according to chip revision
 *
 * @return ptr to DICE CDI
 */
static uint8_t *tip_get_dice_cdi_ptr (void)
{
	extern uint32_t __dme_dice_table_start;
	SEC_DME_DICE_T_A2 *dice_a2 = (SEC_DME_DICE_T_A2 *) &__dme_dice_table_start;
	SEC_DME_DICE_T_A1 *dice_a1 = (SEC_DME_DICE_T_A1 *) &__dme_dice_table_start;
	SEC_DME_DICE_T_Z1 *dice_z1 = (SEC_DME_DICE_T_Z1 *) &__dme_dice_table_start;
	uint32_t chip_revision = CHIP_Get_Version ();

	switch (chip_revision) {
		case ARBEL_VERSION_Z1:
			return dice_z1->cdi;

		case ARBEL_VERSION_A1:
			return dice_a1->cdi;

		/* A2 and above */
		case ARBEL_VERSION_A2:
		default:
			return dice_a2->cdi;
	}
}

/**
 * Compute the extended measurement that will be used as X.509 DICE TCB fw id hash.
 *
 * @param hash The hash engine to utilize.
 * @param buf1 Input buffer for raw measured data
 * @param len1 The length of buf1
 * @param buf2 Input buffer for raw measured data
 * @param len2 The length of raw measured data 2 buffer
 * @param [out] tcb_fw_hash Output buffer for the extended measurements
 * @param out_len The length of output buffer
 *
 * @return 0 if executes successfully or an error code
 */
static int tip_compute_tcb_fwid (struct hash_engine *hash, uint8_t *buf1, size_t len1,
	uint8_t *buf2, size_t len2, uint8_t *tcb_fw_hash, size_t out_len)
{
	int status;
	uint8_t digest1[SHA256_HASH_LENGTH] = { 0 };
	uint8_t digest2[SHA256_HASH_LENGTH] = { 0 };
	uint8_t extended_measurement[SHA256_HASH_LENGTH] = { 0 };

	if (!hash || !buf1 || !buf2 || !tcb_fw_hash || out_len < SHA256_HASH_LENGTH) {
		return -1;
	}

	status = hash->calculate_sha256 (hash, buf1, len1, digest1, SHA256_HASH_LENGTH);
	if (status != 0) {
		goto exit;
	}

	status = hash->calculate_sha256 (hash, buf2, len2, digest2, SHA256_HASH_LENGTH);
	if (status != 0) {
		goto exit;
	}

	/* Calculate extended measurement and use it as X.509 DICE TCB fwid. */
	for (uint8_t i = 0; i < 2; ++i) {
		status = hash->start_sha256 (hash);
		if (status != 0) {
			goto exit;
		}

		status = hash->update (hash, extended_measurement, SHA256_HASH_LENGTH);
		if (status != 0) {
			goto hash_cancel;
		}

		status = hash->update (hash, (i == 0 ? digest1 : digest2), SHA256_HASH_LENGTH);
		if (status != 0) {
			goto hash_cancel;
		}

		status = hash->finish (hash, extended_measurement, SHA256_HASH_LENGTH);
		if (status != 0) {
			goto hash_cancel;
		}
	}

	/* Copy extended measurement to the ouput buffer */
	memcpy (tcb_fw_hash, extended_measurement, SHA256_HASH_LENGTH);

	return status;

hash_cancel:
	hash->cancel (hash);

exit:
	return status;
}

/**
 * Load the main application from the active flash.
 *
 * @param flash The flash device that contains the active image.
 *
 * @return 0 if the main application was loaded successfully or an error code.
 */
static int load_main_application (struct spi_flash *flash)
{
	int status;
	struct tip_firmware_component fw;
	struct tip_firmware_component skmt_img;
	int start_offset = recovery_boot ? recovery_flash_start_offset : 0;
	uint8_t x509_riot_tcb_fwid[SHA256_HASH_LENGTH] = { 0 };
	uint8_t x509_alias_tcb_fwid[SHA256_HASH_LENGTH] = { 0 };

	platform_printf (KCYN NEWLINE "==========" NEWLINE "Load SKMT" NEWLINE "=========="
		NEWLINE KNRM);

	/* Initialize SKMT firmware component instance  */
	status = tip_firmware_component_init (&skmt_img, flash, IMG_SKMT, start_offset);
	if (status != 0) {
		goto exit;
	}

	status = tip_firmware_component_load_to_memory (&skmt_img);
	if (status != 0) {
		goto exit;
	}

	status = tip_firmware_component_verify (&skmt_img, &system_hash.base, &tip_kmt, NULL,
		attestation->skmt_hash, sizeof (attestation->skmt_hash));
	if (status != 0 && TIP_SECBOOT_IS_ACTIVE ()) {
		platform_printf (KRED "SKMT failed authentication" NEWLINE KNRM);
		goto exit;
	}

	status = tip_skmt_init (&tip_skmt, &skmt_img, 0);
	if (status != 0) {
		goto exit;
	}

	status = tip_skmt_copy (&tip_skmt, MANIFEST_KEY_ADDRESS_COPY);
	if (status != 0) {
		goto exit;
	}

	/* KMT is not there anymore.Invalidte it */
	tip_kmt.ram_copy_valid = false;

	status = tip_skmt_parse (&tip_skmt);
	if (status != 0) {
		goto exit;
	}

	platform_printf (KCYN NEWLINE "==========" NEWLINE "Load TIP_FW L1" NEWLINE "=========="
		NEWLINE KNRM);

	/* Initialize firmware component instance  */
	status = tip_firmware_component_init (&fw, flash, IMG_TFT_L1, start_offset);
	if (status != 0) {
		goto exit;
	}

	status = tip_firmware_component_load_to_memory (&fw);
	if (status != 0) {
		goto exit;
	}

	status = tip_firmware_component_verify (&fw, &system_hash.base, NULL, &tip_skmt,
		attestation->app_hash, sizeof (attestation->app_hash));
	if (status != 0 && TIP_SECBOOT_IS_ACTIVE ()) {
		platform_printf (KRED "L1 failed authentication" NEWLINE KNRM);
		goto exit;
	}

	status = tip_compute_tcb_fwid (&system_hash.base, attestation->keys_hash,
		sizeof (attestation->keys_hash), attestation->riot_hash, sizeof (attestation->riot_hash),
		x509_riot_tcb_fwid, sizeof (x509_riot_tcb_fwid));
	if (status != 0) {
		goto exit;
	}

	riot_tcb.version = version;
	riot_tcb.fw_id = x509_riot_tcb_fwid;
	riot_tcb.fw_id_hash = HASH_TYPE_SHA256;
	riot_tcb.ueid = &riot_ueid;

	status = riot.base.generate_device_id (&riot.base, DICE_CDI, SHA512_HASH_LENGTH, &riot_tcb);
	if (status != 0) {
		goto exit;
	}

	/* Clear CDI once it's not needed anymore */
	riot_core_clear ((void *) DICE_CDI, SHA512_HASH_LENGTH);

	status = tip_compute_tcb_fwid (&system_hash.base, attestation->skmt_hash,
		sizeof (attestation->skmt_hash), attestation->app_hash, sizeof (attestation->app_hash),
		x509_alias_tcb_fwid, sizeof (x509_riot_tcb_fwid));
	if (status != 0) {
		goto exit;
	}

	alias_tcb.version = CERBERUS_FW_VERSION_STRING;

	/* Get l1 svn from TIP svn handler */
	alias_tcb.svn = (uint32_t) tip_version.tip_fw_l1_version;
	alias_tcb.fw_id = x509_alias_tcb_fwid;
	alias_tcb.fw_id_hash = HASH_TYPE_SHA256;
	alias_tcb.ueid = NULL;

	status = riot.base.generate_alias_key (&riot.base, &alias_tcb);

exit:
	return status;
}

/**
 * Copy RIoT data to the shared memory location.
 *
 * @param dest The destination for the attestation data.
 * @param dest_length The maximum length that can be copied.
 * @param length Output for the length of the copied data.
 * @param src The RIoT data to copy.
 * @param src_length The RIoT data length.
 */
static void copy_riot_data (uint8_t *dest, size_t dest_length, int *length, const uint8_t *src,
	size_t src_length)
{
	if (src_length <= dest_length) {
		memcpy (dest, src, src_length);
		*length = src_length;
	} else {
		*length = -1;
	}
}

/**
 * Copy KMT and L0 hash calculated by ROM to shared attestation.
 *
 * This should be called before KMT overruns the TIP log.
 */
static void tip_copy_hash_from_boot_log (void)
{
	memcpy (attestation->keys_hash, BOOT_log_table.img_logs[BOOT_IMG_KMT].img_hash,
		sizeof (attestation->keys_hash));
	memcpy (attestation->riot_hash, BOOT_log_table.img_logs[BOOT_IMG_TFT].img_hash,
		sizeof (attestation->riot_hash));
}

/**
 * Store the keys from RIoT Core to a memory location that can be accessed by the loaded
 * application.
 *
 * Upon returning, the RIoT Core will be released.  This will be true regardless of whether the
 * operation was successful or not.
 *
 * @return 0 if the RIoT Core keys and certificates were saved successfully or an error code.
 */
static int save_riot_keys ()
{
	uint8_t *der;
	size_t length;
	int status;
	uint32_t OID = REG_READ (PDID);

	status = riot.base.get_device_id_cert (&riot.base, &der, &length);
	if (status != 0) {
		goto riot_error;
	}

	copy_riot_data (attestation->devid_cert, sizeof (attestation->devid_cert),
		&attestation->devid_cert_length, der, length);
	platform_free (der);

	status = riot.base.get_device_id_csr (&riot.base, (char *) &OID, &der, &length);
	if (status != 0) {
		goto riot_error;
	}

	copy_riot_data (attestation->devid_csr, sizeof (attestation->devid_csr),
		&attestation->devid_csr_length, der, length);
	platform_free (der);

	status = riot.base.get_alias_key (&riot.base, &der, &length);
	if (status != 0) {
		goto riot_error;
	}

	copy_riot_data (attestation->alias_key, sizeof (attestation->alias_key),
		&attestation->alias_key_length, der, length);
	platform_free (der);

	status = riot.base.get_alias_key_cert (&riot.base, &der, &length);
	if (status != 0) {
		goto riot_error;
	}

	copy_riot_data (attestation->alias_cert, sizeof (attestation->alias_cert),
		&attestation->alias_cert_length, der, length);
	platform_free (der);

	strcpy (attestation->riot_version, version);

	/* Calculate the hash of all the keys and the certificates shared with the main application. */
	status = system_hash.base.calculate_sha256 (&system_hash.base, (uint8_t *) attestation,
		sizeof (struct riot_shared_attestation) - sizeof (attestation->attestation_hash),
		attestation->attestation_hash, sizeof (attestation->attestation_hash));

riot_error:
	riot_core_common_release (&riot);
	return status;
}

/**
 * Cerberus bootloader entry point.
 */
int main (void)
{
	int status = 0;
	int error_msg = -1;
	struct tip_firmware_component kmt_img;

	hardware_init ();
	tip_print_status (version);
	tip_init_rom_ncl ();

	memset (attestation, 0, sizeof (struct riot_shared_attestation));

	DICE_CDI = tip_get_dice_cdi_ptr ();

	/* Initialize core system components. */
	status = tip_flash_initialize_access (FLASH_MAX_FIU, FLASH_MAX_CS);
	if (status != 0) {
		error_msg = BOOT_LOGGING_FLASH_INIT;
		goto reset;
	}

	status = tip_flash_get_layout (FLASH_MAX_FIU, FLASH_MAX_CS, &main_flash, &recovery_flash,
		&active_flash, &recovery_flash_start_offset);
	if (status != 0) {
		error_msg = BOOT_LOGGING_FLASH_LAYOUT;
		goto reset;
	}

	recovery_boot = tip_check_recovery_boot ();

	/**
	 * Init debug log.
	 * Debug log is 64KB (one flash block).
	 * Debug log location is
	 * - one flash mode: hard coded address in rot_memory_map.h
	 * - two flash mode: last block in recovery flash.
	 */
	if (recovery_flash_start_offset > 0) {
		/* single flash mode */
		status = logging_flash_init (&debug_logger, &debug_log_context, recovery_flash,
			DEBUG_LOG_OFFSET_SINGLE_FLASH);
	} else {
		/* dual flash mode */
		status = logging_flash_init (&debug_logger, &debug_log_context, recovery_flash,
			recovery_flash->state->device_size - _64KB_);
	}

	if (status == 0) {
		debug_log = &debug_logger.base;
	} else {
		platform_printf ("Failed to initialize debug logging module: %#010lx" NEWLINE, status);
	}

	status = tip_version_init (&tip_version);
	if (status != 0) {
		error_msg = BOOT_LOGGING_SVN_ERROR;
		goto reset;
	}

	status = tip_cmd_device_init (&tip_cmd_device);
	if (status != 0) {
		error_msg = BOOT_LOGGING_CMD_DEVICE;
		goto reset;
	}

	status = tip_cmd_device.base.get_uuid (&tip_cmd_device.base, UUID, sizeof (UUID));
	if (status < 0) {
		error_msg = BOOT_LOGGING_GET_UUID;
		goto reset;
	}

	riot_ueid.ueid = UUID;
	riot_ueid.length = status;

	status = tip_hash_ncl_init (&system_hash);
	if (status != 0) {
		platform_printf ("Failed to initialize hash engine: %#010lx" NEWLINE, status);
		error_msg = BOOT_LOGGING_INIT_HASH;
		goto reset;
	}
	
	status = tip_ecc_hw_ncl_init (&hw_ecc);
	if (status != 0) {
		return status;
	}
	status = ecc_ecc_hw_init (&ecc, &hw_ecc.base);
	if (status != 0) {
		platform_printf ("Failed to initialize ECC engine: %#010lx" NEWLINE, status);
		error_msg = BOOT_LOGGING_INIT_ECC;
		goto reset;
	}

	status = x509_mbedtls_init (&x509);
	if (status != 0) {
		error_msg = BOOT_LOGGING_INIT_X509;
		goto reset;
	}

	status = base64_mbedtls_init (&base64);
	if (status != 0) {
		error_msg = BOOT_LOGGING_INIT_BASE64;
		goto reset;
	}

	status = riot_core_common_init (&riot, &riot_context, &system_hash.base, &ecc.base, &x509.base,
		&base64.base, ECC_KEY_LENGTH_256);
	if (status != 0) {
		error_msg = BOOT_LOGGING_INIT_RIOT_CORE;
		goto reset;
	}

	/* Do not change the order of the following lines (KMT will overwrite the boot log). */
	tip_copy_hash_from_boot_log ();

	/* Initialize KMT img component before key manifest init.
	 * No need to load and verify as ROM have already done. */
	status = tip_firmware_component_init (&kmt_img, active_flash, IMG_KMT, -1);
	if (status != 0) {
		error_msg = BOOT_LOGGING_FW_COMPONENT_INIT;
		goto reset;
	}

	status = tip_key_manifest_init (&tip_kmt, &kmt_img);
	if (status != 0) {
		error_msg = BOOT_LOGGING_TIP_KMT_INIT;
		goto reset;
	}

	status = tip_key_manifest_parse_and_copy (&tip_kmt, verified_manifest,
		MANIFEST_KEY_ADDRESS_COPY);
	if (status != 0) {
		error_msg = BOOT_LOGGING_TIP_KMT_PARSE;
		goto reset;
	}

	status = load_main_application (active_flash);
	if (status != 0) {
		error_msg = BOOT_LOGGING_APP_LOAD;
		goto riot_reset;
	}

	status = save_riot_keys ();
	if (status != 0) {
		error_msg = BOOT_LOGGING_RIOT_KEYS;
		goto reset;
	}

	debug_log_flush ();
	platform_printf (KNRM "Jump to L1 image at %#010lx" NEWLINE, cerberus_fw_start);
	JUMP_TO_ADDRESS (cerberus_fw_start + sizeof (HEADER_GENERAL_T) + 1);

riot_reset:
	riot_core_common_release (&riot);

reset:
	riot_core_clear ((void *) DICE_CDI, 64);

	if (error_msg >= 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_BOOT, error_msg,
			status, 0);
		platform_printf ("System initialization failed: msg=%d, status = %#010lx" NEWLINE, error_msg,
			status);
	} else {
		platform_printf ("System initialization failed: status = %#010lx" NEWLINE, status);
	}

	debug_log_flush ();

	/* Never halt the system. Reboot and try again. */
	platform_printf (NEWLINE);
	platform_reset (0);
}
