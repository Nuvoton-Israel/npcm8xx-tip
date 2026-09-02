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
#include "crypto/crypto_logging.h"
#include "logging/debug_log.h"
#include "platform_io.h"
#include "tip_utils.h"

extern ROM_NCL_ECC_InitContext pROM_NCL_ECC_InitContext;
extern ROM_NCL_ECC_FinalizeContext pROM_NCL_ECC_FinalizeContext;
extern ROM_NCL_ECC_Init pROM_NCL_ECC_Init;
extern ROM_NCL_ECC_Power pROM_NCL_ECC_Power;
extern ROM_NCL_ECC_SetCurve pROM_NCL_ECC_SetCurve;
extern ROM_NCL_ECC_SetSeed pROM_NCL_ECC_SetSeed;
extern ROM_NCL_ECC_GenKey pROM_NCL_ECC_GenKey;
extern ROM_NCL_ECC_ECDHKeyExchange pROM_NCL_ECC_ECDHKeyExchange;
extern ROM_NCL_ECC_ECDSASignHash pROM_NCL_ECC_ECDSASignHash;
extern ROM_NCL_ECC_ECDSAVerifyHash pROM_NCL_ECC_ECDSAVerifyHash;
extern ROM_NCL_ECC_PointVerify pROM_NCL_ECC_PointVerify;
extern ROM_NCL_ECC_GetCurveData pROM_NCL_ECC_GetCurveData;
extern ROM_NCL_SHA_Power pROM_NCL_SHA_Power;
extern ROM_NCL_DRBG_InitContext pROM_NCL_DRBG_InitContext;
extern ROM_NCL_DRBG_Init pROM_NCL_DRBG_Init;
extern ROM_NCL_DRBG_Instantiate pROM_NCL_DRBG_Instantiate;
extern ROM_NCL_DRBG_FinalizeContext pROM_NCL_DRBG_FinalizeContext;

#define EXTRA_BYTES			   2
#define NCL_MAX_ECC_KEY_UINT32 (ECC_MAX_KEY_LENGTH + EXTRA_BYTES) / 4

#ifdef _ARBEL_Z1_
/* Elliptic curve parameters for ECC256*/
uint8_t p_arr_256[] = {
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff
	};

uint8_t a_arr_256[] = {
	0xfc,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff
	};

uint8_t b_arr_256[] = {
	0x4b,0x60,0xd2,0x27,0x3e,0x3c,0xce,0x3b,0xf6,0xb0,0x53,0xcc,
	0xb0,0x06,0x1d,0x65,0xbc,0x86,0x98,0x76,0x55,0xbd,0xeb,0xb3,0xe7,0x93,0x3a,0xaa,
	0xd8,0x35,0xc6,0x5a
	};

uint8_t n_arr_256[] = {
	0x51,0x25,0x63,0xfc,0xc2,0xca,0xb9,0xf3,0x84,0x9e,0x17,0xa7,
	0xad,0xfa,0xe6,0xbc,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff
	};

uint8_t x_arr_256[] = {
	0x96,0xc2,0x98,0xd8,0x45,0x39,0xa1,0xf4,0xa0,0x33,0xeb,0x2d,
	0x81,0x7d,0x03,0x77,0xf2,0x40,0xa4,0x63,0xe5,0xe6,0xbc,0xf8,0x47,0x42,0x2c,0xe1,
	0xf2,0xd1,0x17,0x6b
	};

uint8_t y_arr_256[] = {
	0xf5,0x51,0xbf,0x37,0x68,0x40,0xb6,0xcb,0xce,0x5e,0x31,0x6b,
	0x57,0x33,0xce,0x2b,0x16,0x9e,0x0f,0x7c,0x4a,0xeb,0xe7,0x8e,0x9b,0x7f,0x1a,0xfe,
	0xe2,0x42,0xe3,0x4f
	};

/* Elliptic curve parameters for ECC384*/
#if ECC_MAX_KEY_LENGTH >= ECC_KEY_LENGTH_384
#define a_arr 0x000127e0
#define b_arr 0x00012810
#define n_arr 0x00012840
#define p_arr 0x00012870
#define x_arr 0x000128a0
#define y_arr 0x000128d0
#endif

/* Elliptic curve parameters for ECC521*/
#if ECC_MAX_KEY_LENGTH >= ECC_KEY_LENGTH_521
uint8_t p_arr_521[] = { 
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x01,0x00,0x00
	};

uint8_t a_arr_521[] = {
	0xfc,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x01,0x00,0x00
	};
	
uint8_t b_arr_521[] = {
	0x00,0x3f,0x50,0x6b,0xd4,0x1f,0x45,0xef,0xf1,0x34,0x2c,0x3d,
	0x88,0xdf,0x73,0x35,0x07,0xbf,0xb1,0x3b,0xbd,0xc0,0x52,0x16,0x7b,0x93,0x7e,0xec,
	0x51,0x39,0x19,0x56,0xe1,0x09,0xf1,0x8e,0x91,0x89,0xb4,0xb8,0xf3,0x15,0xb3,0x99,
	0x5b,0x72,0xda,0xa2,0xee,0x40,0x85,0xb6,0xa0,0x21,0x9a,0x92,0x1f,0x9a,0x1c,0x8e,
	0x61,0xb9,0x3e,0x95,0x51,0x00,0x00,0x00
	};

uint8_t n_arr_521[] = {
	0x09,0x64,0x38,0x91,0x1e,0xb7,0x6f,0xbb,0xae,0x47,0x9c,0x89,
	0xb8,0xc9,0xb5,0x3b,0xd0,0xa5,0x09,0xf7,0x48,0x01,0xcc,0x7f,0x6b,0x96,0x2f,0xbf,
	0x83,0x87,0x86,0x51,0xfa,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x01,0x00,0x00
	};

uint8_t x_arr_521[] = {0x66,0xbd,0xe5,0xc2,0x31,0x7e,0x7e,0xf9,0x9b,0x42,0x6a,0x85,
	0xc1,0xb3,0x48,0x33,0xde,0xa8,0xff,0xa2,0x27,0xc1,0x1d,0xfe,0x28,0x59,0xe7,0xef,
	0x77,0x5e,0x4b,0xa1,0xba,0x3d,0x4d,0x6b,0x60,0xaf,0x28,0xf8,0x21,0xb5,0x3f,0x05,
	0x39,0x81,0x64,0x9c,0x42,0xb4,0x95,0x23,0x66,0xcb,0x3e,0x9e,0xcd,0xe9,0x04,0x04,
	0xb7,0x06,0x8e,0x85,0xc6,0x00,0x00,0x00
	};

uint8_t y_arr_521[] = {0x50,0x66,0xd1,0x9f,0x76,0x94,0xbe,0x88,0x40,0xc2,0x72,0xa2,
	0x86,0x70,0x3c,0x35,0x61,0x07,0xad,0x3f,0x01,0xb9,0x50,0xc5,0x40,0x26,0xf4,0x5e,
	0x99,0x72,0xee,0x97,0x2c,0x66,0x3e,0x27,0x17,0xbd,0xaf,0x17,0x68,0x44,0x9b,0x57,
	0x49,0x44,0xf5,0x98,0xd9,0x1b,0x7d,0x2c,0xb4,0x5f,0x8a,0x5c,0x04,0xc0,0x3b,0x9a,
	0x78,0x6a,0x29,0x39,0x18,0x01,0x00,0x00
	};
#endif
#endif
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
			out[i] = ((uint8_t *) data)[i];
		}
	}
	if (CHIP_Get_Version () == ARBEL_VERSION_Z1)
		return NCL_STATUS_OK_Z1;
	else
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

static void tip_ecc_get_curve_data (NCL_ECC_CURVE_DATA_T **curve_output,
	void *context, NCL_ECC_CURVE_T curve_type, size_t key_length)
{

	if (CHIP_Get_Version () == ARBEL_VERSION_Z1) {
		static NCL_ECC_CURVE_DATA_T curve_data;
		curve_data.h = 0x1;
#ifdef _ARBEL_Z1_
		switch (key_length) {
			case ECC_KEY_LENGTH_256:
				curve_data.p = (NCL_BN_T){ (uint32_t *) p_arr_256,
					sizeof (p_arr_256) / sizeof (UINT32) };
				curve_data.a = (NCL_BN_T){ (uint32_t *) a_arr_256,
					sizeof (a_arr_256) / sizeof (UINT32) };
				curve_data.b = (NCL_BN_T){ (uint32_t *) b_arr_256,
					sizeof (b_arr_256) / sizeof (UINT32) };
				curve_data.x = (NCL_BN_T){ (uint32_t *) x_arr_256,
					sizeof (x_arr_256) / sizeof (UINT32) };
				curve_data.y = (NCL_BN_T){ (uint32_t *) y_arr_256,
					sizeof (y_arr_256) / sizeof (UINT32) };
				curve_data.n = (NCL_BN_T){ (uint32_t *) n_arr_256,
					sizeof (n_arr_256) / sizeof (UINT32) };
				break;
#if ECC_MAX_KEY_LENGTH >= ECC_KEY_LENGTH_384
			case ECC_KEY_LENGTH_384:
				curve_data.p = (NCL_BN_T){ (uint32_t *) p_arr, 0x30 / sizeof (UINT32) };
				curve_data.a = (NCL_BN_T){ (uint32_t *) a_arr, 0x30 / sizeof (UINT32) };
				curve_data.b = (NCL_BN_T){ (uint32_t *) b_arr, 0x30 / sizeof (UINT32) };
				curve_data.x = (NCL_BN_T){ (uint32_t *) x_arr, 0x30 / sizeof (UINT32) };
				curve_data.y = (NCL_BN_T){ (uint32_t *) y_arr, 0x30 / sizeof (UINT32) };
				curve_data.n = (NCL_BN_T){ (uint32_t *) n_arr, 0x30 / sizeof (UINT32) };

				break;
#if ECC_MAX_KEY_LENGTH >= ECC_KEY_LENGTH_521
			case ECC_KEY_LENGTH_521:
				curve_data.p = (NCL_BN_T){ (uint32_t *) p_arr_521,
					sizeof (p_arr_521) / sizeof (UINT32) };
				curve_data.a = (NCL_BN_T){ (uint32_t *) a_arr_521,
					sizeof (a_arr_521) / sizeof (UINT32) };
				curve_data.b = (NCL_BN_T){ (uint32_t *) b_arr_521,
					sizeof (b_arr_521) / sizeof (UINT32) };
				curve_data.x = (NCL_BN_T){ (uint32_t *) x_arr_521,
					sizeof (x_arr_521) / sizeof (UINT32) };
				curve_data.y = (NCL_BN_T){ (uint32_t *) y_arr_521,
					sizeof (y_arr_521) / sizeof (UINT32) };
				curve_data.n = (NCL_BN_T){ (uint32_t *) n_arr_521,
					sizeof (n_arr_521) / sizeof (UINT32) };
				break;
#endif
#endif

			}
#endif
			*curve_output = &curve_data;
	} else {
		 *curve_output = pROM_NCL_ECC_GetCurveData (context, curve_type);		
	}
}


static int tip_ecc_hw_ncl_engine_get_ecc_public_key (const struct ecc_hw *ecc_hw,
	const uint8_t *priv_key, size_t key_length, struct ecc_point_public_key *pub_key)
{
	struct tip_ecc_hw_ncl_engine *tip_ncl = (struct tip_ecc_hw_ncl_engine *) ecc_hw;
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

	/*copy the input params into NCL params, with option of padding for ecc521 case*/
	memcpy (priv_key_copy + start_address, priv_key, key_length);
	prv_key_ncl.size = keySize;
	prv_key_ncl.number = (uint32_t *) prv_key_arr;
	pub_key_ncl.X.number = (uint32_t *) pub_key_x_arr;
	pub_key_ncl.X.size = keySize;
	pub_key_ncl.Y.number = (uint32_t *) pub_key_y_arr;
	pub_key_ncl.Y.size = keySize;

	/* ECC Gen Key creates private and public in little endian, therefore need to swap it before
	 * creation*/
	tip_mem_swap_endianness (priv_key_copy, NULL, key_length_extended);
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_InitContext (tip_ncl->context));
	
	tip_ecc_get_curve_data(&curve, tip_ncl->context, curve_type, key_length);
	
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_SetCurve (tip_ncl->context, curve));
	
	ncl_status = pROM_NCL_ECC_GenKey (tip_ncl->context, &prv_key_ncl, &pub_key_ncl, curve,
		tip_kdf_mem_cpy, (void *) priv_key_copy);

	/* ECC Gen Key creates private and public in little endian, therefore need to swap it after
	 * creation*/
	if (ncl_status == NCL_STATUS_OK) {
		tip_mem_swap_endianness (pub_key_ncl.X.number, NULL, key_length_extended);
		tip_mem_swap_endianness (pub_key_ncl.Y.number, NULL, key_length_extended);

		pub_key->key_length = key_length;
		/* copy the NCL output public into output param, remove the padding in ecc521 case*/
		memcpy (pub_key->x, (uint8_t *) pub_key_ncl.X.number + start_address,
			pub_key->key_length);
		memcpy (pub_key->y, (uint8_t *) pub_key_ncl.Y.number + start_address,
			pub_key->key_length);
	}
	return tip_ncl_status (ncl_status, ECC_HW_ECC_PUBLIC_FAILED);
}

static int tip_ecc_hw_ncl_engine_generate_ecc_key_pair (const struct ecc_hw *ecc_hw,
	size_t key_length, uint8_t *priv_key, struct ecc_point_public_key *pub_key)
{
	struct tip_ecc_hw_ncl_engine *tip_ncl = (struct tip_ecc_hw_ncl_engine *) ecc_hw;
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
	/* get all sizes according to key_length*/
	tip_get_key_sizes (key_length, &keySize, &curve_type, &start_address, &key_length_extended);

	prv_key_ncl.size = keySize;
	prv_key_ncl.number = (uint32_t *) prv_key_arr;
	pub_key_ncl.X.number = (uint32_t *) pub_key_x_arr;
	pub_key_ncl.X.size = keySize;
	pub_key_ncl.Y.number = (uint32_t *) pub_key_y_arr;
	pub_key_ncl.Y.size = keySize;
	
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_InitContext (tip_ncl->context));
	tip_ecc_get_curve_data (&curve, tip_ncl->context, curve_type, key_length);
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_SetCurve (tip_ncl->context, curve));
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_SetSeed (tip_ncl->context, 0xffffffff));
	ncl_status = pROM_NCL_ECC_GenKey (tip_ncl->context, &prv_key_ncl, &pub_key_ncl, curve, NULL,
			NULL);

	if (ncl_status == NCL_STATUS_OK) {
		pub_key->key_length = key_length;
		/* ECC Gen Key creates private and public in little endian, therefore need to swap it after
		 * creation*/
		tip_mem_swap_endianness (pub_key_ncl.X.number, NULL, key_length_extended);
		tip_mem_swap_endianness (pub_key_ncl.Y.number, NULL, key_length_extended);
		tip_mem_swap_endianness (prv_key_ncl.number, NULL, key_length_extended);

		/* copy the NCL output key into output params, remove the padding in ecc521 case*/
		memcpy (pub_key->x, (uint8_t *) pub_key_ncl.X.number + start_address, key_length);
		memcpy (pub_key->y, (uint8_t *) pub_key_ncl.Y.number + start_address, key_length);
		memcpy (priv_key, (uint8_t *) prv_key_ncl.number + start_address, key_length);
	}

	return tip_ncl_status (ncl_status, ECC_HW_ECC_GENERATE_FAILED);
}

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

	/* get all sizes according to key_length and digest_length*/
	tip_get_key_sizes (key_length, &key_size, &curve_type, &start_address, &key_length_extended);
	tip_get_hash_sizes (digest_length, &hash_alg);

	signature->length = key_length;
	prv_key_ncl.size = key_size;
	prv_key_ncl.number = (uint32_t *) prv_key_arr;
	rSig.number = (uint32_t *) r_sig_arr;
	rSig.size = key_size;
	sSig.number = (uint32_t *) s_sig_arr;
	sSig.size = key_size;

	/* copy the input params into NCL params, with option of padding for ecc521 case*/
	memcpy ((uint8_t *) prv_key_ncl.number + start_address, priv_key, key_length);
	/* need to swap private key input endiandess before signing*/
	tip_mem_swap_endianness ((uint8_t *) prv_key_ncl.number, NULL, key_length_extended);
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_InitContext (tip_ncl->context));
	
	tip_ecc_get_curve_data (&curve, tip_ncl->context, curve_type, key_length);
	ncl_status = pROM_NCL_ECC_SetCurve (tip_ncl->context, curve);
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_SetSeed (tip_ncl->context, 0xffffffff));
	ncl_status = pROM_NCL_ECC_ECDSASignHash (tip_ncl->context, &rSig, &sSig, hash_alg,
		&prv_key_ncl, digest, curve, NULL, NULL);
	
	/* need to swap signature endiandess before returning it*/
	if (ncl_status == NCL_STATUS_OK) {
		tip_mem_swap_endianness ((uint8_t *) rSig.number, NULL, key_length_extended);
		tip_mem_swap_endianness ((uint8_t *) sSig.number, NULL, key_length_extended);

		/* copy the NCL output signature into output param, remove the padding in ecc521 case*/
		memcpy (signature->r, (uint8_t *) rSig.number + start_address, signature->length);
		memcpy (signature->s, (uint8_t *) sSig.number + start_address, signature->length);
	}

	return tip_ncl_status (NCL_STATUS_OK, ECC_HW_ECDSA_SIGN_FAILED);
}

static int tip_ecc_hw_ncl_engine_ecdsa_verify (const struct ecc_hw *ecc_hw,
	const struct ecc_point_public_key *pub_key, const struct ecc_ecdsa_signature *signature,
	const uint8_t *digest, size_t digest_length)
{
	struct tip_ecc_hw_ncl_engine *tip_ncl = (struct tip_ecc_hw_ncl_engine *) ecc_hw;
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
	/* get all sizes according to key_length and digest_length*/
	tip_get_key_sizes (key_length, &key_size, &curve_type, &start_address, &key_length_extended);
	tip_get_hash_sizes (digest_length, &hash_alg);

	/* now that key_size is known, can define NCL params*/
	rSig.number = (uint32_t *) r_sig_arr;
	rSig.size = key_size;
	sSig.number = (uint32_t *) s_sig_arr;
	sSig.size = key_size;

	pub_key_ncl.X.number = (uint32_t *) pub_key_x_arr;
	pub_key_ncl.X.size = key_size;
	pub_key_ncl.Y.number = (uint32_t *) pub_key_y_arr;
	pub_key_ncl.Y.size = key_size;

	/* copy the input params into NCL params, with option of padding for ecc521 case*/
	memcpy ((uint8_t *) rSig.number + start_address, (signature->r), key_length);
	memcpy ((uint8_t *) sSig.number + start_address, (signature->s), key_length);
	memcpy ((uint8_t *) pub_key_ncl.X.number + start_address, (pub_key->x), key_length);
	memcpy ((uint8_t *) pub_key_ncl.Y.number + start_address, (pub_key->y), key_length);
	
	/* need to swap signature and key endiandess before using them to verify*/
	tip_mem_swap_endianness ((uint8_t *) pub_key_ncl.X.number, NULL, key_length_extended);
	tip_mem_swap_endianness ((uint8_t *) pub_key_ncl.Y.number, NULL, key_length_extended);
	tip_mem_swap_endianness ((uint8_t *) rSig.number, NULL, key_length_extended);
	tip_mem_swap_endianness ((uint8_t *) sSig.number, NULL, key_length_extended);
	
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_InitContext (tip_ncl->context));
	
	tip_ecc_get_curve_data (&curve, tip_ncl->context, curve_type, key_length);
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_SetCurve (tip_ncl->context, curve));	
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_SetSeed (tip_ncl->context, 0xffffffff));	
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_ECDSAVerifyHash (tip_ncl->context, &sigOk, &rSig, &sSig,
	hash_alg, digest, &pub_key_ncl, curve));
			
	return tip_ncl_status (sigOk, ECC_HW_ECDSA_BAD_SIGNATURE);
}

static int tip_ecc_hw_ncl_engine_ecdh_compute (const struct ecc_hw *ecc_hw,
	const uint8_t *priv_key, size_t key_length, const struct ecc_point_public_key *pub_key,
	uint8_t *secret, size_t length)
{
	struct tip_ecc_hw_ncl_engine *tip_ncl = (struct tip_ecc_hw_ncl_engine *) ecc_hw;
	uint32_t prv_key_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_x_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t pub_key_y_arr[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t keySize = ECC_KEY_SIZE_DWORD_256;
	NCL_ECC_CURVE_T curve_type = NCL_ECC_CURVE_NIST_P_256;
	int key_length_extended = key_length;
	int start_address = 0;
	uint32_t secret_copy[NCL_MAX_ECC_KEY_UINT32] = { 0 };
	uint32_t r_s_length;
	NCL_BN_T prv_key_ncl;
	NCL_ECC_POINT_T pub_key_ncl, shared_sec;

	NCL_ECC_CURVE_DATA_T *curve = NULL;
	NCL_STATUS_T ncl_status;
	if ((tip_ncl == NULL) || (priv_key == NULL) || (pub_key == NULL) || (secret == NULL)) {
		return ECC_HW_INVALID_ARGUMENT;
	}

	/*get all sizes according to key_length*/
	tip_get_key_sizes (key_length, &keySize, &curve_type, &start_address, &key_length_extended);

	prv_key_ncl.size = keySize;
	prv_key_ncl.number = (uint32_t *) prv_key_arr;
	pub_key_ncl.X.number = (uint32_t *) pub_key_x_arr;
	pub_key_ncl.X.size = keySize;
	pub_key_ncl.Y.number = (uint32_t *) pub_key_y_arr;
	pub_key_ncl.Y.size = keySize;

	/* copy the input params into NCL params, with option of padding for ecc521 case*/
	memcpy ((uint8_t *) pub_key_ncl.X.number + start_address, (pub_key->x), key_length);
	memcpy ((uint8_t *) pub_key_ncl.Y.number + start_address, (pub_key->y), key_length);
	memcpy ((uint8_t *) prv_key_ncl.number + start_address, priv_key, key_length);

	/* r or s length is the size of the curve_local (mult by 4 because uint32)*/
	r_s_length = keySize * 4;

	shared_sec.X.number = (uint32_t *) secret_copy;
	shared_sec.X.size = keySize;

	shared_sec.Y.number = (uint32_t *) secret_copy + (keySize);
	shared_sec.Y.size = keySize;

	/* need to swap key endiandess before using them to perform key exchange*/
	tip_mem_swap_endianness ((uint8_t *) pub_key_ncl.X.number, NULL, keySize * 4);
	tip_mem_swap_endianness ((uint8_t *) pub_key_ncl.Y.number, NULL, keySize * 4);
	tip_mem_swap_endianness ((uint8_t *) prv_key_ncl.number, NULL, keySize * 4);
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_InitContext (tip_ncl->context));

	tip_ecc_get_curve_data (&curve, tip_ncl->context, curve_type, key_length);
	ncl_status = pROM_NCL_ECC_SetCurve (tip_ncl->context, curve);
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_SetSeed (tip_ncl->context, 0xffffffff));
	ncl_status = pROM_NCL_ECC_ECDHKeyExchange (tip_ncl->context, &shared_sec, &prv_key_ncl,
		&pub_key_ncl, curve);
	
	if (ncl_status == NCL_STATUS_OK) {
		/* need to swap shared secret endiandess before returning it*/
		tip_mem_swap_endianness ((uint8_t *) shared_sec.X.number, NULL, r_s_length);
		tip_mem_swap_endianness ((uint8_t *) shared_sec.Y.number, NULL, r_s_length);
		/* copy the NCL output secret into output param, remove the padding in ecc521 case*/
		memcpy (secret, (uint8_t *) shared_sec.X.number + start_address, key_length);
		memcpy (secret + r_s_length, (uint8_t *) shared_sec.Y.number + start_address, key_length);
	}
	return tip_ncl_status (ncl_status, ECC_HW_ECDH_COMPUTE_FAILED);
}

static int tip_ecc_hw_ncl_engine_is_free (const struct ecc_hw *ecc_hw)
{
	struct tip_ecc_hw_ncl_engine *engine = (struct tip_ecc_hw_ncl_engine *) ecc_hw;

	if (engine == NULL) {
		return ECC_ENGINE_INVALID_ARGUMENT;
	}
	return 0;
}

/**
 * Initialize an instance for running ECC operations using tipncl.
 *
 * @param engine The ECC engine to initialize.
 *
 * @return 0 if the engine was successfully initialized or an error code.
 */
int tip_ecc_hw_ncl_init (struct tip_ecc_hw_ncl_engine *engine)
{
	if (engine == NULL) {
		return ECC_HW_INVALID_ARGUMENT;
	}
	memset (engine, 0, sizeof (struct tip_ecc_hw_ncl_engine));
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_Power (engine->context, TRUE));
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_InitContext (engine->context));
	NCL_STATUS_RET_CHECK (pROM_NCL_ECC_Init (engine->context));

	engine->base.get_ecc_public_key = tip_ecc_hw_ncl_engine_get_ecc_public_key;
	engine->base.generate_ecc_key_pair = tip_ecc_hw_ncl_engine_generate_ecc_key_pair;
	engine->base.ecdsa_sign = tip_ecc_hw_ncl_engine_ecdsa_sign;
	engine->base.ecdsa_verify = tip_ecc_hw_ncl_engine_ecdsa_verify;
	engine->base.ecdh_compute = tip_ecc_hw_ncl_engine_ecdh_compute;
	engine->base.is_free = tip_ecc_hw_ncl_engine_is_free;

	return 0;
}

/**
 * Release an tipncl ECC engine.
 *
 * @param engine The ECC engine to release.
 */
void tip_ecc_hw_ncl_release (struct tip_ecc_hw_ncl_engine *engine)
{
	if (engine) {
		pROM_NCL_ECC_FinalizeContext (engine->context);
	}
}
