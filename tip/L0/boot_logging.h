// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#ifndef BOOT_LOGGING_H_
#define BOOT_LOGGING_H_


/**
 * Error messages that can be logged during boot.
 */
enum {
	BOOT_LOGGING_SVN_ERROR = 0,		/**< Error initializing SVN handler. */
	BOOT_LOGGING_INIT_HASH,			/**< Error initializing hash engine. */
	BOOT_LOGGING_INIT_RSA,			/**< Error initializing RSA engine. */
	BOOT_LOGGING_INIT_ECC,			/**< Error initializing ECC engine. */
	BOOT_LOGGING_INIT_X509,			/**< Error initializing X509. */
	BOOT_LOGGING_INIT_BASE64,		/**< Error initializing BASE64. */
	BOOT_LOGGING_INIT_RNG,			/**< Error initializing RNG. */
	BOOT_LOGGING_INIT_RIOT_CORE,	/**< Error initializing RIOT core. */
	BOOT_LOGGING_APP_LOAD,			/**< Error loading main application. */
	BOOT_LOGGING_RIOT_KEYS,			/**< Error in RIOT keys. */
	BOOT_LOGGING_FLASH_INIT,		/**< Error initializing TIP flash. */
	BOOT_LOGGING_FLASH_LAYOUT,		/**< Error getting TIP flash layout. */
	BOOT_LOGGING_CMD_DEVICE,		/**< Error initializing cmd device handler. */
	BOOT_LOGGING_GET_UUID,			/**< Error getting UUID. */
	BOOT_LOGGING_FW_COMPONENT_INIT, /**< Error initializing running firmware component.*/
	BOOT_LOGGING_TIP_KMT_INIT,		/**< Error initiliazing TIP KMT. */
	BOOT_LOGGING_TIP_KMT_PARSE,		/**< Error parsing TIP KMT. */
};


#endif /* BOOT_LOGGING_H_ */