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
#include "common/unused.h"
#include "crypto/crypto_logging.h"
#include "logging/debug_log.h"
#include "platform_io.h"
#include "tip_ecc_hw_ncl.h"
#include "tip_utils.h"
#include "tip_rom_ncl_shared_state.h"


#define EXTRA_BYTES				2
#define NCL_MAX_ECC_KEY_UINT32 	(ECC_MAX_KEY_LENGTH + EXTRA_BYTES) / 4

extern ROM_NCL_ECC_InitContext 						pROM_NCL_ECC_InitContext;
extern ROM_NCL_ECC_FinalizeContext 					pROM_NCL_ECC_FinalizeContext;
extern ROM_NCL_ECC_Init 							pROM_NCL_ECC_Init;
extern ROM_NCL_ECC_Power 							pROM_NCL_ECC_Power;
extern ROM_NCL_ECC_SetCurve 						pROM_NCL_ECC_SetCurve;
extern ROM_NCL_ECC_SetSeed 							pROM_NCL_ECC_SetSeed;
extern ROM_NCL_ECC_GenKey 							pROM_NCL_ECC_GenKey;
extern ROM_NCL_ECC_ECDHKeyExchange 					pROM_NCL_ECC_ECDHKeyExchange;
extern ROM_NCL_ECC_ECDSASignHash 					pROM_NCL_ECC_ECDSASignHash;
extern ROM_NCL_ECC_ECDSAVerifyHash 					pROM_NCL_ECC_ECDSAVerifyHash;
extern ROM_NCL_ECC_PointVerify 						pROM_NCL_ECC_PointVerify;
extern ROM_NCL_ECC_GetCurveData 					pROM_NCL_ECC_GetCurveData;
extern ROM_NCL_SHA_Power 							pROM_NCL_SHA_Power;
extern ROM_NCL_DRBG_InitContext 					pROM_NCL_DRBG_InitContext;
extern ROM_NCL_DRBG_Init 							pROM_NCL_DRBG_Init;
extern ROM_NCL_DRBG_Instantiate 					pROM_NCL_DRBG_Instantiate;
extern ROM_NCL_DRBG_FinalizeContext 				pROM_NCL_DRBG_FinalizeContext;

/**
 * kdf function for ecc gen key- copies the data
 *  @param        data
 *  @param        out - can be NULL
 *  @param        size
 *  @return       DEFS_STATUS 0 if pass or an error code
 */
static NCL_STATUS_T tip_kdf_mem_cpy (void *data, uint8_t *out, uint32_t size)
{
	DEFS_STATUS_COND_CHECK (data != NULL, NCL_STATUS_FAIL);
	if (out != NULL) {
		for (uint32_t i = 0; i < size; i++) {
			out[i] = ((uint8_t*) data)[i];
		}
	}
	return NCL_STATUS_OK;
}

/**
 * tip_get_key_sizes function for all hw functions - updates the values according to the key size
 * digest size
 *  @param        key_length
 *  @param        keySize
 *  @param        curve_type
 *  @param        start_address
 *  @param        key_length_extended
 */
static void tip_get_key_sizes (size_t key_length, uint32_t *keySize, NCL_ECC_CURVE_T *curve_type,
	int *start_address, int *key_length_extended)
{
	switch (key_length) {
		case ECC_KEY_LENGTH_256:
			*keySize = ECC_KEY_SIZE_DWORD_256;
			*curve_type = NCL_ECC_CURVE_NIST_P_256;
			break;
#if ECC_MAX_KEY_LENGTH >= ECC_KEY_LENGTH_384
		case ECC_KEY_LENGTH_384:
			*keySize = ECC_KEY_SIZE_DWORD_384;
			*curve_type = NCL_ECC_CURVE_NIST_P_384;
			break;
#if ECC_MAX_KEY_LENGTH >= ECC_KEY_LENGTH_521
		case ECC_KEY_LENGTH_521:
			*keySize = ECC_KEY_SIZE_DWORD_521;
			*curve_type = NCL_ECC_CURVE_NIST_P_521;
			*start_address += EXTRA_BYTES;
			*key_length_extended += EXTRA_BYTES;
			break;
#endif
#endif
		default:
			break;
	}
}

/**
* tip_get_key_sizes function for all hw functions - updates the values according to the key size
* digest size
*  @param		 digest_length
*  @param		 hash_alg

*/
static void tip_get_hash_sizes (size_t digest_length, NCL_SHA_TYPE_T *hash_alg)
{
	switch (digest_length) {
		case SHA256_HASH_LENGTH:
			*hash_alg = NCL_SHA_TYPE_2_256;
			break;

		case SHA384_HASH_LENGTH:
			*hash_alg = NCL_SHA_TYPE_2_384;
			break;

		case SHA512_HASH_LENGTH:
			*hash_alg = NCL_SHA_TYPE_2_512;
			break;

		default:
			break;
	}
}

static void tip_ecc_get_curve_data (NCL_ECC_CURVE_DATA_T **curve_output, void *context,
	NCL_ECC_CURVE_T curve_type, size_t key_length)
{
	*curve_output = pROM_NCL_ECC_GetCurveData (context, curve_type);
}


int tip_ecc_hw_ncl_engine_get_ecc_public_key (const struct ecc_hw *ecc_hw,
	const uint8_t *priv_key, size_t key_length, struct ecc_point_public_key *pub_key)
{
	const struct tip_ecc_hw_ncl_engine *tip_ncl = (const struct tip_ecc_hw_ncl_engine*) ecc_hw;
	NCL_STATUS_T ncl_status;
	uint8_t priv_key_copy[ECC_MAX_KEY_LENGTH + EXTRA_BYTES] = { 0 };
	uint32_t prv_key_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_x_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_y_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t keySize = ECC_KEY_SIZE_DWORD_256;
	NCL_ECC_CURVE_T curve_type = NCL_ECC_CURVE_NIST_P_256;
	int key_length_extended = key_length;
	int start_address = 0;
	NCL_BN_T prv_key_ncl;
	NCL_ECC_POINT_T pub_key_ncl;
	NCL_ECC_CURVE_DATA_T *curve = NULL;

	if (tip_ncl == NULL) {
		return ECC_HW_INVALID_ARGUMENT;
	}
	/* get all sizes according to key_length*/
	tip_get_key_sizes (key_length, &keySize, &curve_type, &start_address, &key_length_extended);

	/*copy the input params into NCL params, with option of padding for ecc521 case */
	memcpy (priv_key_copy + start_address, priv_key, key_length);
	prv_key_ncl.size = keySize;
	prv_key_ncl.number = (uint32_t*) prv_key_arr;
	pub_key_ncl.X.number = (uint32_t*) pub_key_x_arr;
	pub_key_ncl.X.size = keySize;
	pub_key_ncl.Y.number = (uint32_t*) pub_key_y_arr;
	pub_key_ncl.Y.size = keySize;

	/* ECC Gen Key creates private and public in little endian, therefore need to swap it before
	 * creation */
	tip_mem_swap_endianness (priv_key_copy, NULL, key_length_extended);

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_InitContext (tip_ncl->state->context),
		tip_ncl->shared_state);

	tip_ecc_get_curve_data(&curve, tip_ncl->state->context, curve_type, key_length);

	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_SetCurve (tip_ncl->state->context, curve),
		tip_ncl->shared_state);

	ncl_status = pROM_NCL_ECC_GenKey (tip_ncl->state->context, &prv_key_ncl, &pub_key_ncl, curve,
		tip_kdf_mem_cpy, (void*) priv_key_copy);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

	/* ECC Gen Key creates private and public in little endian, therefore need to swap it after
	 * creation */
	if (ncl_status == NCL_STATUS_OK) {
		tip_mem_swap_endianness (pub_key_ncl.X.number, NULL, key_length_extended);
		tip_mem_swap_endianness (pub_key_ncl.Y.number, NULL, key_length_extended);

		if (key_length <= sizeof (pub_key->x) && key_length <= sizeof (pub_key->y)) {
			/* copy the NCL output public into output param, remove the padding in ecc521 case */
			pub_key->key_length = key_length;
			memcpy (pub_key->x, (uint8_t*) pub_key_ncl.X.number + start_address, key_length);
			memcpy (pub_key->y, (uint8_t*) pub_key_ncl.Y.number + start_address, key_length);
		}
	}
	return tip_ncl_status (ncl_status, ECC_HW_ECC_PUBLIC_FAILED);
}

int tip_ecc_hw_ncl_engine_generate_ecc_key_pair (const struct ecc_hw *ecc_hw,
	size_t key_length, uint8_t *priv_key, struct ecc_point_public_key *pub_key)
{
	const struct tip_ecc_hw_ncl_engine *tip_ncl = (const struct tip_ecc_hw_ncl_engine*) ecc_hw;
	NCL_STATUS_T ncl_status;

	uint32_t prv_key_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_x_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_y_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t keySize = ECC_KEY_SIZE_DWORD_256;
	NCL_ECC_CURVE_T curve_type = NCL_ECC_CURVE_NIST_P_256;
	int key_length_extended = key_length;
	int start_address = 0;
	NCL_BN_T prv_key_ncl;
	NCL_ECC_POINT_T pub_key_ncl;
	NCL_ECC_CURVE_DATA_T *curve = NULL;

	if (tip_ncl == NULL) {
		return ECC_HW_INVALID_ARGUMENT;
	}
	/* get all sizes according to key_length */
	tip_get_key_sizes (key_length, &keySize, &curve_type, &start_address, &key_length_extended);

	prv_key_ncl.size = keySize;
	prv_key_ncl.number = (uint32_t*) prv_key_arr;
	pub_key_ncl.X.number = (uint32_t*) pub_key_x_arr;
	pub_key_ncl.X.size = keySize;
	pub_key_ncl.Y.number = (uint32_t*) pub_key_y_arr;
	pub_key_ncl.Y.size = keySize;

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_InitContext (tip_ncl->state->context),
		tip_ncl->shared_state);
	tip_ecc_get_curve_data (&curve, tip_ncl->state->context, curve_type, key_length);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_SetCurve (tip_ncl->state->context, curve),
		tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_SetSeed (tip_ncl->state->context, 0xffffffff),
		tip_ncl->shared_state);
	ncl_status = pROM_NCL_ECC_GenKey (tip_ncl->state->context, &prv_key_ncl, &pub_key_ncl, curve,
		NULL, NULL);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

	if (ncl_status == NCL_STATUS_OK) {
		/* ECC Gen Key creates private and public in little endian, therefore need to swap it after
		 * creation */
		tip_mem_swap_endianness (pub_key_ncl.X.number, NULL, key_length_extended);
		tip_mem_swap_endianness (pub_key_ncl.Y.number, NULL, key_length_extended);
		tip_mem_swap_endianness (prv_key_ncl.number, NULL, key_length_extended);

		if ((key_length <= sizeof (pub_key->x)) && (key_length <= sizeof (pub_key->y)) &&
			(key_length <= sizeof (priv_key) * keySize)) {
			pub_key->key_length = key_length;
			/* copy the NCL output key into output params, remove the padding in ecc521 case */
			memcpy (pub_key->x, (uint8_t*) pub_key_ncl.X.number + start_address, key_length);
			memcpy (pub_key->y, (uint8_t*) pub_key_ncl.Y.number + start_address, key_length);
			memcpy (priv_key, (uint8_t*) prv_key_ncl.number + start_address, key_length);
		}
	}

	return tip_ncl_status (ncl_status, ECC_HW_ECC_GENERATE_FAILED);
}

int tip_ecc_hw_ncl_engine_ecdsa_sign (const struct ecc_hw *ecc_hw, const uint8_t *priv_key,
	size_t key_length, const uint8_t *digest, size_t digest_length, const struct rng_engine *rng,
	struct ecc_ecdsa_signature *signature)
{
	const struct tip_ecc_hw_ncl_engine *tip_ncl = (const struct tip_ecc_hw_ncl_engine*) ecc_hw;

	if (rng != NULL) {
		return ECC_HW_UNSUPPORTED_OP;
	}

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
	prv_key_ncl.number = (uint32_t*) prv_key_arr;
	rSig.number = (uint32_t*) r_sig_arr;
	rSig.size = key_size;
	sSig.number = (uint32_t*) s_sig_arr;
	sSig.size = key_size;

	/* copy the input params into NCL params, with option of padding for ecc521 case */
	memcpy ((uint8_t*) prv_key_ncl.number + start_address, priv_key, key_length);
	/* need to swap private key input endiandess before signing*/
	tip_mem_swap_endianness ((uint8_t*) prv_key_ncl.number, NULL, key_length_extended);

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_InitContext (tip_ncl->state->context),
		tip_ncl->shared_state);

	tip_ecc_get_curve_data (&curve, tip_ncl->state->context, curve_type, key_length);
	ncl_status = pROM_NCL_ECC_SetCurve (tip_ncl->state->context, curve);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_SetSeed (tip_ncl->state->context, 0xffffffff),
		tip_ncl->shared_state);
	ncl_status = pROM_NCL_ECC_ECDSASignHash (tip_ncl->state->context, &rSig, &sSig, hash_alg,
		&prv_key_ncl, digest, curve, NULL, NULL);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

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

	return tip_ncl_status (ncl_status, ECC_HW_ECDSA_SIGN_FAILED);
}

int tip_ecc_hw_ncl_engine_ecdsa_verify (const struct ecc_hw *ecc_hw,
	const struct ecc_point_public_key *pub_key, const struct ecc_ecdsa_signature *signature,
	const uint8_t *digest, size_t digest_length)
{
	const struct tip_ecc_hw_ncl_engine *tip_ncl = (const struct tip_ecc_hw_ncl_engine*) ecc_hw;
	uint32_t r_sig_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t s_sig_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_x_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_y_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	NCL_BN_T rSig, sSig;
	NCL_ECC_POINT_T pub_key_ncl;
	NCL_SHA_TYPE_T hash_alg = NCL_SHA_TYPE_2_256;
	uint32_t key_size = ECC_KEY_SIZE_DWORD_256;
	NCL_ECC_CURVE_T curve_type = NCL_ECC_CURVE_NIST_P_256;
	NCL_STATUS_T sigOk = NCL_STATUS_FAIL;
	int start_address = 0;
	int key_length;
	int key_length_extended;

	NCL_ECC_CURVE_DATA_T *curve = NULL;
	if ((tip_ncl == NULL) || (digest == NULL) || (signature == NULL)) {
		return ECC_HW_INVALID_ARGUMENT;
	}

	key_length = key_length_extended = pub_key->key_length;
	/* get all sizes according to key_length and digest_length */
	tip_get_key_sizes (key_length, &key_size, &curve_type, &start_address, &key_length_extended);
	tip_get_hash_sizes (digest_length, &hash_alg);

	/* now that key_size is known, can define NCL params */
	rSig.number = (uint32_t*) r_sig_arr;
	rSig.size = key_size;
	sSig.number = (uint32_t*) s_sig_arr;
	sSig.size = key_size;

	pub_key_ncl.X.number = (uint32_t*) pub_key_x_arr;
	pub_key_ncl.X.size = key_size;
	pub_key_ncl.Y.number = (uint32_t*) pub_key_y_arr;
	pub_key_ncl.Y.size = key_size;

	/* copy the input params into NCL params, with option of padding for ecc521 case */
	memcpy ((uint8_t*) rSig.number + start_address, (signature->r), key_length);
	memcpy ((uint8_t*) sSig.number + start_address, (signature->s), key_length);
	memcpy ((uint8_t*) pub_key_ncl.X.number + start_address, (pub_key->x), key_length);
	memcpy ((uint8_t*) pub_key_ncl.Y.number + start_address, (pub_key->y), key_length);

	/* need to swap signature and key endiandess before using them to verify */
	tip_mem_swap_endianness ((uint8_t*) pub_key_ncl.X.number, NULL, key_length_extended);
	tip_mem_swap_endianness ((uint8_t*) pub_key_ncl.Y.number, NULL, key_length_extended);
	tip_mem_swap_endianness ((uint8_t*) rSig.number, NULL, key_length_extended);
	tip_mem_swap_endianness ((uint8_t*) sSig.number, NULL, key_length_extended);

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_InitContext (tip_ncl->state->context),
		tip_ncl->shared_state);

	tip_ecc_get_curve_data (&curve, tip_ncl->state->context, curve_type, key_length);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_SetCurve (tip_ncl->state->context, curve),
		tip_ncl->shared_state);
	/* SetSeed is only required for signing (k generation); ignore failure for verify */
	pROM_NCL_ECC_SetSeed (tip_ncl->state->context, 0xffffffff);

	/*
	 * Do NOT use NCL_STATUS_RET_CHECK_MUTEX here.  That macro returns `ret` directly on
	 * any value != NCL_STATUS_OK, including NCL_STATUS_OK_Z1 (0), which Cerberus callers
	 * interpret as success — bypassing the sigOk check entirely.
	 *
	 * Instead: call the ROM directly, unlock unconditionally, then cross-validate both
	 * `ret` (function status) and `sigOk` (verify result) with positive equality checks
	 * plus an anti-glitch re-verify before committing to success.
	 */
	volatile NCL_STATUS_T verify_ret = pROM_NCL_ECC_ECDSAVerifyHash (tip_ncl->state->context,
		&sigOk, &rSig, &sSig, hash_alg, digest, &pub_key_ncl, curve);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

	if (verify_ret == NCL_STATUS_OK) {
		if (sigOk == NCL_STATUS_OK) {
			/* anti-glitch re-check: both values must still hold */
			if (verify_ret != NCL_STATUS_OK || sigOk != NCL_STATUS_OK) {
				return ECC_HW_ECDSA_BAD_SIGNATURE;
			}
			return 0;
		}
	}
	return ECC_HW_ECDSA_BAD_SIGNATURE;
}

int tip_ecc_hw_ncl_engine_ecdh_compute (const struct ecc_hw *ecc_hw,
	const uint8_t *priv_key, size_t key_length, const struct ecc_point_public_key *pub_key,
	uint8_t *secret, size_t length)
{
	const struct tip_ecc_hw_ncl_engine *tip_ncl = (const struct tip_ecc_hw_ncl_engine*) ecc_hw;
	uint32_t prv_key_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_x_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_y_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t keySize = ECC_KEY_SIZE_DWORD_256;
	NCL_ECC_CURVE_T curve_type = NCL_ECC_CURVE_NIST_P_256;
	int key_length_extended = key_length;
	int start_address = 0;
	uint32_t r_s_length;
	NCL_BN_T prv_key_ncl;
	NCL_ECC_POINT_T pub_key_ncl, shared_sec;

	NCL_ECC_CURVE_DATA_T *curve = NULL;
	NCL_STATUS_T ncl_status;
	if ((tip_ncl == NULL) || (priv_key == NULL) || (pub_key == NULL) || (secret == NULL)) {
		return ECC_HW_INVALID_ARGUMENT;
	}

	/*get all sizes according to key_length */
	tip_get_key_sizes (key_length, &keySize, &curve_type, &start_address, &key_length_extended);

	prv_key_ncl.size = keySize;
	prv_key_ncl.number = (uint32_t*) prv_key_arr;
	pub_key_ncl.X.number = (uint32_t*) pub_key_x_arr;
	pub_key_ncl.X.size = keySize;
	pub_key_ncl.Y.number = (uint32_t*) pub_key_y_arr;
	pub_key_ncl.Y.size = keySize;

	/* copy the input params into NCL params, with option of padding for ecc521 case */
	memcpy ((uint8_t*) pub_key_ncl.X.number + start_address, (pub_key->x), key_length);
	memcpy ((uint8_t*) pub_key_ncl.Y.number + start_address, (pub_key->y), key_length);
	memcpy ((uint8_t*) prv_key_ncl.number + start_address, priv_key, key_length);

	/* r or s length is the size of the curve_local (mult by 4 because uint32)*/
	r_s_length = keySize * 4;

	shared_sec.X.number = (uint32_t*) pub_key_x_arr;
	shared_sec.X.size = keySize;

	shared_sec.Y.number = (uint32_t*) pub_key_y_arr;
	shared_sec.Y.size = keySize;

	/* need to swap key endiandess before using them to perform key exchange */
	tip_mem_swap_endianness ((uint8_t*) pub_key_ncl.X.number, NULL, keySize * 4);
	tip_mem_swap_endianness ((uint8_t*) pub_key_ncl.Y.number, NULL, keySize * 4);
	tip_mem_swap_endianness ((uint8_t*) prv_key_ncl.number, NULL, keySize * 4);

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_InitContext (tip_ncl->state->context),
		tip_ncl->shared_state);

	tip_ecc_get_curve_data (&curve, tip_ncl->state->context, curve_type, key_length);
	ncl_status = pROM_NCL_ECC_SetCurve (tip_ncl->state->context, curve);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_SetSeed (tip_ncl->state->context, 0xffffffff),
		tip_ncl->shared_state);
	ncl_status = pROM_NCL_ECC_ECDHKeyExchange (tip_ncl->state->context, &shared_sec, &prv_key_ncl,
		&pub_key_ncl, curve);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

	if (ncl_status == NCL_STATUS_OK) {
		/* need to swap shared secret endiandess before returning it*/
		tip_mem_swap_endianness ((uint8_t*) shared_sec.X.number, NULL, r_s_length);
		tip_mem_swap_endianness ((uint8_t*) shared_sec.Y.number, NULL, r_s_length);
		/* copy the NCL output secret into output param, remove the padding in ecc521 case */
		memcpy (secret, (uint8_t*) shared_sec.X.number + start_address, key_length);
		memcpy (secret + r_s_length, (uint8_t*) shared_sec.Y.number + start_address, key_length);
	}
	return tip_ncl_status (ncl_status, ECC_HW_ECDH_COMPUTE_FAILED);
}

#ifdef ENABLE_TIP_ECC_HW_PUBLIC_KEY_VERIFICATION
int tip_ecc_hw_ncl_engine_verify_ecc_public_key (const struct ecc_hw *ecc_hw,
	const struct ecc_point_public_key *pub_key)
{
	UNUSED (ecc_hw);
	UNUSED (pub_key);

	/* TODO: add check for whether ECC public key is valid for FIPs compliance.
	 * For now, return success and allow signature verification to fail downstream if not valid. */
	return 0;
}
#endif

/**
 * Initialize only the variable state of an NCL ECC engine.  The rest of the instance is
 * assumed to already have been initialized.
 *
 * This would generally be used with a statically initialized instance.
 *
 * @param engine The ECC engine that contains the state to initialize.
 *
 * @return 0 if the state was successfully initialized or an error code.
 */
int tip_ecc_hw_ncl_init_state (const struct tip_ecc_hw_ncl_engine *engine)
{
	if ((engine == NULL) || (engine->state == NULL)) {
		return ECC_HW_INVALID_ARGUMENT;
	}

	memset (engine->state, 0, sizeof (*engine->state));

	engine->shared_state->lock (engine->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_Power (engine->state->context, TRUE),
		engine->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_InitContext (engine->state->context),
		engine->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_ECC_Init (engine->state->context),
		engine->shared_state);
	engine->shared_state->unlock (engine->shared_state);

	return 0;
}

/**+
 * Initialize an instance for running ECC operations using tipncl.
 *
 * @param engine The ECC engine to initialize.
 * @param state The variable context for ECC engine.  This must be uninitialized.
 *
 * @return 0 if the engine was successfully initialized or an error code.
 */
int tip_ecc_hw_ncl_init (struct tip_ecc_hw_ncl_engine *engine, struct tip_ecc_hw_ncl_state *state,
	struct tip_rom_ncl_shared_state *shared_state)
{
	if ((engine == NULL) || (shared_state == NULL)) {
		return ECC_HW_INVALID_ARGUMENT;
	}

	memset (engine, 0, sizeof (struct tip_ecc_hw_ncl_engine));

	engine->base.get_ecc_public_key = tip_ecc_hw_ncl_engine_get_ecc_public_key;
	engine->base.generate_ecc_key_pair = tip_ecc_hw_ncl_engine_generate_ecc_key_pair;
	engine->base.ecdsa_verify = tip_ecc_hw_ncl_engine_ecdsa_verify;
	engine->base.ecdh_compute = tip_ecc_hw_ncl_engine_ecdh_compute;
	engine->base.ecdsa_sign = tip_ecc_hw_ncl_engine_ecdsa_sign;
#ifdef ENABLE_TIP_ECC_HW_PUBLIC_KEY_VERIFICATION
	engine->base.verify_ecc_public_key = tip_ecc_hw_ncl_engine_verify_ecc_public_key;
#endif
	engine->state = state;
	engine->shared_state = shared_state;

	return tip_ecc_hw_ncl_init_state (engine);
}

/**
 * Release an tipncl ECC engine.
 *
 * @param engine The ECC engine to release.
 */
void tip_ecc_hw_ncl_release (struct tip_ecc_hw_ncl_engine *engine)
{
	if (engine && engine->shared_state) {
		engine->shared_state->lock (engine->shared_state);
		pROM_NCL_ECC_FinalizeContext (engine->state->context);
		engine->shared_state->unlock (engine->shared_state);
	}
}
