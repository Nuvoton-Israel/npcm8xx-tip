/*
 *  Nuvoton Technology Corporation confidential
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *<<<----------------------------------------------------------------------
 * File Contents:
 *   tip_ecc_hw_ncl.c
 *            This file contains API of security routines for ROM code ECC engine
 *  Project:
 *            Arbel
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "tip_ecc_hw_ncl.h"
#include "tip_ecc_hw_ncl_common.h"
#include "crypto/crypto_logging.h"
#include "logging/debug_log.h"
#include "platform_io.h"
#include "tip_utils.h"


static int tip_ecc_hw_ncl_engine_ecdsa_sign (const struct ecc_hw *ecc_hw, const uint8_t *priv_key,
	size_t key_length, const uint8_t *digest, size_t digest_length,
	struct ecc_ecdsa_signature *signature)
{
	struct tip_ecc_hw_ncl_engine *tip_ncl = (struct tip_ecc_hw_ncl_engine *) ecc_hw;

	uint32_t prv_key_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t r_sig_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t s_sig_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	NCL_BN_T prv_key_ncl, rSig, sSig;
	NCL_STATUS_T ncl_status;
	NCL_SHA_TYPE_T hash_alg = NCL_SHA_TYPE_2_256;
	uint32_t key_size = ECC_KEY_SIZE_DWORD_256;
	NCL_ECC_CURVE_T curve_type = NCL_ECC_CURVE_NIST_P_256;
	int start_address = 0;
	int key_length_extended = key_length;
	NCL_ECC_CURVE_DATA_T *curve = NULL;


	if ((tip_ncl == NULL) || (digest == NULL) || (signature == NULL)) {
		return ECC_HW_INVALID_ARGUMENT;
	}

	/* get all sizes according to key_length and digest_length */
	tip_get_key_sizes (key_length, &key_size, &curve_type, &start_address, &key_length_extended);
	tip_get_hash_sizes (digest_length, &hash_alg);

	signature->length = key_length;
	prv_key_ncl.size = key_size;
	prv_key_ncl.number = (uint32_t *) prv_key_arr;
	rSig.number = (uint32_t *) r_sig_arr;
	rSig.size = key_size;
	sSig.number = (uint32_t *) s_sig_arr;
	sSig.size = key_size;

	/* copy the input params into NCL params, with option of padding for ecc521 case */
	memcpy ((uint8_t*) prv_key_ncl.number + start_address, priv_key, key_length);
	/* need to swap private key input endiandess before signing*/
	tip_mem_swap_endianness ((uint8_t*) prv_key_ncl.number, NULL, key_length_extended);
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_InitContext (tip_ncl->context));

	tip_ecc_get_curve_data (&curve, tip_ncl->context, curve_type, key_length);
	ncl_status = pROM_NCL_ECC_SetCurve (tip_ncl->context, curve);
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_SetSeed (tip_ncl->context, 0xffffffff));
	ncl_status = pROM_NCL_ECC_ECDSASignHash (tip_ncl->context, &rSig, &sSig, hash_alg,
		&prv_key_ncl, digest, curve, NULL, NULL);

	/* need to swap signature endiandess before returning it */
	if (ncl_status == NCL_STATUS_OK) {
		tip_mem_swap_endianness ((uint8_t*) rSig.number, NULL, key_length_extended);
		tip_mem_swap_endianness ((uint8_t*) sSig.number, NULL, key_length_extended);

		if (key_length <= sizeof (signature->r) && key_length <= sizeof (signature->s)) {
			/* copy the NCL output signature into output param, remove the padding in ecc521 case */
			memcpy (signature->r, (uint8_t*) rSig.number + start_address, key_length);
			memcpy (signature->s, (uint8_t*) sSig.number + start_address, key_length);
		}
	}

	return tip_ncl_status (NCL_STATUS_OK, ECC_HW_ECDSA_SIGN_FAILED);
}

/**+
 * Initialize an instance for running ECC operations using tipncl.
 *
 * @param engine The ECC engine to initialize.
 *
 * @return 0 if the engine was successfully initialized or an error code.
 */
int tip_ecc_hw_ncl_init (struct tip_ecc_hw_ncl_engine *engine)
{
	int status;

	status = tip_ecc_hw_ncl_common_init (engine);
	if (status != 0) {
		return status;
	}

	engine->base.ecdsa_sign = tip_ecc_hw_ncl_engine_ecdsa_sign;

	return 0;
}

/**
 * Release an tipncl ECC engine.
 *
 * @param engine The ECC engine to release.
 */
void tip_ecc_hw_ncl_release (struct tip_ecc_hw_ncl_engine *engine)
{
	return tip_ecc_hw_ncl_common_release (engine);
}
