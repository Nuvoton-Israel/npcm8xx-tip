// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_SKMT_H_
#define TIP_SKMT_H_

#include "firmware/key_manifest.h"
#include "tip_firmware_component.h"


/**
 * Maximum number of public keys in secondary key manifest.
 */
#define SKMT_MAX_KEY_NUM 10

/**
 * Public key type in TIP secondary key manifest.
 */
typedef enum {
	SKMT_KEY_RSA = 0x520A,
	SKMT_KEY_ECC = 0x450A,
	SKMT_KEY_AES = 0x410A,
	SKMT_KEY_CERT = 0x430A,
	SKMT_KEY_LMS = 0x6c0A,
} SKMT_KEY_TYPE_E;

/**
 * Public key residing in TIP secondary key manifest.
 */
struct skmt_key {
	SKMT_KEY_TYPE_E skmt_type; /**< Public key type. */
	uint16_t skmt_key_size;	   /**< Public key size. */
	uint8_t *skmt_key;		   /**< The in-memory buffer for public keys. */
};

/**
 * TIP secondary key manifest.
 */
struct tip_secondary_key_manifest {
	struct key_manifest base;						/**< Key manifest base instance. */
	struct tip_firmware_component *fw;				/**< Firmware component handler. SKMT is also a firmware component. */
	int num_of_keys;								/**< The number of public keys available inside. */
	uint32_t ram_copy_addr; 						/**< Copied by L0 on top of KMT (usually it's MANIFEST_KEY_ADDRESS_COPY). */
	struct skmt_key keys[SKMT_MAX_KEY_NUM];			/**< Public keys parsed from SKMT. */
	struct key_manifest_public_key manifest_key;	/**< Public key used for manifest verification. */

};


int tip_skmt_init (struct tip_secondary_key_manifest *skmt, struct tip_firmware_component *skmt_img,
	uint32_t verified_skmt_addr);
int tip_skmt_copy (struct tip_secondary_key_manifest *skmt, uint32_t addr);
int tip_skmt_parse (struct tip_secondary_key_manifest *skmt);
int tip_skmt_key_lookup (struct tip_secondary_key_manifest *skmt, int id, uint8_t **key,
	size_t *length);


#endif /* TIP_SKMT_H_ */
