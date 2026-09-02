
//-----------------------------------------------------------
// SPDX-License-Identifier: GPL-2.0                          
//                                                           
// Copyright (c) 2020 by Nuvoton Technology Corporation      
// All rights reserved                                       
//-----------------------------------------------------------

    .syntax unified
    .cpu cortex-m4
    .thumb

    .global NCL_AES_ClearParams
    .global romfunc_NCL_AES_ClearParams

romfunc_NCL_AES_ClearParams:
    b.w NCL_AES_ClearParams

    .global NCL_AES_Decrypt
    .global romfunc_NCL_AES_Decrypt

romfunc_NCL_AES_Decrypt:
    b.w NCL_AES_Decrypt

    .global NCL_AES_GcmEncrypt
    .global romfunc_NCL_AES_GcmEncrypt

romfunc_NCL_AES_GcmEncrypt:
    b.w NCL_AES_GcmEncrypt

    .global NCL_AES_Init
    .global romfunc_NCL_AES_Init

romfunc_NCL_AES_Init:
    b.w NCL_AES_Init

    .global NCL_AES_Power
    .global romfunc_NCL_AES_Power

romfunc_NCL_AES_Power:
    b.w NCL_AES_Power

    .global NCL_AES_SelectKey
    .global romfunc_NCL_AES_SelectKey

romfunc_NCL_AES_SelectKey:
    b.w NCL_AES_SelectKey

    .global NCL_AES_SelfTest
    .global romfunc_NCL_AES_SelfTest

romfunc_NCL_AES_SelfTest:
    b.w NCL_AES_SelfTest

    .global NCL_AES_SetGcmParams
    .global romfunc_NCL_AES_SetGcmParams

romfunc_NCL_AES_SetGcmParams:
    b.w NCL_AES_SetGcmParams

    .global NCL_AES_SetKey
    .global romfunc_NCL_AES_SetKey

romfunc_NCL_AES_SetKey:
    b.w NCL_AES_SetKey

    .global NCL_AES_SetParams
    .global romfunc_NCL_AES_SetParams

romfunc_NCL_AES_SetParams:
    b.w NCL_AES_SetParams

    .global NCL_DRBG_Config
    .global romfunc_NCL_DRBG_Config

romfunc_NCL_DRBG_Config:
    b.w NCL_DRBG_Config

    .global NCL_DRBG_FinalizeContext
    .global romfunc_NCL_DRBG_FinalizeContext

romfunc_NCL_DRBG_FinalizeContext:
    b.w NCL_DRBG_FinalizeContext

    .global NCL_DRBG_Generate
    .global romfunc_NCL_DRBG_Generate

romfunc_NCL_DRBG_Generate:
    b.w NCL_DRBG_Generate

    .global NCL_DRBG_Init
    .global romfunc_NCL_DRBG_Init

romfunc_NCL_DRBG_Init:
    b.w NCL_DRBG_Init

    .global NCL_DRBG_InitContext
    .global romfunc_NCL_DRBG_InitContext

romfunc_NCL_DRBG_InitContext:
    b.w NCL_DRBG_InitContext

    .global NCL_DRBG_Instantiate
    .global romfunc_NCL_DRBG_Instantiate

romfunc_NCL_DRBG_Instantiate:
    b.w NCL_DRBG_Instantiate

    .global NCL_DRBG_Power
    .global romfunc_NCL_DRBG_Power

romfunc_NCL_DRBG_Power:
    b.w NCL_DRBG_Power

    .global NCL_DRBG_Reseed
    .global romfunc_NCL_DRBG_Reseed

romfunc_NCL_DRBG_Reseed:
    b.w NCL_DRBG_Reseed

    .global NCL_DRBG_SelfTest
    .global romfunc_NCL_DRBG_SelfTest

romfunc_NCL_DRBG_SelfTest:
    b.w NCL_DRBG_SelfTest

    .global NCL_DRBG_UnInstantiate
    .global romfunc_NCL_DRBG_UnInstantiate

romfunc_NCL_DRBG_UnInstantiate:
    b.w NCL_DRBG_UnInstantiate

    .global NCL_ECC_ECDHKeyExchange
    .global romfunc_NCL_ECC_ECDHKeyExchange

romfunc_NCL_ECC_ECDHKeyExchange:
    b.w NCL_ECC_ECDHKeyExchange

    .global NCL_ECC_ECDSASignData
    .global romfunc_NCL_ECC_ECDSASignData

romfunc_NCL_ECC_ECDSASignData:
    b.w NCL_ECC_ECDSASignData

    .global NCL_ECC_ECDSASignHash
    .global romfunc_NCL_ECC_ECDSASignHash

romfunc_NCL_ECC_ECDSASignHash:
    b.w NCL_ECC_ECDSASignHash

    .global NCL_ECC_ECDSAVerifyData
    .global romfunc_NCL_ECC_ECDSAVerifyData

romfunc_NCL_ECC_ECDSAVerifyData:
    b.w NCL_ECC_ECDSAVerifyData

    .global NCL_ECC_ECDSAVerifyHash
    .global romfunc_NCL_ECC_ECDSAVerifyHash

romfunc_NCL_ECC_ECDSAVerifyHash:
    b.w NCL_ECC_ECDSAVerifyHash

    .global NCL_ECC_FinalizeContext
    .global romfunc_NCL_ECC_FinalizeContext

romfunc_NCL_ECC_FinalizeContext:
    b.w NCL_ECC_FinalizeContext

    .global NCL_ECC_GenKey
    .global romfunc_NCL_ECC_GenKey

romfunc_NCL_ECC_GenKey:
    b.w NCL_ECC_GenKey

    .global NCL_ECC_Init
    .global romfunc_NCL_ECC_Init

romfunc_NCL_ECC_Init:
    b.w NCL_ECC_Init

    .global NCL_ECC_InitContext
    .global romfunc_NCL_ECC_InitContext

romfunc_NCL_ECC_InitContext:
    b.w NCL_ECC_InitContext

    .global NCL_ECC_Power
    .global romfunc_NCL_ECC_Power

romfunc_NCL_ECC_Power:
    b.w NCL_ECC_Power

    .global NCL_ECC_SelfTest
    .global romfunc_NCL_ECC_SelfTest

romfunc_NCL_ECC_SelfTest:
    b.w NCL_ECC_SelfTest

    .global NCL_ECC_SetCurve
    .global romfunc_NCL_ECC_SetCurve

romfunc_NCL_ECC_SetCurve:
    b.w NCL_ECC_SetCurve

    .global NCL_MISC_GetStatus
    .global romfunc_NCL_MISC_GetStatus

romfunc_NCL_MISC_GetStatus:
    b.w NCL_MISC_GetStatus

    .global NCL_MISC_Init
    .global romfunc_NCL_MISC_Init

romfunc_NCL_MISC_Init:
    b.w NCL_MISC_Init

    .global NCL_MISC_SelfTest
    .global romfunc_NCL_MISC_SelfTest

romfunc_NCL_MISC_SelfTest:
    b.w NCL_MISC_SelfTest

    .global NCL_RSA_Decrypt
    .global romfunc_NCL_RSA_Decrypt

romfunc_NCL_RSA_Decrypt:
    b.w NCL_RSA_Decrypt

    .global NCL_RSA_Encrypt
    .global romfunc_NCL_RSA_Encrypt

romfunc_NCL_RSA_Encrypt:
    b.w NCL_RSA_Encrypt

    .global NCL_RSA_GetStatus
    .global romfunc_NCL_RSA_GetStatus

romfunc_NCL_RSA_GetStatus:
    b.w NCL_RSA_GetStatus

    .global NCL_RSA_Init
    .global romfunc_NCL_RSA_Init

romfunc_NCL_RSA_Init:
    b.w NCL_RSA_Init

    .global NCL_RSA_SelfTest
    .global romfunc_NCL_RSA_SelfTest

romfunc_NCL_RSA_SelfTest:
    b.w NCL_RSA_SelfTest

    .global NCL_RSA_SetKey
    .global romfunc_NCL_RSA_SetKey

romfunc_NCL_RSA_SetKey:
    b.w NCL_RSA_SetKey

    .global NCL_RSA_SignHash
    .global romfunc_NCL_RSA_SignHash

romfunc_NCL_RSA_SignHash:
    b.w NCL_RSA_SignHash

    .global NCL_RSA_VerifyHash
    .global romfunc_NCL_RSA_VerifyHash

romfunc_NCL_RSA_VerifyHash:
    b.w NCL_RSA_VerifyHash

    .global NCL_SHA_Calc
    .global romfunc_NCL_SHA_Calc

romfunc_NCL_SHA_Calc:
    b.w NCL_SHA_Calc

    .global NCL_SHA_FinalizeContext
    .global romfunc_NCL_SHA_FinalizeContext

romfunc_NCL_SHA_FinalizeContext:
    b.w NCL_SHA_FinalizeContext

    .global NCL_SHA_Finish
    .global romfunc_NCL_SHA_Finish

romfunc_NCL_SHA_Finish:
    b.w NCL_SHA_Finish

    .global NCL_SHA_HASH_SelfTest
    .global romfunc_NCL_SHA_HASH_SelfTest

romfunc_NCL_SHA_HASH_SelfTest:
    b.w NCL_SHA_HASH_SelfTest

    .global NCL_SHA_HKDF
    .global romfunc_NCL_SHA_HKDF

romfunc_NCL_SHA_HKDF:
    b.w NCL_SHA_HKDF

    .global NCL_SHA_HKDF_SelfTest
    .global romfunc_NCL_SHA_HKDF_SelfTest

romfunc_NCL_SHA_HKDF_SelfTest:
    b.w NCL_SHA_HKDF_SelfTest

    .global NCL_SHA_HMAC
    .global romfunc_NCL_SHA_HMAC

romfunc_NCL_SHA_HMAC:
    b.w NCL_SHA_HMAC

    .global NCL_SHA_HMAC_SelfTest
    .global romfunc_NCL_SHA_HMAC_SelfTest

romfunc_NCL_SHA_HMAC_SelfTest:
    b.w NCL_SHA_HMAC_SelfTest

    .global NCL_SHA_Init
    .global romfunc_NCL_SHA_Init

romfunc_NCL_SHA_Init:
    b.w NCL_SHA_Init

    .global NCL_SHA_InitContext
    .global romfunc_NCL_SHA_InitContext

romfunc_NCL_SHA_InitContext:
    b.w NCL_SHA_InitContext

    .global NCL_SHA_Power
    .global romfunc_NCL_SHA_Power

romfunc_NCL_SHA_Power:
    b.w NCL_SHA_Power

    .global NCL_SHA_Reset
    .global romfunc_NCL_SHA_Reset

romfunc_NCL_SHA_Reset:
    b.w NCL_SHA_Reset

    .global NCL_SHA_Start
    .global romfunc_NCL_SHA_Start

romfunc_NCL_SHA_Start:
    b.w NCL_SHA_Start

    .global NCL_SHA_Update
    .global romfunc_NCL_SHA_Update

romfunc_NCL_SHA_Update:
    b.w NCL_SHA_Update


// ------------------------------------------------------------
// End of file                                                 
// ------------------------------------------------------------
