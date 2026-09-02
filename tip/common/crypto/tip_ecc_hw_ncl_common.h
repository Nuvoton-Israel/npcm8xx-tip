/*
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2023 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 *<<<-------------------------------------------------------
 * File Contents:
 *   tip_ecc_hw_ncl.h
 *            This file contains the definition of ecc hw engine for ncl
 *  Project:  Arbel
 */

#ifndef TIP_ECC_HW_NCL_COMMON_H_
#define TIP_ECC_HW_NCL_COMMON_H_

#include "tip_security.h"
#include "crypto/ecc_hw.h"

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
 * A TIP NCL context for ECC operations.
 */
struct tip_ecc_hw_ncl_engine {
	struct ecc_hw base;								/**< The base ECC HW instance. */
	uint8_t context[NCL_ECC_CONTEXT_BYTE_SIZE];		/**< Buffer for the ECC context used by the engine. */
};

/* Helper function used by derived types. */
int tip_ecc_hw_ncl_common_init (struct tip_ecc_hw_ncl_engine *engine);
void tip_ecc_hw_ncl_common_release (struct tip_ecc_hw_ncl_engine *engine);

void tip_get_key_sizes (size_t key_length, uint32_t *keySize, NCL_ECC_CURVE_T *curve_type,
	int *start_address, int *key_length_extended);
void tip_get_hash_sizes (size_t digest_length, NCL_SHA_TYPE_T *hash_alg);
void tip_ecc_get_curve_data (NCL_ECC_CURVE_DATA_T **curve_output, void *context,
	NCL_ECC_CURVE_T curve_type, size_t key_length);


#endif /* TIP_ECC_HW_NCL_COMMON_H_ */
