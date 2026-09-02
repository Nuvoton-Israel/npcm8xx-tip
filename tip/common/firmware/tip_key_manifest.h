// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_KEY_MANIFEST_H_
#define TIP_KEY_MANIFEST_H_

#include "firmware/key_manifest.h"
#include "tip_firmware_component.h"
#include "crypto/ecc_hw.h"

/**
 * Maximum number of public keys in primary key manifest.
 * All users need 1 or 2 keys (one key for SKMT signing, and one key for L0 signing).
 * keys may be shared so can use a single KMT key.
 * KMT with header must be up to 0x320 (limited by the size of the log).
 * So maximum theoretical is 4 keys: 256 bytes header + 4 keys of 128 bytes < 0x320.
 * Warning: KMT is part of DICE identity: recommend do not change often.
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
	const struct ecc_hw *ecc;		   /**< ECC engine used to verify the key manifest signature. */
};

int tip_key_manifest_init (struct tip_primary_key_manifest *kmt,
	struct tip_firmware_component *kmt_img);
int tip_key_manifest_parse_and_copy (struct tip_primary_key_manifest *kmt,
	const uint8_t *verified_kmt, uint32_t kmt_copy_addr, bool copy_to_ram);
int tip_invalidate_otp_keys (void);


#endif /* TIP_KEY_MANIFEST_H_ */