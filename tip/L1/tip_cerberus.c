// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
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
#include "tip_app_context.h"
#include "cmd_interface_tip.h"
#include "tip_mbx.h"
#include "tip_key_manifest.h"
#include "tip_virtual_flash.h"
#include "cerberus_pcr.h"
#include "pcr_tcg.h"
#include "init_logging.h"
#include "tip_device_id.h"
#include "tip_image_combo.h"
#include "deprecated/mctp_cmd_task.h"
#include "event_task_freertos.h"
#include "bmc_task.h"
#include "twd_task.h"
#include "tip_fw_update_task.h"
#include "status/rot_status.h"
#include "crypto/hash_thread_safe.h"
#include "crypto/rsa_thread_safe.h"
#include "crypto/ecc_thread_safe.h"
#include "asn1/x509_mbedtls.h"
#include "asn1/x509_thread_safe.h"
#include "crypto/rng_mbedtls.h"
#include "crypto/rng_thread_safe.h"
#include "cmd_interface/session_manager_ecc.h"
#include "cmd_interface/cmd_background_handler.h"
#include "mctp/cmd_interface_mctp_control.h"
#include "logging/logging_flash.h"
#include "logging/debug_log.h"
#include "keystore/keystore_flash.h"
#include "flash/flash_store_contiguous_blocks.h"
#include "riot/riot_key_manager.h"
#include "tip_rng_ncl.h"
#include "tip_aes_ncl.h"
#include "tip_hash_ncl.h"
#include "crypto/ecc_ecc_hw.h"
#include "tip_ecc_hw_ncl.h"

/* stuff from TIP_ROM */
extern TIP_LOG_Arr_T TIP_LOG_Arr __attribute__ ((section (".log")));
BOOT_LOG_TABLE_T BOOT_log_table __attribute__ ((section (".boot_log")));

/**
 * Version of Cerberus FW. This must be incremented with every change to the
 * bootloader.
 */
static const char *version_L1 = CERBERUS_FW_VERSION_STRING;

/**
 * Version string for the Cerberus FW.
 */
static char version[CERBERUS_PROTOCOL_FW_VERSION_LEN];

/**
 * Version string for RIoT core.
 */
static char riot_core_version[CERBERUS_PROTOCOL_FW_VERSION_LEN];

/**
 * List of FW version strings.
 */
static const char *fw_version_list[2];

/**
 * Container for FW version data.
 */
static struct cmd_interface_fw_version firmware_version;

/**
 * SVN version handler.
 */
struct tip_version_handler tip_version;

/**
 * Handler for the main flash.
 */
struct spi_flash *main_flash;

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

/**
 * Offset of recovery image location.
 */
static uint32_t recovery_flash_start_offset;

/**
 * Source of the most recent chip reset.
 */
static uint16_t reset_source;

/**
 * Hash engine that will be shared between multiple components.
 */
struct tip_hash_ncl_engine system_hash;

/**
 * Wrapper for the shared hash engine.
 */
struct hash_engine_thread_safe shared_hash;

/**
 * HW Engine for ECC operations.
 */
static struct tip_ecc_hw_ncl_engine hw_ecc;

/**
 * ECC engine that will be shared between multiple components.
 */
static struct ecc_engine_ecc_hw system_ecc;

/**
 * Wrapper for the shared ECC engine.
 */
static struct ecc_engine_thread_safe shared_ecc;

/**
 * X.509 engine that will be shared between multiple components.
 */
static struct x509_engine_mbedtls system_x509;

/**
 * Wrapper for the shared X.509 engine.
 */
static struct x509_engine_thread_safe shared_x509;

/**
 * Hardware RNG that will be shared between multiple components.
 */
static struct tip_rng_ncl_engine system_rng;

/**
 * Wrapper for the shared RNG engine.
 */
static struct rng_engine_thread_safe shared_rng;

/**
 * The interface to use when updating a firmware image combo.
 */
static struct tip_image_combo updating_img[NUM_TIP_FW_UPDATER];

/**
 * State management for the running image.
 */
static struct tip_app_context running_state;

/**
 * Variable context for the Cerberus firmware updater.
 */
static struct firmware_update_state fw_updater_context[NUM_TIP_FW_UPDATER];

/**
 * The module for updating Cerberus firmware.
 */
static struct firmware_update fw_updater[NUM_TIP_FW_UPDATER];

/**
 * The task for executing tip wd periodic
 */
static TaskHandle_t tip_wd_task;

/**
 * The task for executing Cerberus firmware update actions.
 */
static struct tip_fw_update_task cerberus_update;

/**
 * tip system manager
 */
static struct system tip_system;

/**
 * The I2C interface(mailbox) to the BMC.
 */
static struct tip_cmd_channel system_i2c;

/**
 * The system command interface processing task
 */
static struct mctp_cmd_task system_cmd_task;

/**
 * Variable context for the background command handler.
 */
static struct cmd_background_handler_state background_handler_context;

/**
 * Handler for processing commands in the background.
 */
static struct cmd_background_handler background_handler;

/**
 * List of handlers for the background command task.
 */
static const struct event_task_handler *background_handlers[1] = { &background_handler.base_event };

/**
 * Varible context for the background command processing task.
 */
static struct event_task_freertos_state cmd_background_context;

/**
 * Task for processing commands in the background.
 */
static struct event_task_freertos cmd_background_task;

/**
 * Task for handling bmc reset.
 */
struct bmc_task bmc_reset_task;

/**
 * The command handler for device operations.
 */
static struct tip_cmd_device tip_cmd_device;

/**
 * The BMC MCTP interface context
 */
static struct mctp_interface system_interface;

/**
 * The system command interface
 */
static struct cmd_interface_tip system_cmd_interface;

/**
 * The MCTP control command interface
 */
static struct cmd_interface_mctp_control mctp_control_cmd_interface;

/**
 * The system command interface processing task.
 */
static struct mctp_cmd_task system_cmd_task;

/**
 * The system attestation responder instance
 */
static struct attestation_responder system_attestation_responder;

/**
 * Variable context for the flash storage of keystore.
 */
static struct flash_store_contiguous_blocks_state keystore_flash_context;

/**
 * Flash block storage for keys.
 */
static struct flash_store_contiguous_blocks keystore_flash;

/**
 * Storage for keys and certificates.
 */
static struct keystore_flash main_keystore;

/**
 * Secondary Key manifest image component.
 */
static struct tip_firmware_component skmt_img;

/**
 * TIP secondary Key manifest.
 */
struct tip_secondary_key_manifest tip_skmt;

/**
 * Variable context for the debug log.
 */
static struct logging_flash_state debug_log_context;

/**
 * Flash logger for storing the debug log.
 */
static struct logging_flash debug_logger;

/**
 * Application manager for RIoT keys and certificates.
 */
static struct riot_key_manager riot;

/**
 * Device manager.
 */
static struct device_manager device_manager;

/**
 *  Platform and host PCR storage.
 */
static struct pcr_store pcr_storage;

#ifdef CMD_SUPPORT_ENCRYPTED_SESSIONS
#define CERBERUS_MAX_SESSIONS 3

/**
 * AES engine for session management.
 */
struct tip_aes_ncl_engine session_aes;

/**
 * Table for session management.
 */
struct session_manager_entry session_entries[CERBERUS_MAX_SESSIONS];
#endif

/**
 * Session manager instance.
 */
static struct session_manager_ecc session;

/******************************************
 * Attestation Measurement Data Structures
 ******************************************/

/* PCR 0 */
/**
 * Cerberus boot image (L0) measured data.
 */
static struct pcr_measured_data pcr_boot_image_measured_data = {
	.type = PCR_DATA_TYPE_MEMORY,
};

/**
 * Cerberus application image (L1) measured data.
 */
static struct pcr_measured_data pcr_app_image_measured_data = {
	.type = PCR_DATA_TYPE_MEMORY,
};

/**
 * TIP key manifest measured data.
 */
static struct pcr_measured_data pcr_tip_kmt_measured_data = {
	.type = PCR_DATA_TYPE_MEMORY,
};

/**
 * TIP secondary key maniefst measured data.
 */
static struct pcr_measured_data pcr_tip_skmt_measured_data = {
	.type = PCR_DATA_TYPE_MEMORY,
};

/**
 * BMC Bootblock measured data.
 */
static struct pcr_measured_data pcr_bb_measured_data = {
	.type = PCR_DATA_TYPE_MEMORY,
};

/**
 * BMC BL31 measured data.
 */
static struct pcr_measured_data pcr_bl31_measured_data = {
	.type = PCR_DATA_TYPE_MEMORY,
};

/**
 * BMC Optee measured data.
 */
static struct pcr_measured_data pcr_optee_measured_data = {
	.type = PCR_DATA_TYPE_MEMORY,
};

/**
 * BMC Uboot measured data.
 */
static struct pcr_measured_data pcr_uboot_measured_data = {
	.type = PCR_DATA_TYPE_MEMORY,
};

/**
 * Cerberus platform fw version measured data.
 */
static struct pcr_measured_data pcr_fw_version_measured_data = {
	.type = PCR_DATA_TYPE_MEMORY,
};

/**
 *  Variable context for the virtual flash device.
 */
static struct spi_flash_state virtual_flash_context;

/**
 * Virtual flash as staging area for FW update
 */
struct spi_flash virtual_flash;

/**
 * Virtual flash master
 */
static struct tip_flash_master_virtual virtual_flash_master;

/**
 * Firmware image partitioning information.
 */
static struct firmware_flash_map fw_flash_map[NUM_TIP_FW_UPDATER];

extern void vPortSVCHandler (void);
extern void xPortPendSVHandler (void);
extern void xPortSysTickHandler (void);
extern uint32_t *_stack_start_os;

/**
 * Run low-level initialization for TIP hardware.
 */
static void hardware_app_init ()
{
	DISABLE_INTERRUPTS ();
	NVIC_ClearAll ();
	tip_twd_common_init (false, WD_PRESET_L1, WD_WDIV_L1);
	serial_printf_init ();
	NVIC_Init (TRUE);
	NVIC_Reset ();
	SCS_Init ();
	SCS_FPEnableAccess (TRUE);

	/* Freertos use this address as MSP address. */
	NVIC_InstallSwTrap (NVIC_TRAP_INIT_SP, (SW_HANDLER_T) (&_stack_start_os - 4));

	for (uint32_t i = 1; i < NVIC_TRAP_NUM; i++) {
		NVIC_InstallSwTrap (i, (SW_HANDLER_T) NVIC_TrapHandlerCommon);
		NVIC_ClearInt (i);
	}

	SCS_ClearPendingSysTickInt ();

	for (uint32_t i = 0; i < NVIC_INTERRUPT_NUM; i++) {
		NVIC_EnableInt (i, FALSE);
		NVIC_InstallSwHandler (i, (SW_HANDLER_T) NVIC_IntHandlerCommon);
		NVIC_ClearInt (i);
	}

	NVIC_InstallSwTrap (NVIC_TRAP_SVC, (SW_HANDLER_T) vPortSVCHandler);
	NVIC_InstallSwTrap (NVIC_TRAP_PEND_SV, (SW_HANDLER_T) xPortPendSVHandler);
	NVIC_InstallSwTrap (NVIC_TRAP_SYST, (SW_HANDLER_T) xPortSysTickHandler);

	/* Clear BMC reset event */
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_DBGRST_STS, 1);
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_BMC_CRST_EV, 1);

	/* Detect BMC reset interrupt: INT46 Level High BMC CPU reset Interrupt */
	NVIC_InstallSwHandler (NVIC_INT_46, (SW_HANDLER_T) NVIC_BMC_reset);
	NVIC_ConfigPriority (NVIC_INT_46, 0x5);
	NVIC_ClearInt (NVIC_INT_46);
	NVIC_EnableInt (NVIC_INT_46, TRUE);
	tip_twd_common_init (true, WD_PRESET_L1, WD_WDIV_L1);
	ENABLE_INTERRUPTS ();
}

/**
 * Initialize crypto engines shared between system components.
 *
 * @return  0 if the operations was successful or an error code.
 */
static int initialize_crypto ()
{
	int status;

	status = tip_init_rom_ncl ();
	if (status != 0) {
		return status;
	}

	status = tip_hash_ncl_init (&system_hash);
	if (status != 0) {
		return status;
	}

	status = hash_thread_safe_init (&shared_hash, &system_hash.base);
	if (status != 0) {
		return status;
	}

	status = tip_ecc_hw_ncl_init (&hw_ecc);
	if (status != 0) {
		return status;
	}
	status = ecc_ecc_hw_init (&system_ecc, &hw_ecc.base);
	if (status != 0) {
		return status;
	}

	status = ecc_thread_safe_init (&shared_ecc, &system_ecc.base);
	if (status != 0) {
		return status;
	}

	status = x509_mbedtls_init (&system_x509);
	if (status != 0) {
		return status;
	}

	status = x509_thread_safe_init (&shared_x509, &system_x509.base);
	if (status != 0) {
		return status;
	}

	status = tip_rng_ncl_init (&system_rng);
	if (status != 0) {
		return status;
	}

	status = rng_thread_safe_init (&shared_rng, &system_rng.base);
	if (status != 0) {
		return status;
	}

	return 0;
}

/**
 * Verify the PCR hash of the shared PCR values with the bootloader.
 *
 * @param hash The initialized hash engine to calculate SHA256.
 *
 * @return 0 if the verification is successful or an error code.
 */
static int verify_stored_pcr_hash (struct hash_engine *hash)
{
	struct riot_shared_attestation *keys = (struct riot_shared_attestation *) RIOT_SHARED_ADDRESS;
	uint8_t pcr_hash[SHA256_HASH_LENGTH];
	int status;

	status = hash->calculate_sha256 (hash, (uint8_t *) RIOT_SHARED_ADDRESS,
		sizeof (struct riot_shared_attestation) - sizeof (keys->attestation_hash), pcr_hash,
		sizeof (pcr_hash));
	if (status != 0) {
		return status;
	}

	if (memcmp (pcr_hash, keys->attestation_hash, SHA256_HASH_LENGTH) != 0) {
		status = -1;
	}

	return status;
}

#ifdef CMD_SUPPORT_ENCRYPTED_SESSIONS
/**
 * Initialize management of encrypted sessions.
 *
 * @return 0 if session management was successfully initialized or an error code.
 */
static int initialize_session_management ()
{
	int status;

	status = tip_aes_ncl_init (&session_aes);
	if (status != 0) {
		return status;
	}

	status = session_manager_ecc_init (&session, &session_aes.base, &shared_ecc.base,
		&shared_hash.base, &shared_rng.base, &riot, session_entries, CERBERUS_MAX_SESSIONS, NULL, 0,
		NULL);
	return status;
}
#endif

/**
 * @brief Initialize TIP flash map for firmware updater
 *
 * @param index The index of flash map
 * @return 0 if the flash map was successfully initialized or an error code
 */
static int tip_fw_flash_map_init (int index)
{
	if (index >= NUM_TIP_FW_UPDATER) {
		return FIRMWARE_UPDATE_INVALID_ARGUMENT;
	}

	if (!main_flash || !recovery_flash) {
		return FLASH_HW_NOT_INIT;
	}

	memset (&fw_flash_map[index], 0, sizeof (struct firmware_flash_map));

	fw_flash_map[index].active_flash = &main_flash->base;
	fw_flash_map[index].active_addr = (index == TIP_FW_UPDATER_COMBO_0_2 ? ROT_COMBO0_ADDR :
																			ROT_COMBO1_ADDR);
	fw_flash_map[index].active_size = ROT_IMAGE_MAX_SIZE;
	fw_flash_map[index].backup_flash = NULL;
	fw_flash_map[index].staging_flash = &virtual_flash.base;
	fw_flash_map[index].staging_addr = ROT_STAGING_ADDR;
	fw_flash_map[index].staging_size = ROT_IMAGE_MAX_SIZE;
	fw_flash_map[index].recovery_flash = &recovery_flash->base;
	fw_flash_map[index].recovery_addr = recovery_flash_start_offset + index * ROT_COMBO0_MAX_SIZE;
	fw_flash_map[index].recovery_size = ROT_IMAGE_MAX_SIZE;
	fw_flash_map[index].rec_backup_flash = NULL;

	return 0;
}

/**
 * Initialize and start the Cerberus firmware updater.
 *
 * @return 0 if the firmware updater was successfully initialized or an error code.
 */
static int initialize_firmware_updater ()
{
	int allowed_version = -1;
	int status;

	for (int i = TIP_FW_UPDATER_COMBO_0_2; i < NUM_TIP_FW_UPDATER; i++) {
		status = tip_image_combo_init (&updating_img[i]);
		if (status != 0) {
			return status;
		}

		status = tip_fw_flash_map_init (i);
		if (status != 0) {
			return status;
		}

		/* TIP has its own API tip_version_set_and_check() to check each FW component within the
		 * combo image to avoid roll-back during the update flow. No need to provide the allowed
		 * revision to the updater. */
		status = firmware_update_init_no_firmware_header (&fw_updater[i], &fw_updater_context[i],
			&fw_flash_map[i], &running_state.base, &updating_img[i].base, &shared_hash.base,
			allowed_version);
		if (status != 0) {
			return status;
		}
	}

	status = tip_fw_update_task_init (&cerberus_update, fw_updater, NUM_TIP_FW_UPDATER, &tip_system);

	return status;
}

/**
 * Initialize I2C channel for receiving commands.
 *
 * @return 0 if the command channel was successfully initialized or an error code.
 */
static int init_cmd_interface ()
{
	struct device_manager_full_capabilities i2c_caps;
	int status;

	status = device_manager_init_ac_rot (&device_manager, 4, DEVICE_MANAGER_SLAVE_BUS_ROLE);
	if (status != 0) {
		return status;
	}

	/* Adjust the capabilities on the I2C interface. */
	device_manager_get_device_capabilities (&device_manager, DEVICE_MANAGER_SELF_DEVICE_NUM,
		&i2c_caps);

	i2c_caps.request.security_mode |= DEVICE_MANAGER_SECURITY_CONFIDENTIALITY;
	i2c_caps.request.ecc_key_strength = DEVICE_MANAGER_ECC_KEY_256;
	i2c_caps.request.ecdsa = 1;
	i2c_caps.request.rsa_key_strength = DEVICE_MANAGER_RSA_KEY_2048 | DEVICE_MANAGER_RSA_KEY_3072;
	i2c_caps.request.rsa = 0;
	i2c_caps.request.aes_enc_key_strength = DEVICE_MANAGER_AES_KEY_256;
	i2c_caps.request.pfm_support = 0;
	i2c_caps.request.fw_protection = 1;

	device_manager_update_device_capabilities (&device_manager, DEVICE_MANAGER_SELF_DEVICE_NUM,
		&i2c_caps);

	/* Update entry for Cerberus */
	status = device_manager_update_not_attestable_device_entry (&device_manager,
		DEVICE_MANAGER_SELF_DEVICE_NUM, MCTP_BASE_PROTOCOL_PA_ROT_CTRL_EID, CERBERUS_SLAVE_ADDR,
		DEVICE_MANAGER_NOT_PCD_COMPONENT);
	if (status != 0) {
		return status;
	}

	/* Update entry for BMC */
	status = device_manager_update_not_attestable_device_entry (&device_manager,
		DEVICE_MANAGER_MCTP_BRIDGE_DEVICE_NUM, MCTP_BASE_PROTOCOL_BMC_EID, 0,
		DEVICE_MANAGER_NOT_PCD_COMPONENT);
	if (status != 0) {
		return status;
	}

	/* Update entry for in-band utility */
	status = device_manager_update_not_attestable_device_entry (&device_manager, 2,
		MCTP_BASE_PROTOCOL_IB_EXT_MGMT, 0, DEVICE_MANAGER_NOT_PCD_COMPONENT);
	if (status != 0) {
		return status;
	}

	/* Update entry for out-of-band utility */
	status = device_manager_update_not_attestable_device_entry (&device_manager, 3,
		MCTP_BASE_PROTOCOL_OOB_EXT_MGMT, 0, DEVICE_MANAGER_NOT_PCD_COMPONENT);
	if (status != 0) {
		return status;
	}

	status = attestation_responder_init_no_aux (&system_attestation_responder, &riot,
		&shared_hash.base, &shared_ecc.base, &shared_rng.base, &pcr_storage,
		CERBERUS_PROTOCOL_PROTOCOL_VERSION, CERBERUS_PROTOCOL_PROTOCOL_VERSION);
	if (status != 0) {
		return status;
	}

	status = cmd_background_handler_init (&background_handler, &background_handler_context,
		&system_attestation_responder, &shared_hash.base, NULL, &riot, &cmd_background_task.base);
	if (status != 0) {
		return status;
	}

	status = event_task_freertos_init (&cmd_background_task, &cmd_background_context, &tip_system,
		background_handlers, 1);
	if (status != 0) {
		return status;
	}

	status = tip_cmd_device_init (&tip_cmd_device);
	if (status != 0) {
		return status;
	}

	fw_version_list[0] = version;
	fw_version_list[1] = riot_core_version;
	firmware_version.count = 2;
	firmware_version.id = fw_version_list;

	status = tip_cmd_channel_init (&system_i2c, 0, 0xfffce000, 0x1000);
	if (status != 0) {
		return status;
	}

	status = cmd_interface_tip_init (&system_cmd_interface, &cerberus_update.base,
		&system_attestation_responder, &device_manager, &pcr_storage, &shared_hash.base,
		&background_handler.base_cmd, &firmware_version, &riot, &tip_cmd_device.base,
		CERBERUS_PROTOCOL_MSFT_PCI_VID, TIP_DEVICE_ID_CERBERUS, CERBERUS_PROTOCOL_MSFT_PCI_VID,
		TIP_SUBSYSTEM_DEVICE_ID_CERBERUS, &session.base);
	if (status != 0) {
		return status;
	}

	status = cmd_interface_mctp_control_init (&mctp_control_cmd_interface, &device_manager,
		CERBERUS_PROTOCOL_MSFT_PCI_VID, CERBERUS_PROTOCOL_PROTOCOL_VERSION);
	if (status != 0) {
		return status;
	}

	status = mctp_interface_init (&system_interface, &system_cmd_interface.base,
		&mctp_control_cmd_interface.base, NULL, &device_manager);
	if (status != 0) {
		return status;
	}

	return 0;
}

/**
 * Start the command interface tasks.
 *
 * @return 0 if the command channel was successfully started or an error code.
 */
static int start_cmd_interface ()
{
	int status;

	status = event_task_freertos_start (&cmd_background_task, (4 * 256) + 128, "CmdBgnd",
		CERBERUS_PRIORITY_NORMAL);
	if (status != 0) {
		return status;
	}

	status = mctp_cmd_task_init (&system_cmd_task, &system_i2c.base, &system_interface,
		CERBERUS_PRIORITY_HIGH, (3 * 256) + 128);
	if (status != 0) {
		return status;
	}

	return 0;
}

/**
 * Allocate a new buffer for the RIoT key and copy the data from the temporary location.
 *
 * @param dest Output for the new buffer to be allocated.
 * @param length Output for the size of the data.
 * @param src The RIoT data to be copied.
 * @param src_length Length of the RIoT data.
 * @param name Name of the RIoT data being copied.
 * @param id Logging ID for the key.
 */
static void copy_riot_key (uint8_t **dest, size_t *length, const uint8_t *src, int src_length,
	int id)
{
	if (src_length > 0) {
		*dest = platform_malloc (src_length);
		if (*dest != NULL) {
			memcpy (*dest, src, src_length);
			*length = src_length;
		}
	} else if (src_length < 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_RIOT_KEY_TOO_BIG, id, 0);
	} else {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_NO_RIOT_KEY_DATA_AVAILABLE, id, 0);
	}
}

/**
 * Get the RIoT keys passed from the bootloader and store them in the current memory space.
 *
 * @param riot_core Location to copy the RIoT keys to.
 */
static void get_riot_keys (struct riot_keys *riot_core)
{
	struct riot_shared_attestation *keys = (struct riot_shared_attestation *) RIOT_SHARED_ADDRESS;

	memset (riot_core, 0, sizeof (riot_core));

	copy_riot_key ((uint8_t **) &riot_core->devid_cert, &riot_core->devid_cert_length,
		keys->devid_cert, keys->devid_cert_length, INIT_RIOT_KEY_DEVICE_ID);

	copy_riot_key ((uint8_t **) &riot_core->devid_csr, &riot_core->devid_csr_length,
		keys->devid_csr, keys->devid_csr_length, INIT_RIOT_KEY_DEVICE_ID_CSR);

	copy_riot_key ((uint8_t **) &riot_core->alias_key, &riot_core->alias_key_length,
		keys->alias_key, keys->alias_key_length, INIT_RIOT_KEY_ALIAS_KEY);

	copy_riot_key ((uint8_t **) &riot_core->alias_cert, &riot_core->alias_cert_length,
		keys->alias_cert, keys->alias_cert_length, INIT_RIOT_KEY_ALIAS_CERT);

	strncpy (riot_core_version, keys->riot_version, CERBERUS_PROTOCOL_FW_VERSION_LEN);
}

/**
 * Initialize the manager for RIoT certificates and keys.
 *
 * @return 0 if the RIoT manager was successfully initialized or an error code.
 */
static int initialize_riot_manager ()
{
	struct riot_keys riot_core;
	uint32_t main_keystore_addr;
	int status;

	get_riot_keys (&riot_core);

	/* The base flash address for the main keystore starts at 256KB from the end of main flash.
	   It grows down and includes RIoT certificates and manifest keys. */
	main_keystore_addr = recovery_flash_start_offset > 0 ? recovery_flash_start_offset - _256KB_ :
															 main_flash->state->device_size - _256KB_;

	status = flash_store_contiguous_blocks_init_variable_storage_decreasing (&keystore_flash,
		&keystore_flash_context, &main_flash->base, main_keystore_addr, MAIN_KEYSTORE_MAX_KEYS, 0,
		&shared_hash.base);
	if (status != 0) {
		return status;
	}

	status = keystore_flash_init (&main_keystore, &keystore_flash.base);
	if (status != 0) {
		return status;
	}

	status = riot_key_manager_init_static (&riot, &main_keystore.base, &riot_core,
		&shared_x509.base);

	return status;
}

/**
 * Initialize the Cerberus attestation measurements.
 *
 * @param hash Hash to use to generate the measurement.
 *
 * @return 0 if attestation was successfully initialized or an error code.
 */
static int initialize_cerberus_attestation (struct hash_engine *hash)
{
	struct riot_shared_attestation *keys = (struct riot_shared_attestation *) RIOT_SHARED_ADDRESS;
	uint8_t num_pcr_measurements[PCR_CERBERUS_NUM_BANKS] = { PCR_CERBERUS_PLATFORM_MEASUREMENTS };
	int status;

	status = pcr_store_init (&pcr_storage, num_pcr_measurements, sizeof (num_pcr_measurements));
	if (status != 0) {
		return status;
	}

	/* Init FW version measured data. */
	pcr_fw_version_measured_data.type = PCR_DATA_TYPE_MEMORY;
	pcr_fw_version_measured_data.data.memory.buffer = (uint8_t *) version;
	pcr_fw_version_measured_data.data.memory.length = strlen (version) + 1;

	/* Init boot image measured data */
	pcr_boot_image_measured_data.type = PCR_DATA_TYPE_MEMORY;
	pcr_boot_image_measured_data.data.memory.buffer = keys->riot_hash;
	pcr_boot_image_measured_data.data.memory.length = sizeof (keys->riot_hash);

	/* Init app image measured data */
	pcr_app_image_measured_data.type = PCR_DATA_TYPE_MEMORY;
	pcr_app_image_measured_data.data.memory.buffer = keys->app_hash;
	pcr_app_image_measured_data.data.memory.length = sizeof (keys->app_hash);

	/* Init KMT image measured data */
	pcr_tip_kmt_measured_data.type = PCR_DATA_TYPE_MEMORY;
	pcr_tip_kmt_measured_data.data.memory.buffer = keys->keys_hash;
	pcr_tip_kmt_measured_data.data.memory.length = sizeof (keys->keys_hash);

	/* Init SKMT image measured data */
	pcr_tip_skmt_measured_data.type = PCR_DATA_TYPE_MEMORY;
	pcr_tip_skmt_measured_data.data.memory.buffer = keys->skmt_hash;
	pcr_tip_skmt_measured_data.data.memory.length = sizeof (keys->skmt_hash);

	/* Init Bootblock image measured data */
	pcr_bb_measured_data.type = PCR_DATA_TYPE_MEMORY;
	pcr_bb_measured_data.data.memory.buffer = bmc_component_get_digest_buf (IMG_BOOTBLOCK);
	pcr_bb_measured_data.data.memory.length = SHA512_HASH_LENGTH;

	/* Init BL31 image measured data */
	pcr_bl31_measured_data.type = PCR_DATA_TYPE_MEMORY;
	pcr_bl31_measured_data.data.memory.buffer = bmc_component_get_digest_buf (IMG_BL31);
	pcr_bl31_measured_data.data.memory.length = SHA512_HASH_LENGTH;

	/* Init optee image measured data */
	pcr_optee_measured_data.type = PCR_DATA_TYPE_MEMORY;
	pcr_optee_measured_data.data.memory.buffer = bmc_component_get_digest_buf (IMG_OPTEE);
	pcr_optee_measured_data.data.memory.length = SHA512_HASH_LENGTH;

	/* Init uboot image measured data */
	pcr_uboot_measured_data.type = PCR_DATA_TYPE_MEMORY;
	pcr_uboot_measured_data.data.memory.buffer = bmc_component_get_digest_buf (IMG_UBOOT);
	pcr_uboot_measured_data.data.memory.length = SHA512_HASH_LENGTH;

	pcr_store_update_event_type (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_BOOT_IMG,
		PCR_TCG_EVENT_TYPE_PLATFORM_BOOT_IMG);
	pcr_store_set_measurement_data (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_BOOT_IMG,
		&pcr_boot_image_measured_data);

	pcr_store_update_event_type (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_APP_IMG,
		PCR_TCG_EVENT_TYPE_PLATFORM_APP_IMG);
	pcr_store_set_measurement_data (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_APP_IMG,
		&pcr_app_image_measured_data);

	pcr_store_update_event_type (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_FW_VERSION,
		PCR_TCG_EVENT_TYPE_PLATFORM_CERBERUS_FW_VERSION);
	pcr_store_set_measurement_data (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_FW_VERSION,
		&pcr_fw_version_measured_data);

	pcr_store_update_event_type (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_KMT_IMG,
		PCR_TCG_EVENT_TYPE_PLATFORM_KMT_IMG);
	pcr_store_set_measurement_data (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_KMT_IMG,
		&pcr_tip_kmt_measured_data);

	pcr_store_update_event_type (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_SKMT_IMG,
		PCR_TCG_EVENT_TYPE_PLATFORM_SKMT_IMG);
	pcr_store_set_measurement_data (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_SKMT_IMG,
		&pcr_tip_skmt_measured_data);

	pcr_store_update_event_type (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_BB_IMG,
		PCR_TCG_EVENT_TYPE_PLATFORM_BB_IMG);
	pcr_store_set_measurement_data (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_BB_IMG,
		&pcr_bb_measured_data);

	pcr_store_update_event_type (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_BL31_IMG,
		PCR_TCG_EVENT_TYPE_PLATFORM_BL31_IMG);
	pcr_store_set_measurement_data (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_BL31_IMG,
		&pcr_bl31_measured_data);

	pcr_store_update_event_type (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_OPTEE_IMG,
		PCR_TCG_EVENT_TYPE_PLATFORM_OPTEE_IMG);
	pcr_store_set_measurement_data (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_OPTEE_IMG,
		&pcr_optee_measured_data);

	pcr_store_update_event_type (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_UBOOT_IMG,
		PCR_TCG_EVENT_TYPE_PLATFORM_UBOOT_IMG);
	pcr_store_set_measurement_data (&pcr_storage, PCR_MEASUREMENT_TYPE_PLATFORM_UBOOT_IMG,
		&pcr_uboot_measured_data);

	status = pcr_store_update_versioned_buffer (&pcr_storage, hash,
		PCR_MEASUREMENT_TYPE_PLATFORM_BOOT_IMG, pcr_boot_image_measured_data.data.memory.buffer,
		pcr_boot_image_measured_data.data.memory.length, true, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_PCR_STORE_UPDATE_BUFFER, PCR_MEASUREMENT_TYPE_PLATFORM_BOOT_IMG, status);
	}

	status = pcr_store_update_versioned_buffer (&pcr_storage, hash,
		PCR_MEASUREMENT_TYPE_PLATFORM_APP_IMG, pcr_app_image_measured_data.data.memory.buffer,
		pcr_app_image_measured_data.data.memory.length, true, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_PCR_STORE_UPDATE_BUFFER, PCR_MEASUREMENT_TYPE_PLATFORM_APP_IMG, status);
	}

	status = pcr_store_update_versioned_buffer (&pcr_storage, hash,
		PCR_MEASUREMENT_TYPE_PLATFORM_FW_VERSION, pcr_fw_version_measured_data.data.memory.buffer,
		pcr_fw_version_measured_data.data.memory.length, true, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_PCR_STORE_UPDATE_BUFFER, PCR_MEASUREMENT_TYPE_PLATFORM_FW_VERSION, status);
	}

	status = pcr_store_update_versioned_buffer (&pcr_storage, hash,
		PCR_MEASUREMENT_TYPE_PLATFORM_KMT_IMG, pcr_tip_kmt_measured_data.data.memory.buffer,
		pcr_tip_kmt_measured_data.data.memory.length, true, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_PCR_STORE_UPDATE_BUFFER, PCR_MEASUREMENT_TYPE_PLATFORM_KMT_IMG, status);
	}

	status = pcr_store_update_versioned_buffer (&pcr_storage, hash,
		PCR_MEASUREMENT_TYPE_PLATFORM_SKMT_IMG, pcr_tip_skmt_measured_data.data.memory.buffer,
		pcr_tip_skmt_measured_data.data.memory.length, true, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_PCR_STORE_UPDATE_BUFFER, PCR_MEASUREMENT_TYPE_PLATFORM_SKMT_IMG, status);
	}

	status = pcr_store_update_versioned_buffer (&pcr_storage, hash,
		PCR_MEASUREMENT_TYPE_PLATFORM_BB_IMG, pcr_bb_measured_data.data.memory.buffer,
		pcr_bb_measured_data.data.memory.length, true, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_PCR_STORE_UPDATE_BUFFER, PCR_MEASUREMENT_TYPE_PLATFORM_BB_IMG, status);
	}

	status = pcr_store_update_versioned_buffer (&pcr_storage, hash,
		PCR_MEASUREMENT_TYPE_PLATFORM_BL31_IMG, pcr_bl31_measured_data.data.memory.buffer,
		pcr_bl31_measured_data.data.memory.length, true, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_PCR_STORE_UPDATE_BUFFER, PCR_MEASUREMENT_TYPE_PLATFORM_BL31_IMG, status);
	}

	status = pcr_store_update_versioned_buffer (&pcr_storage, hash,
		PCR_MEASUREMENT_TYPE_PLATFORM_OPTEE_IMG, pcr_optee_measured_data.data.memory.buffer,
		pcr_optee_measured_data.data.memory.length, true, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_PCR_STORE_UPDATE_BUFFER, PCR_MEASUREMENT_TYPE_PLATFORM_OPTEE_IMG, status);
	}

	status = pcr_store_update_versioned_buffer (&pcr_storage, hash,
		PCR_MEASUREMENT_TYPE_PLATFORM_UBOOT_IMG, pcr_uboot_measured_data.data.memory.buffer,
		pcr_uboot_measured_data.data.memory.length, true, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT,
			INIT_LOGGING_PCR_STORE_UPDATE_BUFFER, PCR_MEASUREMENT_TYPE_PLATFORM_UBOOT_IMG, status);
	}

	return 0;
}

/**
 * Initialize the running application context.
 *
 * @return 0 if the context was successfully initialized or an error code.
 */
static int initialize_app_context ()
{
	int status;

	status = tip_app_context_init (&running_state);
	if (status != 0) {
		return status;
	}

	if ((reset_source & RESET_PORST) == 0) {
		/* Use abstract API for now as nothing to be restored. */
		tip_app_context_restore (&running_state.base);
	}

	return 0;
}

/**
 * Task that will run system initialization.
 *
 * @param unused Unused.
 */
static void cerberus_init (void *unused)
{
	int status;
	int error_msg = -1;
	int tip_wd_delay = WD_PERIOD_SEC;

	/* initialize WD periodic handling*/
	status = tip_watchdog_service_start (&tip_wd_task, tip_wd_delay, 
		configMINIMAL_STACK_SIZE);
	if (status != 0) {
		error_msg = INIT_LOGGING_WD_TASK;
		goto reset;
	}
	
	/* Initialize core system components. */
	status = tip_flash_initialize_access (FLASH_MAX_FIU, FLASH_MAX_CS);
	if (status != 0) {
		error_msg = INIT_LOGGING_FLASH_ACCESS;
		goto reset;
	}

	status = tip_flash_get_layout (FLASH_MAX_FIU, FLASH_MAX_CS, &main_flash, &recovery_flash,
		&active_flash, &recovery_flash_start_offset);
	if (status != 0) {
		error_msg = INIT_LOGGING_FLASH_LAYOUT;
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
		platform_printf ("Failed to initialize debug logging module:  %#010lx" NEWLINE, status);
	}

	debug_log_flush ();

	debug_log_create_entry ((recovery_boot) ? DEBUG_LOG_SEVERITY_WARNING : DEBUG_LOG_SEVERITY_INFO,
		DEBUG_LOG_COMPONENT_INIT, INIT_LOGGING_BOOT_SOURCE, recovery_boot, reset_source);

	/* Initialize a staging virtual flash */
	status = tip_initialize_flash_access_virtual (&virtual_flash, &virtual_flash_context,
		&virtual_flash_master.base, TIP_VIRTUAL_FLASH_BASE_ADDR, ROT_STAGING_SIZE);
	if (status != 0) {
		error_msg = INIT_LOGGING_VIRTUAL_FLASH_ACCESS;
		goto reset;
	}

	status = initialize_crypto ();
	if (status != 0) {
		error_msg = INIT_LOGGING_SYSTEM_CRYPTO;
		goto reset;
	}

	status = tip_version_init (&tip_version);
	if (status != 0) {
		error_msg = INIT_LOGGING_TIP_SVN;
		goto reset;
	}

	/* No need to load and verify, already done at L0. Hence -1 start offset. */
	status = tip_firmware_component_init (&skmt_img, active_flash, IMG_SKMT, 0, false);
	if (status != 0) {
		error_msg = INIT_LOGGING_FW_COMPONENT;
		goto reset;
	}

	/* Initialize SKMT API. */
	status = tip_skmt_init (&tip_skmt, &skmt_img, MANIFEST_KEY_ADDRESS_COPY);
	if (status != 0) {
		error_msg = INIT_LOGGING_TIP_SKMT_INIT;
		goto reset;
	}

	status = tip_skmt_parse (&tip_skmt);
	if (status != 0) {
		error_msg = INIT_LOGGING_SKMT_PARSE;
		goto reset;
	}

	status = bmc_task_init (&bmc_reset_task, CERBERUS_PRIORITY_BACKGROUND, 256 * 3 + 128);
	if (status != 0) {
		error_msg = INIT_LOGGING_BMC_TASK;
		goto reset;
	}

	status = tip_load_bmc_firmware (&bmc_reset_task, active_flash,
		recovery_boot ? recovery_flash_start_offset + ROT_COMBO1_ADDR : ROT_COMBO1_ADDR,
		&shared_hash.base);
	if (status != 0) {
		error_msg = INIT_LOGGING_A35_INIT;
		goto reset;
	}

	status = verify_stored_pcr_hash (&shared_hash.base);
	if (status != 0) {
		error_msg = INIT_LOGGING_PCR_VERIFY;
		goto reset;
	}

	status = initialize_riot_manager ();
	if (status != 0) {
		error_msg = INIT_LOGGING_RIOT_MANAGER;
		goto reset;
	}

	status = initialize_cerberus_attestation (&shared_hash.base);
	if (status != 0) {
		error_msg = INIT_LOGGING_PCR_STORE;
		goto reset;
	}

	/* Nuvoton BMC specific workflow */
	bmc_export_data ();

	status = initialize_app_context ();
	if (status != 0) {
		error_msg = INIT_LOGGING_RESTORE_CONTEXT;
		goto reset;
	}

	status = system_init (&tip_system, &tip_cmd_device.base);
	if (status != 0) {
		error_msg = INIT_LOGGING_SYSTEM_STATE;
		goto reset;
	}

	status = initialize_firmware_updater ();
	if (status != 0) {
		error_msg = INIT_LOGGING_FW_UPDATER;
		goto reset;
	}

#ifdef CMD_SUPPORT_ENCRYPTED_SESSIONS
	status = initialize_session_management ();
	if (status != 0) {
		error_msg = INIT_LOGGING_SESSION_MANAGEMENT;
		goto reset;
	}
#endif

	status = init_cmd_interface ();
	if (status != 0) {
		error_msg = INIT_LOGGING_COMMAND_HANDLER;
		goto reset;
	}

	status = tip_fw_update_task_start (&cerberus_update, 6 * 256, recovery_boot);
	if (status != 0) {
		error_msg = INIT_LOGGING_FW_UPDATE_TASK;
		goto reset;
	}

	/* Workaround: Wait until fw update task finishes loading recovery image or
	   restoring active image to avoid failure while uboot reads env variable from the same flash.
	   Will remove when flash sharing enhancement is done. */
	while (cerberus_update.running != 0) {
		platform_msleep (0);
	}

	status = start_cmd_interface ();
	if (status != 0) {
		goto reset;
	}

	/* Set next reboot addr starting from active flash */
	tip_select_next_boot_image (0);

	bmc_continue ();

	vTaskDelete (NULL);

reset:
	if (error_msg >= 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_INIT, error_msg,
			status, 0);
		platform_printf ("System initialization failed: msg=%d, status=%#010lx" NEWLINE, error_msg,
			status);
	} else {
		platform_printf ("System initialization failed: status=%#010lx" NEWLINE, status);
	}

	debug_log_flush ();

	/* Never halt the system. Reboot and try again. */
	platform_printf (NEWLINE);
	platform_reset (0);
}

/**
 * Cerberus entry point.
 */
int main (void)
{
	int status;

	hardware_app_init ();

	memcpy (version, version_L1, CERBERUS_PROTOCOL_FW_VERSION_LEN);

	platform_printf (KMAG NEWLINE ">================================================" NEWLINE);
	platform_printf (">  Arbel TIP FW L1 Version %s" NEWLINE, version);
	platform_printf (">================================================" NEWLINE);
	platform_printf ("Compile time: %s %s " NEWLINE KNRM, __DATE__, __TIME__, NEWLINE);

	reset_source = tip_get_reset_indication ();
	tip_update_reset_indication (TRUE);

	status = xTaskCreate (cerberus_init, "Init", 5 * 256, NULL, CERBERUS_PRIORITY_BACKGROUND, NULL);
	if (status == pdPASS) {
		vTaskStartScheduler ();
		platform_printf ("Returned from FreeRTOS scheduler!?" NEWLINE);
	} else {
		platform_printf ("Failed to create init task (%d)!" NEWLINE, status);
		goto reset;
	}

reset:
	/* Never halt the system.  Reboot and try again. */
	platform_printf (NEWLINE);
	platform_reset (0);
}
