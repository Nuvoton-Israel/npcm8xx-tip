// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "riot_core_tip.h"
#include "common/buffer_util.h"
#include "tip_boot.h"
#include "platform_io.h"
#include "logging/debug_log.h"
#include "boot_logging.h"


int riot_core_tip_generate_device_id (const struct riot_core *riot, const uint8_t *cdi,
	size_t length)
{
	struct riot_core_tip *core = (struct riot_core_tip*) riot;
	uint8_t cdi_be[ECC_MAX_KEY_LENGTH] = { 0 };
	int status;
	bool cdi_empty;

	if ((core == NULL) || (cdi == NULL) || (length == 0) || (length > SHA512_HASH_LENGTH)) {
		return RIOT_CORE_INVALID_ARGUMENT;
	}

	/* Original CDI is stored in little endian format. Convert it to big endian format to meet ECC
	 * engine key generation API requirement. */
	buffer_reverse_copy (cdi_be, cdi, core->base.key_length);

	/* Check CDI contains random data. */
	cdi_empty = true;
	for (int i = 0; i < length; i++) {
		if (cdi[i] != 0) {
			cdi_empty = false;
		}
	}

	/* If device life cycle is not production - init CDI to counter.
	 * This allows the chip to boot to uboot (but fail at attestation server).
	 * For production device boot will fail.
	 */  
	if (cdi_empty == true) {
		int i;
		uint8_t life_cycle;
		FUSE_WRPR_get (LIFE_CYCLE_ENC_PROPERTY, &life_cycle);

		platform_printf (
			KRED "NO DICE IDENTITY ON DEVICE! CHIP CAN BE USED FOR DEBUG ONLY."
			"LIFE CYCLE 0x%x" NEWLINE KNRM, life_cycle);

		debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_RIOT,
								BOOT_LOGGING_DICE_ERROR, RIOT_CORE_NO_CDI, life_cycle);

		if (life_cycle != LIFE_CYCLE_PRODUCTION) {
			for (i = 0; i < length; i++) {
				cdi_be[i] = i;
			}
		} else {
			return RIOT_CORE_NO_CDI;
		}
	}

	/* Generate device ID private key from the first 48 bytes of CDI as TIP ROM does. */
	status = core->base.ecc->generate_derived_key_pair (core->base.ecc, cdi_be,
		core->base.key_length, &core->base.state->dev_id, NULL);
	if (status != 0) {
		return status;
	}

	status = hash_calculate (core->base.hash, core->base.state->hash_algo, cdi_be, sizeof (cdi_be),
		core->base.state->cdi_hash, sizeof (core->base.state->cdi_hash));
	if (ROT_IS_ERROR (status)) {
		return status;
	}

	riot_core_clear (cdi_be, sizeof (cdi_be));

	return riot_core_common_create_device_id_certificate (&core->base);
}

/**
 * Initialize TIP RIoT Core to be ready for RIoT operations.
 *
 * @param riot TIP RIoT Core instance to initialize.
 * @param state Variable context for the DICE handler. This must be uninitialized.
 * @param hash The hash engine to use with RIoT Core.
 * @param ecc The ECC engine to use with RIoT Core.
 * @param x509 The X.509 certificate engine to use with RIoT Core.
 * @param base64 The base64 encoding engine to use with RIoT Core.
 * @param key_length Length of the DICE keys that should be created.
 * @param device_id_ext A list of additional, custom extensions that should be added to the
 * Device ID certificate and CSR.  At minimum, this should include the DICE TcbInfo extension for
 * layer 0.
 * @param device_id_ext_count The number of custom extensions to add to the Device ID certificate
 * and CSR.
 * @param alias_ext A list of additional, custom extensions that should be added to the
 * Alias certificate.  At minimum, this should include the DICE TcbInfo extension for layer 1.
 * @param alias_ext_count The number of custom extensions to add to the Alias certificate.
 *
 * @return 0 if RIoT Core was been initialize successfully or an error code.
 */
int riot_core_tip_init (struct riot_core_tip *riot, struct riot_core_common_state *state,
	struct hash_engine *hash, struct ecc_engine *ecc, struct x509_engine *x509,
	struct base64_engine *base64, size_t key_length,
	const struct x509_extension_builder *const *device_id_ext, size_t device_id_ext_count,
	const struct x509_extension_builder *const *alias_ext, size_t alias_ext_count)
{
	int status;

	if (riot == NULL) {
		return RIOT_CORE_INVALID_ARGUMENT;
	}

	status = riot_core_common_init (&riot->base, state, hash, ecc, x509, base64, key_length,
		device_id_ext, device_id_ext_count, alias_ext, alias_ext_count);
	if (status != 0) {
		return status;
	}

	/* TIP needs a customized device ID generation workflow. */
	riot->base.base.generate_device_id = riot_core_tip_generate_device_id;

	return 0;
}

/**
 * Release TIP RIoT core and zeroize all internal state with private data.
 *
 * It is imperative that all RIoT Core instances be released before starting the next application
 * stage, even if it's not necessary from a resource management perspective.  Releasing the RIoT
 * Core instance ensures that private data in memory is zeroized.
 *
 * @param riot The TIP RIoT Core to release.
 */
void riot_core_tip_release (struct riot_core_tip *riot)
{
	if (riot != NULL) {
		riot_core_common_release (&riot->base);
	}
}