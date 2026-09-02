// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_KEY_MANIFEST_H_
#define TIP_KEY_MANIFEST_H_

#include "firmware/key_manifest.h"
#include "tip_firmware_component.h"

/**
 * Maximum number of public keys in primary key manifest.
 * All users need 1 or 2 keys. Adding 4 keys limit will allows saving TIP_RAM
 * space.
 */
#define KMT_MAX_KEY_NUM 4

/**
 * TIP primary key manifest.
 * It holds two public keys which are used to verify L0 and secondary kmt.
 */
struct tip_primary_key_manifest {
	struct key_manifest base;		   /**< Key manifest base instance. */
	struct tip_firmware_component *fw; /**< Firmware component handler. KMT is also a firmware component. */
	bool ram_copy_valid; 			   /**< KMT will be overriden by SKMT, so it gets invalidated after SKMT loading. */
	uint8_t num_of_kmt_keys;      /**< total number of key in the KMT. Typically 2 keys */
};


int tip_key_manifest_init (struct tip_primary_key_manifest *kmt,
	struct tip_firmware_component *kmt_img);
int tip_key_manifest_parse_and_copy (struct tip_primary_key_manifest *kmt,
	const uint8_t *verified_kmt, uint32_t kmt_copy_addr, bool copy_to_ram);
int tip_invalidate_otp_keys (void);


#endif /* TIP_KEY_MANIFEST_H_ */