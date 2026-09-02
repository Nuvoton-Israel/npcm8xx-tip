// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef ROT_MEMORY_MAP_H_
#define ROT_MEMORY_MAP_H_

#include <stdint.h>
#include "crypto/hash.h"


/***********************
 * RoT flash layout
 ***********************/

#define CERBERUS_PROTOCOL_FW_VERSION_LEN 	32

/**
 * The maximum number of keys supported by the main keystore.
 */
#define	MAIN_KEYSTORE_MAX_KEYS		6

/**
 * Key IDs stored in the main keystore.
 */
#define	RIOT_DEVICE_ID_KEY			0
#define	RIOT_ROOT_CA_KEY			1
#define	RIOT_INTERMEDIATE_CA_KEY	2

/**
 * The maximum size for a RoT firmware image. Each FW region will be this size.
 */
#define ROT_IMAGE_MAX_SIZE 					ROT_COMBO0_MAX_SIZE + ROT_COMBO1_MAX_SIZE

/**
 * Flash address for the RoT boot image. This address is used for both the main and recovery
 * images.
 */
#define ROT_COMBO0_ADDR 					0
#define ROT_COMBO0_MAX_SIZE 				_512KB_
#define ROT_COMBO1_ADDR 					ROT_COMBO0_ADDR + ROT_COMBO0_MAX_SIZE
#define ROT_COMBO1_MAX_SIZE 				_2MB_

/**
 * Flash address for the RoT staging image. virtual address. It's an offset on DRAM
 */
#define TIP_VIRTUAL_FLASH_BASE_ADDR 		0x1000000
#define ROT_STAGING_ADDR 					0

/**
 * The maximum size for staging area on DRAM
 */
#define ROT_STAGING_SIZE 					_16MB_

/***********************
 * Shared memory
 **********************/

/**
 * Address where the share attestation data is stored by RIoT Core.
 */
#define RIOT_SHARED_ADDRESS 				0x6DF00

#define MAX_CERT_LENGTH 					MAX_DEVID_CERT_LENGTH

/**
 * Max Device ID Certificate length
 */
#define MAX_DEVID_CERT_LENGTH 				584

/**
 * Max Device ID CSR length
 */
#define MAX_DEVID_CSR_LENGTH 				530

/**
 * Max Device ID Alias private key length
 */
#define MAX_ALIAS_KEY_LENGTH 				132

/**
 * Max Alias Certificate length
 */
#define MAX_ALIAS_CERT_LENGTH 				584

/**
 * Structure for storing information that will be passed from RIoT Core to the main application.
 */
#pragma pack(push, 1)
struct riot_shared_attestation {
	uint8_t devid_cert[MAX_DEVID_CERT_LENGTH];			 /**< Storage for the self-signed device ID certificate. */
	int devid_cert_length;								 /**< Length of the certificate. */
	uint8_t devid_csr[MAX_DEVID_CSR_LENGTH];			 /**< Storage for the device ID CSR. */
	int devid_csr_length;								 /**< Length of the CSR. */
	uint8_t alias_key[MAX_ALIAS_KEY_LENGTH];			 /**< Storage for the alias private key. */
	int alias_key_length;								 /**< Length of the alias key. */
	uint8_t alias_cert[MAX_ALIAS_CERT_LENGTH];			 /**< Storage for the alias certificate signed by the device ID. */
	int alias_cert_length;								 /**< Length of the alias certificate. */
	uint8_t riot_hash[SHA512_HASH_LENGTH];				 /**< Measurement of L0: RIoT Core. */
	uint8_t keys_hash[SHA512_HASH_LENGTH];				 /**< Measurement of the KMT: key manifest. */
	uint8_t app_hash[SHA512_HASH_LENGTH];				 /**< Measurement of the L1: main application. */
	uint8_t skmt_hash[SHA512_HASH_LENGTH];				 /**< Measurement of the SKMT. */
	char riot_version[CERBERUS_PROTOCOL_FW_VERSION_LEN]; /**< RIoT Core version. */
	uint8_t attestation_hash[SHA256_HASH_LENGTH];		 /**< Hash of the shared attestation data. */
};
#pragma pack(pop)

/****************************
 * Static application memory
 ****************************/

/**
 * Address where the manifest verification key will be stored.
 * assume first key in KMT. Will be updated to TFT header.
 */
#define MANIFEST_KEY_ADDRESS 							0x0005F000

/**
 *  KMT copy. L0 copies the KMT over the parts of TIP_ROM log area.
 */
#define MANIFEST_KEY_ADDRESS_COPY 						0x0006FCE0

#define DEBUG_LOG_OFFSET_SINGLE_FLASH					0x6c2000

#endif /* ROT_MEMORY_MAP_H_ */
