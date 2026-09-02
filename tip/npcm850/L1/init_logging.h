// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#ifndef INIT_LOGGING_H_
#define INIT_LOGGING_H_


/**
 * Error messages that can be logged during initialization.
 */
enum {
	INIT_LOGGING_BOOT_SOURCE = 0,				/**< Indicate the flash device used for booting. */
	INIT_LOGGING_FW_UPDATER,					/**< Error with the firmware updater. */
	INIT_LOGGING_RIOT_MANAGER,					/**< Error initializing RIoT keys. */
	INIT_LOGGING_SYSTEM_STATE,					/**< Error initializing system state. */
	INIT_LOGGING_FW_UPDATE_TASK,				/**< Error starting the firmware updater. */
	INIT_LOGGING_PCR_STORE,						/**< Error initializing PCR store. */
	INIT_LOGGING_PCR_STORE_UPDATE_BUFFER,		/**< Error adding buffer to PCR. */
	INIT_LOGGING_RESTORE_CONTEXT,				/**< Error restoring context after reset. */
	INIT_LOGGING_SAVE_CONTEXT,					/**< Error saving the context during POR. */
	INIT_LOGGING_PCR_VERIFY,					/**< Error verifying shared PCR values. */
	INIT_LOGGING_RIOT_KEY_TOO_BIG,				/**< A RIoT key was too large for the reserved buffer. */
	INIT_LOGGING_SYSTEM_CRYPTO,					/**< Error initializing system crypto engines. */
	INIT_LOGGING_SESSION_MANAGEMENT,			/**< Error initializing session manager. */
	INIT_LOGGING_NO_RIOT_KEY_DATA_AVAILABLE,	/**< A RIoT key data was not available. */
	INIT_LOGGING_COMMAND_HANDLER,				/**< Error initializing command hander. */
	INIT_LOGGING_FLASH_ACCESS,					/**< Error initializing flash access. */
	INIT_LOGGING_FLASH_LAYOUT,					/**< Error getting flash layout.*/
	INIT_LOGGING_VIRTUAL_FLASH_ACCESS,			/**< Error initializing virtual flash access.*/
	INIT_LOGGING_TIP_SVN,						/**< Error initiliazing TIP SVN handler. */
	INIT_LOGGING_FW_COMPONENT,					/**< Error intitalizing running firmware component.*/
	INIT_LOGGING_TIP_KMT_INIT,					/**< Error initiliazing TIP KMT. */
	INIT_LOGGING_TIP_SKMT_INIT,					/**< Error initiliazing TIP SKMT. */
	INIT_LOGGING_SKMT_PARSE,					/**< Error parsing TIP SKMT. */
	INIT_LOGGING_A35_INIT,						/**< Error initilizing A35 subsystem. */
	INIT_LOGGING_BMC_TASK,						/**< Error initilizing BMC task. */
	INIT_LOGGING_WD_TASK,						/**< Error initilizing WD update task. */
	INIT_LOGGING_CONFIG_CMD_TASK,				/**< Error starting command task context. */
	INIT_LOGGING_LOG_TASK,						/**< Error starting the task to flush the log. */
	INIT_LOGGING_INIT_CONFIG_CMD_TASK,			/**< Error initializing config cmd task. */
	INIT_LOGGING_PCD_MANAGEMENT,				/**< Error initializing PCDs. */
	INIT_LOGGING_CFM_MANAGEMENT,				/**< Error initializing CFMs. */
	INIT_LOGGING_ROT_CONFIG,					/**< Error getting the RoT configuration. */
	INIT_LOGGING_PCD_COMPONENT_ERROR,			/**< Error getting components from PCD. */
	INIT_LOGGING_PCD_COMPONENT,					/**< PCD component added to attestable devices list. */
	INIT_LOGGING_CONFIG_MGMT,					/**< Error initializing configuration management. */
	INIT_LOGGING_OTP_KEY_REVOCATION,			/**< Error OTP key revocation. */

};

/**
 * IDs for logging errors passing RIoT keys and certificates.
 */
enum {
	INIT_RIOT_KEY_DEVICE_ID = 0,  				/**< Device ID certificate. */
	INIT_RIOT_KEY_DEVICE_ID_CSR,				/**< Device ID CSR. */
	INIT_RIOT_KEY_ALIAS_KEY,					/**< Alias key pair. */
	INIT_RIOT_KEY_ALIAS_CERT,					/**< Alias certificate. */
	INIT_RIOT_KEY_ATTESTATION_CERT				/**< Host attestation certificate. */
};


#endif /* INIT_LOGGING_H_ */
