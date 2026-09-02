// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "asn1/base64_core.h"
#include "asn1/dice/x509_extension_builder_dice_tcbinfo.h"
#include "asn1/dice/x509_extension_builder_dice_ueid.h"
#include "asn1/dme/x509_extension_builder_dme.h"
#include "asn1/dme/dme_structure_raw_ecc_le.h"
#include "asn1/x509_cert_build.h"
#include "crypto/ecc_ecc_hw.h"
#include "logging/logging_flash.h"
#include "logging/debug_log.h"
#include "riot/riot_keys.h"
#include "serial_printf/serial_printf.h"
#include "boot_logging.h"
#include "build_version.h"
#include "platform_api.h"
#include "platform_io.h"
#include "riot_core_tip.h"
#include "rot_memory_map.h"
#include "tip_aes_ncl.h"
#include "tip_boot.h"
#include "tip_cmd_device.h"
#include "tip_dice_oid.h"
#include "tip_ecc_hw_ncl.h"
#include "tip_firmware_component.h"
#include "tip_flash.h"
#include "tip_hash_ncl.h"
#include "tip_key_manifest.h"
#include "tip_log.h"
#include "tip_rom_utils.h"
#include "tip_security.h"
#include "tip_skmt.h"
#include "tip_utils.h"
#include "tip_version.h"
#include "tip_virtual_flash.h"
#include "tip_reset.h"


/**
 * Version of RIoT Core. This must be incremented with every change to the bootloader.
 */
static const char *version = "0.7.4";

/* Stuff from TIP_ROM */
static BOOT_LOG_TABLE_T BOOT_log_table __attribute__ ((section (".boot_log")));

/**
 * Handler for the main flash.
 */
static struct spi_flash *main_flash;

/**
 * Handler for the recovery flash.
 */
struct spi_flash *recovery_flash;

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
struct spi_flash virtual_flash;
static struct tip_flash_master_virtual virtual_flash_master;
static struct spi_flash_state virtual_flash_context;

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
struct tip_version_handler *tip_version =
	(struct tip_version_handler*) TIP_VERSION_SHARED_ADDRESS;

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
struct tip_aes_ncl_engine session_aes;

/**
 * HW Engine for ECC operations.
 */
static struct ecc_engine_ecc_hw ecc;

/**
 * Engine for X.509 certificate operations.
 */
static struct x509_engine_cert_build x509;

/**
 * Engine for base64 encoding.
 */
static struct base64_engine_core base64;

/**
 * Variable context for TIP RIoT Core.
 */
static struct riot_core_common_state riot_context;

/**
 * TIP RIoT Core implementation.
 */
static struct riot_core_tip riot;

/**
 *  FWID within RIoT Core TCB.
 */
static uint8_t x509_riot_tcb_fwid[SHA512_HASH_LENGTH];

/**
 *  FWID within main image TCB.
 */
static uint8_t x509_alias_tcb_fwid[SHA384_HASH_LENGTH];

/**
 * TCB info for RIoT Core.
 */
static struct tcg_dice_tcbinfo riot_tcb;

/**
 * TCB info for the main image.
 */
static struct tcg_dice_tcbinfo alias_tcb;

/**
 * Buffer for building the TcbInfo, UEID and DME extensions.  This can be shared by all extension builders since
 * only one will execute at a time.
 */
static uint8_t ext_buffer[1024];

/**
 * Extension builder for the RIoT Core TcbInfo extension.
 */
static struct x509_extension_builder_dice_tcbinfo riot_tcb_ext;

/**
 * Extension builder for the Alias TcbInfo extension.
 */
static struct x509_extension_builder_dice_tcbinfo alias_tcb_ext;

/**
 * DME info for TIP and boot context.
 */
static struct dme_structure_raw_ecc_le dme;

/**
 *  DME struct data (Type 7) for TIP.
 */
static struct tip_dme_struct_data_a2 dme_struct_data;

/**
 * Extension builder for DME extension for L0 certificate and CSR.
 */
static struct x509_extension_builder_dme l0_dme_ext;

/**
 * The UUID for the device.
 */
static uint8_t UUID[TIP_UID_LEN];

/**
 * Extension builder for the device UEID extension.
 */
static struct x509_extension_builder_dice_ueid riot_ueid_ext;

/**
 * List of extensions for the Device ID certificate and CSR.
 */
static const struct x509_extension_builder *const riot_ext[] = {
	&riot_tcb_ext.base, &riot_ueid_ext.base, &l0_dme_ext.base
};

/**
 * List of extensions for the Alias certificate.
 */
static const struct x509_extension_builder *const alias_ext[] = { &alias_tcb_ext.base };

/**
 * Shared attestation information for the main application.
 */
static struct riot_shared_attestation *attestation =
	(struct riot_shared_attestation*) RIOT_SHARED_ADDRESS;

/**
 * The location where the CDI is stored by the ROM.
 */
static uint8_t *DICE_CDI;

/**
 * The location where DICE_PCR0 is stored by the ROM.
 */
static uint8_t *DICE_PCR0;

/**
 * KMT copy in RAM pointer
 */
static const uint8_t *verified_manifest = (uint8_t*) MANIFEST_KEY_ADDRESS;

/**
 * TIP fw L1 start address
 */
static uint32_t cerberus_fw_start = 0x20000;

/**
 * Parse the DME/DICE data that TIP ROM saves to shared RAM.
 *
 * @param [in/out] dme_dice_ptr Pointer to the DME/DICE data.
 * @param [in/out] dme_struct_data Pointer to the DME struct data.
 * @param [out] cdi Pointer to the CDI data.
 * @param [out] dice_pcr0 Pointer to the DICE PCR0 data.
 * @return 0 if DME/DICE data is parsed successfully or an error code.
 */
static int tip_parse_rom_dme_dice_data (SEC_DME_DICE_T_PTR *dme_dice_ptr,
	struct tip_dme_struct_data_a2 *dme_struct_data, uint8_t **cdi, uint8_t **dice_pcr0)
{
	/* Starting address of DME/DICE data that TIP ROM saves to RAM. */
	extern uint32_t __dme_dice_table_start;
	uint32_t chip_revision;

	if (dme_dice_ptr == NULL || cdi == NULL || dice_pcr0 == NULL) {
		return -1;
	}

	dme_dice_ptr->z1 = (SEC_DME_DICE_T_Z1*) &__dme_dice_table_start;
	dme_dice_ptr->a1 = (SEC_DME_DICE_T_A1*) &__dme_dice_table_start;
	dme_dice_ptr->a2 = (SEC_DME_DICE_T_A2*) &__dme_dice_table_start;

	chip_revision = CHIP_Get_Version ();
	switch (chip_revision) {
		case ARBEL_VERSION_Z1:
			*cdi = dme_dice_ptr->z1->cdi;
			*dice_pcr0 = dme_dice_ptr->z1->dice_pcr0;
			break;

		case ARBEL_VERSION_A1:
			*cdi = dme_dice_ptr->a1->cdi;
			*dice_pcr0 = dme_dice_ptr->a1->dice_pcr0;
			break;

		case ARBEL_VERSION_A2:
			*cdi = dme_dice_ptr->a2->cdi;
			*dice_pcr0 = dme_dice_ptr->a2->dice_pcr0;
			if (dme_struct_data != NULL) {
				memcpy (dme_struct_data->dme_nonce, dme_dice_ptr->a2->dme_nonce,
					SEC_DME_NONCE_LENGTH);
				memcpy (dme_struct_data->dme_challenge, dme_dice_ptr->a2->dme_challenge,
					SEC_DME_CHALLENGE_LENGTH);
				memcpy (dme_struct_data->dice_pub_key, dme_dice_ptr->a2->dice_pub_key,
					SEC_PUB_KEY_SIZE);
				memcpy (dme_struct_data->dme_pcr0, dme_dice_ptr->a2->dme_pcr0, SEC_PCR0_LENGTH);
#if TIP_DUMMY_OTP_ON_RAM
				/* Copy the cdi into the footer of the mailbox for test purposes*/
				memcpy ((uint8_t *) (PCIMBX_BASE_ADDR (0) + _16KB_ - SHA512_HASH_LENGTH),
					dme_dice_ptr->a2->cdi, SHA512_HASH_LENGTH);
#endif
			}
			break;

		default:
			return -1;
	}

	return 0;
}

/**
 * Initialize the components necessary to execute TIP RIoT Core functionality.
 *
 * @return 0 if initialized successfully or an error code.
 */
static int initialize_dice ()
{
	SEC_DME_DICE_T_PTR dme_dice_ptr;
	int status;
	/* DICE TCB and UEID extension are supported by all chip revisions.
	 * DME extension is supported from A2. */
	size_t riot_ext_count = 2;

	status = tip_parse_rom_dme_dice_data (&dme_dice_ptr, &dme_struct_data,
		&DICE_CDI, &DICE_PCR0);
	if (status != 0) {
		return status;
	}

	status = x509_extension_builder_dice_tcbinfo_init_with_buffer (&riot_tcb_ext, &riot_tcb,
		ext_buffer, sizeof (ext_buffer));
	if (status != 0) {
		return status;
	}

	status = x509_extension_builder_dice_tcbinfo_init_with_buffer (&alias_tcb_ext,
		&alias_tcb, ext_buffer, sizeof (ext_buffer));
	if (status != 0) {
		return status;
	}

	status = x509_extension_builder_dice_ueid_init_with_buffer (&riot_ueid_ext,
		(uint8_t*) UUID, sizeof (UUID), ext_buffer, sizeof (ext_buffer));
	if (status != 0) {
		return status;
	}

	/* Support the inclusion of DME extension into CSR only from A2. */
	if (CHIP_Get_Version () == ARBEL_VERSION_A2) {
		status = x509_extension_builder_dme_init_with_buffer (&l0_dme_ext, &dme.base.base,
			ext_buffer, sizeof (ext_buffer));
		if (status != 0) {
			return status;
		}

		status = dme_structure_raw_ecc_le_init_le_ecc384_with_sha512_nonce_and_challenge (&dme,
			(const uint8_t*) &dme_struct_data, sizeof (dme_struct_data),
			dme_dice_ptr.a2->dme_pub_key, dme_dice_ptr.a2->dme_pub_key + ECC_KEY_LENGTH_384,
			ECC_KEY_LENGTH_384, dme_dice_ptr.a2->dme_signature,
			dme_dice_ptr.a2->dme_signature + ECC_KEY_LENGTH_384, HASH_TYPE_SHA512);
		if (status != 0) {
			return status;
		}

		riot_ext_count = ARRAY_SIZE (riot_ext);
	}

	status = riot_core_tip_init (&riot, &riot_context, &system_hash.base, &ecc.base, &x509.base,
		&base64.base, ECC_KEY_LENGTH_384, riot_ext, riot_ext_count, alias_ext,
		ARRAY_SIZE (alias_ext));

	return status;
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
	uint8_t digest1[SHA384_HASH_LENGTH] = { 0 };
	uint8_t digest2[SHA384_HASH_LENGTH] = { 0 };
	uint8_t extended_measurement[SHA384_HASH_LENGTH] = { 0 };

	if (!hash || !buf1 || !buf2 || !tcb_fw_hash || out_len < SHA384_HASH_LENGTH) {
		return -1;
	}

	status = hash->calculate_sha384 (hash, buf1, len1, digest1, SHA384_HASH_LENGTH);
	if (status != 0) {
		goto exit;
	}

	status = hash->calculate_sha384 (hash, buf2, len2, digest2, SHA384_HASH_LENGTH);
	if (status != 0) {
		goto exit;
	}

	/* Calculate extended measurement and use it as X.509 DICE TCB fwid. */
	for (uint8_t i = 0; i < 2; ++i) {
		status = hash->start_sha384 (hash);
		if (status != 0) {
			goto exit;
		}

		status = hash->update (hash, extended_measurement, SHA384_HASH_LENGTH);
		if (status != 0) {
			goto hash_cancel;
		}

		status = hash->update (hash, (i == 0 ? digest1 : digest2), SHA384_HASH_LENGTH);
		if (status != 0) {
			goto hash_cancel;
		}

		status = hash->finish (hash, extended_measurement, SHA384_HASH_LENGTH);
		if (status != 0) {
			goto hash_cancel;
		}
	}

	/* Copy extended measurement to the ouput buffer */
	memcpy (tcb_fw_hash, extended_measurement, SHA384_HASH_LENGTH);

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
 * @param reset Reset type indication. Composite of RESET_TYPE_T
 *
 * @return 0 if the main application was loaded successfully or an error code.
 */
static int load_main_application (struct spi_flash *flash, uint16_t reset)
{
	int status;
	struct tip_firmware_component fw;
	struct tip_firmware_component skmt_img;
	uint32_t start_offset = recovery_boot ? recovery_flash_start_offset : 0;

	/* Scan for KMT image on flash.
	 * before searching for SKMT need to search for KMT and L0.
	 * once TIP finds the start addresses and sizes TIP can skip directly to SKMT.
	 * This is both efficient and prevents finding the SKMT tag itself inside L0 image.
	 */
	status = tip_firmware_component_init (&fw, flash, IMG_KMT, start_offset, true);

	if (status != 0) {
		goto exit;
	}

	tip_version_set (tip_version, fw.header_flash->header.version, fw.header_flash->header.next_version, IMG_KMT);
	/* scan for L0 firmware on flash  */
	start_offset = fw.base.start_addr + fw.size;
	status = tip_firmware_component_init (&fw, flash, IMG_TFT_L0, start_offset, true);
	if (status != 0) {
		goto exit;
	}
	tip_version_set (tip_version,  fw.header_flash->header.version, fw.header_flash->header.next_version , IMG_TFT_L0);

	platform_printf (KCYN NEWLINE "==========" NEWLINE "Load SKMT" NEWLINE "=========="
		NEWLINE KNRM);

	/* Initialize SKMT firmware component instance  */
	start_offset = fw.base.start_addr + fw.size;
	status = tip_firmware_component_init (&skmt_img, flash, IMG_SKMT, start_offset, true);

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
	start_offset = skmt_img.base.start_addr + skmt_img.size;
	status = tip_firmware_component_init (&fw, flash, IMG_TFT_L1, start_offset, true);

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

	tip_firmware_component_load_external_reg_table (&fw, reset);

	/* Use DICE PCR0 from ROM as DICE TCB FWID. */
	memcpy (x509_riot_tcb_fwid, DICE_PCR0, SHA512_HASH_LENGTH);

	riot_tcb.version = version;
	riot_tcb.fwid = x509_riot_tcb_fwid;
	riot_tcb.fwid_hash = HASH_TYPE_SHA512;

	status = riot.base.base.generate_device_id (&riot.base.base, DICE_CDI, SHA512_HASH_LENGTH);
	if (status != 0) {
		goto exit;
	}
	
#if  (TIP_DUMMY_OTP_ON_RAM == 0) 
	/* Clear CDI once it's not needed anymore and only in case the debug flag is not set*/
	riot_core_clear ((void*) DICE_CDI, SHA512_HASH_LENGTH);
#endif
	status = tip_compute_tcb_fwid (&system_hash.base, attestation->skmt_hash,
		sizeof (attestation->skmt_hash), attestation->app_hash, sizeof (attestation->app_hash),
		x509_alias_tcb_fwid, sizeof (x509_alias_tcb_fwid));
	if (status != 0) {
		goto exit;
	}

	alias_tcb.version = CERBERUS_FW_VERSION_STRING;

	/* Get l1 svn from TIP svn handler */
	alias_tcb.svn = (uint32_t) tip_version->tip_fw_l1_version;
	alias_tcb.fwid = x509_alias_tcb_fwid;
	alias_tcb.fwid_hash = HASH_TYPE_SHA384;

	status = riot.base.base.generate_alias_key (&riot.base.base, alias_tcb.fwid, alias_tcb.fwid_hash);

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

	status = riot.base.base.get_device_id_cert (&riot.base.base, &der, &length);
	if (status != 0) {
		goto riot_error;
	}

	copy_riot_data (attestation->devid_cert, sizeof (attestation->devid_cert),
		&attestation->devid_cert_length, der, length);
	platform_free (der);

	status = riot.base.base.get_device_id_csr (&riot.base.base, TIP_DICE_OID, TIP_DICE_OID_LENGTH,
		&der, &length);
	if (status != 0) {
		goto riot_error;
	}

	copy_riot_data (attestation->devid_csr, sizeof (attestation->devid_csr),
		&attestation->devid_csr_length, der, length);
	platform_free (der);

	status = riot.base.base.get_alias_key (&riot.base.base, &der, &length);
	if (status != 0) {
		goto riot_error;
	}

	copy_riot_data (attestation->alias_key, sizeof (attestation->alias_key),
		&attestation->alias_key_length, der, length);
	platform_free (der);

	status = riot.base.base.get_alias_key_cert (&riot.base.base, &der, &length);
	if (status != 0) {
		goto riot_error;
	}

	copy_riot_data (attestation->alias_cert, sizeof (attestation->alias_cert),
		&attestation->alias_cert_length, der, length);
	platform_free (der);

	strncpy (attestation->riot_version, version , sizeof (attestation->riot_version) - 1);
	attestation->riot_version[sizeof (attestation->riot_version) - 1] = '\0';

	/* Calculate the hash of all the keys and the certificates shared with the main application. */
	status = system_hash.base.calculate_sha256 (&system_hash.base, (uint8_t*) attestation,
		sizeof (struct riot_shared_attestation) - sizeof (attestation->attestation_hash),
		attestation->attestation_hash, sizeof (attestation->attestation_hash));

riot_error:
	riot_core_tip_release (&riot);
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
	uint16_t reset_source;

	hardware_init ();

#if TIP_DUMMY_OTP_ON_RAM
	platform_printf (KCYN "clean RAM3 for testing" NEWLINE KNRM);
	memset ((uint8_t *) RAM3_BASE_ADDR, 0, RAM3_MEMORY_SIZE);
#endif

	tip_print_status (version);
	reset_source = tip_get_reset_indication ();
	status = tip_init_rom_ncl ();
	if (status != 0) {
		error_msg = BOOT_LOGGING_TIP_NCL_INIT;
		goto reset;
	}

	memset (attestation, 0, sizeof (struct riot_shared_attestation));

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
	 */
	status = logging_flash_init (&debug_logger, &debug_log_context, recovery_flash,
		DEBUG_LOG_FLASH_ADDR(recovery_flash));

	if (status == 0) {
		debug_log = &debug_logger.base;
	} else {
		platform_printf ("Failed to initialize debug logging module: %#010lx" NEWLINE, status);
	}

	status = tip_initialize_flash_access_virtual (&virtual_flash, &virtual_flash_context,
		&virtual_flash_master.base, TIP_VIRTUAL_FLASH_BASE_ADDR, ROT_STAGING_SIZE);
	if (status != 0) {
		error_msg = BOOT_LOGGING_VIRTUAL_FLASH;
		goto reset;
	}

	status = tip_version_init (tip_version);
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

	status = x509_cert_build_init (&x509, &ecc.base, &system_hash.base, MAX_DEVID_CERT_LENGTH);
	if (status != 0) {
		error_msg = BOOT_LOGGING_INIT_X509;
		goto reset;
	}

	status = base64_core_init (&base64);
	if (status != 0) {
		error_msg = BOOT_LOGGING_INIT_BASE64;
		goto reset;
	}

	status = initialize_dice ();
	if (status != 0) {
		error_msg = BOOT_LOGGING_INIT_RIOT_CORE;
		goto reset;
	}

	/* Do not change the order of the following lines (KMT will overwrite the boot log). */
	tip_copy_hash_from_boot_log ();

	/* take key_invalid from kmt and header and place in tip scratchpad to be found later on L1 */
	uint32_t key_invalid =  BOOT_log_table.img_logs[BOOT_IMG_KMT].img_header_copy.key_invalid;
	REG_WRITE (TIP_SCR2, key_invalid);

	/* Initialize KMT img component before key manifest init.
	 * No need to load and verify as ROM have already done. */
	status = tip_firmware_component_init (&kmt_img, active_flash, IMG_KMT, 0, false);
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
		MANIFEST_KEY_ADDRESS_COPY, true);
	if (status != 0) {
		error_msg = BOOT_LOGGING_TIP_KMT_PARSE;
		goto reset;
	}

	status = load_main_application (active_flash, reset_source);
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
	riot_core_tip_release (&riot);

reset:
	riot_core_clear ((void*) DICE_CDI, SHA512_HASH_LENGTH);

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
