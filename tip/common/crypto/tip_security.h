/*
 *  Nuvoton Technology Corporation confidential
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *<<<----------------------------------------------------------------------
 * File Contents:
 *   tip_security.h
 *            This file contains API of security routines for ROM code
 *  Project:
 *            Arbel
 */

#ifndef TIP_SECURITY_H
#define TIP_SECURITY_H

#include "defs.h"
#include "tip_boot.h"
#include "hal.h"
#include "tip_ncl_rom_if.h"

#define NCL_RESPONSE_COUNTER 0x00FFFFFF
int 		tip_ncl_status(NCL_STATUS_T status , int error);
/* Security module exported functions */
int  tip_image_hash (HEADER_GENERAL_T* imgHeaderInRAM, uint8_t* imageAddr, uint32_t imageLen, uint8_t* hash, uint8_t hashAlg);
int  tip_image_auth_ECC (HEADER_GENERAL_T* imgHeaderInRAM, uint8_t* imageAddr, uint32_t imageLen, uint8_t* key, uint8_t* hash, SECURED_BOOLEAN_T *sigOK, uint8_t hashAlg);
int  tip_image_auth_LMS (HEADER_GENERAL_T* imgHeaderInRAM, uint8_t* imageAddr, uint32_t imageLen, uint8_t* key, uint8_t* hash, SECURED_BOOLEAN_T *sigOK, uint8_t hashAlg);
int tip_init_rom_ncl (void);
int tip_drbg_config(int config_num);
int tip_rng_optimization(void);


#define NCL_BUSY_WAIT_TIMEOUT(busy_cond, timeout) \
	{                                             \
		uint32_t __time = timeout;                \
                                                  \
		do {                                      \
			if (__time-- == 0) {                  \
				return NCL_STATUS_NO_RESPONSE;    \
			}                                     \
		} while (busy_cond);                      \
	}
#ifdef NCL_DFT_MODE /* DFT is optional, depends on FUSTRAP2.oFIPS. Needed for B1 too. */
typedef void         (*ROM_NCL_DFT_Init )(void);
typedef void         (*ROM_NCL_DFT_MISC_SetStatus )(NCL_ALGO_T algo, NCL_DFT_MISC_STATUS_T status);
typedef void         (*ROM_NCL_DFT_MISC_FailKAT )(NCL_DFT_KAT_T kat);
typedef NCL_STATUS_T (*ROM_NCL_DFT_MISC_GetKatStatus )(NCL_DFT_KAT_T kat);
typedef void         (*ROM_NCL_DFT_ECC_SetKeyChange )(NCL_DFT_ECC_T keyChange);
typedef void         (*ROM_NCL_DFT_DRBG_SetManualReseed )(BOOLEAN manualReseed);
typedef NCL_STATUS_T (*ROM_NCL_DFT_DRBG_GenerateAndTestEntropy) (BOOLEAN  powerOff,
																 uint8_t *entropy,
																 uint32_t size,
																 BOOLEAN  verify);
#endif /* TIP_SECURITY_H */
typedef NCL_ECC_CURVE_DATA_T* (*ROM_NCL_ECC_GetCurveData        ) ( void* context, NCL_ECC_CURVE_T curve);
typedef NCL_STATUS_T (*ROM_NCL_AES_CCM_Decrypt) (void				   *context,
												 const uint32_t		   *cipherData,
												 uint32_t			   *plainData,
												 uint32_t				dataLen,
												 NCL_AES_CCM_MAC_SIZE_T tagSize);
typedef NCL_STATUS_T (*ROM_NCL_AES_CCM_Encrypt) (void				   *context,
												 const uint32_t		   *plainData,
												 uint32_t			   *cipherData,
												 uint32_t				dataLen,
												 NCL_AES_CCM_MAC_SIZE_T tagSize);
typedef NCL_STATUS_T (*ROM_NCL_AES_CCM_SetParams) (void					  *context,
												   const uint32_t		  *iv,
												   const uint32_t		  *key,
												   NCL_AES_KEY_SIZE_T	   keySize,
												   const NCL_AES_ADD_DATA *addData,
												   uint32_t				   dataSize,
												   NCL_AES_OP_T			   op);
typedef NCL_STATUS_T (*ROM_NCL_AES_Clear )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_AES_ClearParams )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_AES_CMAC_Generate) (void			  *context,
												   const uint32_t *message,
												   uint32_t		  *mac,
												   uint32_t		   messageLen,
												   uint32_t		   macLen);
typedef NCL_STATUS_T (*ROM_NCL_AES_CMAC_Verify) (void			*context,
												 NCL_STATUS_T	*isVerified,
												 const uint32_t *message,
												 const uint32_t *mac,
												 uint32_t		 messageLen,
												 uint32_t		 macLen);
typedef NCL_STATUS_T (*ROM_NCL_AES_Decrypt) (void			*context,
											 const uint32_t *cipherData,
											 uint32_t		*plainData,
											 uint32_t		 dataLen);
typedef NCL_STATUS_T (*ROM_NCL_AES_Encrypt) (void			*context,
											 const uint32_t *plainData,
											 uint32_t		*cipherData,
											 uint32_t		 dataLen);
typedef NCL_STATUS_T (*ROM_NCL_AES_ExtendedSelfTest_l )(void);
typedef NCL_STATUS_T (*ROM_NCL_AES_FinalizeContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_AES_GCM_Decrypt) (void			*context,
												 const uint32_t *cipherData,
												 uint32_t		*plainData,
												 uint32_t		 dataLen,
												 const uint32_t *tag);
typedef NCL_STATUS_T (*ROM_NCL_AES_GCM_Encrypt) (void			*context,
												 const uint32_t *plainData,
												 uint32_t		*cipherData,
												 uint32_t		 dataLen,
												 uint32_t		*tag);
typedef NCL_STATUS_T (*ROM_NCL_AES_GCM_SetParams) (void					  *context,
												   uint32_t				  *iv,
												   const uint32_t		  *key,
												   NCL_AES_KEY_SIZE_T	   keySize,
												   const NCL_AES_ADD_DATA *addData,
												   NCL_AES_OP_T			   op);
typedef NCL_STATUS_T (*ROM_NCL_AES_Init )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_AES_InitContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_AES_LockKey )(void* context, uint8_t keyIndex);
typedef NCL_STATUS_T (*ROM_NCL_AES_Power )(void* context, uint8_t on);
typedef NCL_STATUS_T (*ROM_NCL_AES_SelectKey )(void* context, uint8_t keyIndex);
typedef NCL_STATUS_T (*ROM_NCL_AES_SetIv               ) ( void* context, const uint32_t* ivCtr);
typedef NCL_STATUS_T (*ROM_NCL_AES_SetKey) (void			  *context,
											const uint32_t	  *key,
											NCL_AES_KEY_SIZE_T keySize);
typedef NCL_STATUS_T (*ROM_NCL_AES_SetParams) (void				 *context,
											   const uint32_t	 *key,
											   NCL_AES_KEY_SIZE_T keySize,
											   NCL_AES_MODE_T	  mode,
											   const uint32_t	 *ivCtr);
typedef NCL_STATUS_T (*ROM_NCL_AES_SetSeed )(void* context, uint32_t seed, BOOLEAN bUseRandGen);
typedef NCL_STATUS_T (*ROM_NCL_AES_Update) (void		   *context,
											const uint32_t *in,
											uint32_t	   *out,
											uint32_t		dataLen);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_Clear )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_ExtendedSelfTest_l )(void);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_FinalizeContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_Generate) (void			 *context,
											   const uint8_t *addData,
											   uint32_t		  addDataLen,
											   uint8_t		 *outBuff,
											   uint32_t		  outBuffLen);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_Init )(void* context, BOOLEAN intEnable);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_InitContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_Instantiate) (
											void *context,
											NCL_DRBG_SECURITY_STRENGTH_T securityStrength,
											const uint8_t *persString,
											uint32_t persStringLen);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_Power )(void* context, uint8_t on);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_Config)(void*       context,    uint32_t    reseedInterval,    uint8_t     predResistance);
typedef NCL_STATUS_T (*ROM_NCL_DRBG_UnInstantiate )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_ECC_Clear )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_ECC_ECDHKeyExchange) (void						*context,
													 const NCL_ECC_POINT_T		*sharedSecret,
													 const NCL_BN_T				*APrvKey,
													 const NCL_ECC_POINT_T		*BPubKey,
													 const NCL_ECC_CURVE_DATA_T *curveData);
typedef NCL_STATUS_T (*ROM_NCL_ECC_ECDSASignData) (void						  *context,
												   NCL_BN_T					  *r,
												   NCL_BN_T					  *s,
												   NCL_SHA_TYPE_T			   shaType,
												   const NCL_BN_T			  *prvKey,
												   const uint8_t			  *msg,
												   uint32_t					   msgLen,
												   const NCL_ECC_CURVE_DATA_T *curveData,
												   NCL_ECC_KdfFuncPtr		   kdfFunc,
												   void						  *kdfData);
typedef NCL_STATUS_T (*ROM_NCL_ECC_ECDSASignHash) (void						  *context,
												   NCL_BN_T					  *r,
												   NCL_BN_T					  *s,
												   NCL_SHA_TYPE_T			   shaType,
												   const NCL_BN_T			  *prvKey,
												   const uint8_t			  *hash,
												   const NCL_ECC_CURVE_DATA_T *curveData,
												   NCL_ECC_KdfFuncPtr		   kdfFunc,
												   void						  *kdfData);
typedef NCL_STATUS_T (*ROM_NCL_ECC_ECDSAVerifyData) (void						*context,
													 NCL_STATUS_T				*sigOk,
													 const NCL_BN_T				*r,
													 const NCL_BN_T				*s,
													 NCL_SHA_TYPE_T				 shaType,
													 const uint8_t				*msg,
													 uint32_t					 msgLen,
													 const NCL_ECC_POINT_T		*pubKey,
													 const NCL_ECC_CURVE_DATA_T *curveData);
typedef NCL_STATUS_T (*ROM_NCL_ECC_ECDSAVerifyHash) (void						*context,
													 NCL_STATUS_T				*sigOk,
													 const NCL_BN_T				*r,
													 const NCL_BN_T				*s,
													 NCL_SHA_TYPE_T				 shaType,
													 const uint8_t				*hash,
													 const NCL_ECC_POINT_T		*pubKey,
													 const NCL_ECC_CURVE_DATA_T *curveData);
typedef NCL_STATUS_T (*ROM_NCL_ECC_ExtendedSelfTest_l)(void);
typedef NCL_STATUS_T (*ROM_NCL_ECC_FinalizeContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_ECC_GenKey) (void					   *context,
											const NCL_BN_T			   *prvKey,
											const NCL_ECC_POINT_T	   *pubKey,
											const NCL_ECC_CURVE_DATA_T *curveData,
											NCL_ECC_KdfFuncPtr			kdfFunc,
											void					   *kdfData);
typedef NCL_STATUS_T (*ROM_NCL_ECC_Init )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_ECC_InitContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_ECC_PointVerify) (void						*context,
												 uint8_t					*isVerified,
												 const NCL_ECC_POINT_T		*pointIn,
												 const NCL_ECC_CURVE_DATA_T *curveData);
typedef NCL_STATUS_T (*ROM_NCL_ECC_Power )(void* context, uint8_t on);
typedef NCL_STATUS_T (*ROM_NCL_ECC_SetCurve )(void* context, const NCL_ECC_CURVE_DATA_T* curveData);
typedef NCL_STATUS_T (*ROM_NCL_ECC_SetSeed )(void* context, uint32_t randSeed);
typedef NCL_STATUS_T (*ROM_NCL_MISC_GetModuleDescription )(NCL_MODULE_DESC_T* description);
typedef NCL_STATUS_T (*ROM_NCL_MISC_GetStatus )(NCL_ALGO_T algo);
typedef NCL_STATUS_T (*ROM_NCL_MISC_Init_A1) (NCL_FIPS_MODE_T fipsMode,
											  NCL_USR_HANDLER userHandler);
typedef NCL_STATUS_T (*ROM_NCL_MISC_Init_Z1 )(NCL_USR_HANDLER userHandler);
typedef NCL_STATUS_T (*ROM_NCL_MISC_SelfTest )(NCL_ALGO_T algo, NCL_USR_HANDLER userHandler);
typedef NCL_STATUS_T (*ROM_NCL_RSA_Clear )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_RSA_ClearKey )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_RSA_Decrypt) (void			  *context,
											 const uint8_t	  *data,
											 NCL_RSA_PADDING_T padding,
											 NCL_SHA_TYPE_T	   shaType,
											 const uint8_t	  *associatedData,
											 uint32_t		   associatedDataLen,
											 uint8_t		  *dataOut,
											 uint32_t		  *dataOutLen);
typedef NCL_STATUS_T (*ROM_NCL_RSA_Encrypt) (void			  *context,
											 const uint8_t	  *data,
											 uint32_t		   dataLen,
											 NCL_RSA_PADDING_T padding,
											 NCL_SHA_TYPE_T	   shaType,
											 const uint8_t	  *associatedData,
											 uint32_t		   associatedDataLen,
											 uint8_t		  *dataOut);
typedef NCL_STATUS_T (*ROM_NCL_RSA_ExtendedEncryptSelfTest_l )(void);
typedef NCL_STATUS_T (*ROM_NCL_RSA_ExtendedSelfTest_l )(void);
typedef NCL_STATUS_T (*ROM_NCL_RSA_ExtendedSignSelfTest_l )(void);
typedef NCL_STATUS_T (*ROM_NCL_RSA_ExtendedVerifySelfTest_l )(void);
typedef NCL_STATUS_T (*ROM_NCL_RSA_FinalizeContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_RSA_GetStatus )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_RSA_Init )(void* context, NCL_USR_HANDLER handler);
typedef NCL_STATUS_T (*ROM_NCL_RSA_InitContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_RSA_Power )(void* context, uint8_t on);
typedef NCL_STATUS_T (*ROM_NCL_RSA_SetKey) (void				  *context,
											const NCL_RSA_PUB_KEY *pubKey,
											const NCL_RSA_PRV_KEY *prvKey,
											NCL_RSA_KEY_SIZE_T	   keySize);
typedef NCL_STATUS_T (*ROM_NCL_RSA_SignData) (void			   *context,
											  const uint8_t	   *data,
											  uint32_t			dataLen,
											  NCL_RSA_PADDING_T padding,
											  NCL_SHA_TYPE_T	shaType,
											  uint8_t		   *sig);
typedef NCL_STATUS_T (*ROM_NCL_RSA_SignHash) (void			   *context,
											  const uint8_t	   *hash,
											  NCL_RSA_PADDING_T padding,
											  NCL_SHA_TYPE_T	shaType,
											  uint8_t		   *sig);
typedef NCL_STATUS_T (*ROM_NCL_RSA_VerifyData) (void			 *context,
												NCL_STATUS_T	 *sigOk,
												const uint8_t	 *data,
												uint32_t		  dataLen,
												NCL_RSA_PADDING_T padding,
												NCL_SHA_TYPE_T	  shaType,
												const uint8_t	 *sig);
typedef NCL_STATUS_T (*ROM_NCL_RSA_VerifyHash) (void			 *context,
												NCL_STATUS_T	 *sigOk,
												const uint8_t	 *hash,
												NCL_RSA_PADDING_T padding,
												NCL_SHA_TYPE_T	  shaType,
												const uint8_t	 *sig);
typedef NCL_STATUS_T (*ROM_NCL_SHA_Calc_A1) (void			*context,
										  NCL_SHA_TYPE_T shaType,
										  const uint8_t *data,
										  uint32_t		 dataLen,
										  uint8_t		*hashDigest);
typedef NCL_STATUS_T (*ROM_NCL_SHA_Calc_A3) (void			*context,
											NCL_SHA_MODULE_T    moduleNum,
										  NCL_SHA_TYPE_T shaType,
										  const uint8_t *data,
										  uint32_t		 dataLen,
										  uint8_t		*hashDigest);
typedef NCL_STATUS_T (*ROM_SEC_SHA_CALC) (void			*context,
										  NCL_SHA_TYPE_T shaType,
										  const uint8_t *data,
										  uint32_t		 dataLen,
										  uint8_t		*hashDigest);

typedef NCL_STATUS_T (*ROM_NCL_SHA_Clear )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_SHA_ExtendedHmacSelfTest_l )(void);
typedef NCL_STATUS_T (*ROM_NCL_SHA_ExtendedSelfTest_l )(void);
typedef NCL_STATUS_T (*ROM_NCL_SHA_FinalizeContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_SHA_Finish )(void* context, uint8_t* hashDigest);
typedef NCL_STATUS_T (*ROM_NCL_SHA_HKDF_Calc) (void								 *context,
											   _CONST_TYPE_ NCL_SHA_HKDF_INPUT_T *hkdfInput,
											   uint8_t							 *result,
											   uint32_t							  resultSize);
typedef NCL_STATUS_T (*ROM_NCL_SHA_HMAC_Calc) (void					  *context,
											   NCL_SHA_TYPE_T		   shaType,
											   const NCL_SHA_HMAC_KEY *key,
											   const uint8_t		  *data,
											   uint32_t				   dataLen,
											   uint8_t				  *hmac);
typedef NCL_STATUS_T (*ROM_NCL_SHA_HMAC_Finish         ) ( void*  context, uint8_t* padKey, uint8_t* hmac);
typedef NCL_STATUS_T (*ROM_NCL_SHA_HMAC_Start) (void				   *context,
												NCL_SHA_TYPE_T			shaType,
												const NCL_SHA_HMAC_KEY *key,
												uint8_t				   *padKey);
typedef NCL_STATUS_T (*ROM_NCL_SHA_HMAC_Update) (void		   *context,
												 const uint8_t *data,
												 uint32_t		dataLen);
typedef NCL_STATUS_T (*ROM_NCL_SHA_Init )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_SHA_InitContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_SHA_Power )(void* context, uint8_t on);
typedef NCL_STATUS_T (*ROM_NCL_SHA_Reset )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_SHA_Start )(void* context, NCL_SHA_TYPE_T shaType);
typedef NCL_STATUS_T (*ROM_NCL_SHA_Update )(void* context, const uint8_t* data, uint32_t dataLen);
typedef NCL_STATUS_T (*ROM_NCL_LMS_VerifyData) (
    _UNUSED_ void*  context,
    const uint8_t*  publicKey,
    NCL_STATUS_T*   sigOk,
    const uint8_t*  msg,
    uint32_t        msgLen,
    const uint8_t*  signature,
    uint32_t        signatureLen
);
#ifdef LMS_ENABLE
typedef NCL_STATUS_T (*ROM_NCL_LMS_Init )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_LMS_InitContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_LMS_FinalizeContext )(void* context);
typedef NCL_STATUS_T (*ROM_NCL_LMS_Clear )(void* context);
#endif
typedef uint32_t     (*ROM_NCL_AES_GetContextSize )(void);
typedef uint32_t     (*ROM_NCL_DRBG_GetContextSize )(void);
typedef uint32_t     (*ROM_NCL_ECC_GetContextSize )(void);
typedef uint32_t     (*ROM_NCL_RSA_GetContextSize )(void);
typedef uint32_t     (*ROM_NCL_SHA_GetContextSize )(void);
typedef void         (*ROM_NCL_MISC_TamperDetected )(void);
typedef void         (*ROM_PKA_IntHandler )(void);
typedef void         (*ROM_RNG_IntHandler )(void);

/*
 * RNG registers which require initialization in A1.
 */
#define RNGCS(n)        (RNG_BASE_ADDR(n) + 0x00), RNG_ACCESS, 8
#define RNGMODE(n)      (RNG_BASE_ADDR(n) + 0x08), RNG_ACCESS, 8
#define RNGROEN(n, x)   (RNG_BASE_ADDR(n) + 0x0C + 4 * (x)), RNG_ACCESS, 8

#endif /* TIP_SECURITY_H */
