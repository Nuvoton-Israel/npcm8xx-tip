/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *   tip_security.c
 *            This file contains the tip_ncl IF and helper functions
 *  Project:  Arbel
 */

#include "platform_api.h"
#include "platform_io.h"
#include "platform_config.h"
#include "status/rot_status.h"
#include "crypto/rng.h"
#include "crypto/ecc.h"
#include "firmware/firmware_image.h"
#include "crypto/rng.h"
#include "hal.h"
#include "hal_regs.h"
#include "tip_security.h"
#include "tip_utils.h"


/* Elliptic curve parameters */
const uint8_t pArr[] = { 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
					   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
					   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

const uint8_t aArr[] = { 0xfc, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
					   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
					   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

const uint8_t bArr[] = { 0xef, 0x2a, 0xec, 0xd3, 0xed, 0xc8, 0x85, 0x2a, 0x9d, 0xd1, 0x2e, 0x8a,
					   0x8d, 0x39, 0x56, 0xc6, 0x5a, 0x87, 0x13, 0x50, 0x8f, 0x08, 0x14, 0x03,
					   0x12, 0x41, 0x81, 0xfe, 0x6e, 0x9c, 0x1d, 0x18, 0x19, 0x2d, 0xf8, 0xe3,
					   0x6b, 0x05, 0x8e, 0x98, 0xe4, 0xe7, 0x3e, 0xe2, 0xa7, 0x2f, 0x31, 0xb3 };

const uint8_t nArr[] = { 0x73, 0x29, 0xc5, 0xcc, 0x6a, 0x19, 0xec, 0xec, 0x7a, 0xa7, 0xb0, 0x48,
					   0xb2, 0x0d, 0x1a, 0x58, 0xdf, 0x2d, 0x37, 0xf4, 0x81, 0x4d, 0x63, 0xc7,
					   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
					   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

const uint8_t xArr[] = { 0xb7, 0x0a, 0x76, 0x72, 0x38, 0x5e, 0x54, 0x3a, 0x6c, 0x29, 0x55, 0xbf,
					   0x5d, 0xf2, 0x02, 0x55, 0x38, 0x2a, 0x54, 0x82, 0xe0, 0x41, 0xf7, 0x59,
					   0x98, 0x9b, 0xa7, 0x8b, 0x62, 0x3b, 0x1d, 0x6e, 0x74, 0xad, 0x20, 0xf3,
					   0x1e, 0xc7, 0xb1, 0x8e, 0x37, 0x05, 0x8b, 0xbe, 0x22, 0xca, 0x87, 0xaa };

const uint8_t yArr[] = { 0x5f, 0x0e, 0xea, 0x90, 0x7c, 0x1d, 0x43, 0x7a, 0x9d, 0x81, 0x7e, 0x1d,
					   0xce, 0xb1, 0x60, 0x0a, 0xc0, 0xb8, 0xf0, 0xb5, 0x13, 0x31, 0xda, 0xe9,
					   0x7c, 0x14, 0x9a, 0x28, 0xbd, 0x1d, 0xf4, 0xf8, 0x29, 0xdc, 0x92, 0x92,
					   0xbf, 0x98, 0x9e, 0x5d, 0x6f, 0x2c, 0x26, 0x96, 0x4a, 0xde, 0x17, 0x36 };


/**
 *  Address of NCL on TIP ROM functions for A1
 */
/* for Cortex-M4 need to add +1 to address in a branch command (BX) */
#define THUMB_BIT 1

/**
 *  Address of NCL on TIP ROM functions for A1
 */
#define TIP_ROM_A1_ADDR_NCL_AES_CCM_Decrypt            ((ROM_NCL_AES_CCM_Decrypt)(0x00010000 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_CCM_Encrypt            ((ROM_NCL_AES_CCM_Encrypt)(0x00010004 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_CCM_SetParams          ((ROM_NCL_AES_CCM_SetParams)(0x00010008 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_Clear                  ((ROM_NCL_AES_Clear)(0x0001000c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_ClearParams            ((ROM_NCL_AES_ClearParams)(0x00010010 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_CMAC_Generate          ((ROM_NCL_AES_CMAC_Generate)(0x00010014 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_CMAC_Verify            ((ROM_NCL_AES_CMAC_Verify)(0x00010018 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_Decrypt                ((ROM_NCL_AES_Decrypt)(0x0001001c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_Encrypt                ((ROM_NCL_AES_Encrypt)(0x00010020 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_FinalizeContext        ((ROM_NCL_AES_FinalizeContext)(0x00010024 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_GCM_Decrypt            ((ROM_NCL_AES_GCM_Decrypt)(0x00010028 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_GCM_Encrypt            ((ROM_NCL_AES_GCM_Encrypt)(0x0001002c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_GCM_SetParams          ((ROM_NCL_AES_GCM_SetParams)(0x00010030 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_GetContextSize         ((ROM_NCL_AES_GetContextSize)(0x00010034 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_Init                   ((ROM_NCL_AES_Init)(0x00010038 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_InitContext            ((ROM_NCL_AES_InitContext)(0x0001003c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_LockKey                ((ROM_NCL_AES_LockKey)(0x00010040 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_Power                  ((ROM_NCL_AES_Power)(0x00010044 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_SelectKey              ((ROM_NCL_AES_SelectKey)(0x00010048 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_SetIv                  ((ROM_NCL_AES_SetIv)(0x0001004c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_SetKey                 ((ROM_NCL_AES_SetKey)(0x00010050 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_SetParams              ((ROM_NCL_AES_SetParams)(0x00010054 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_SetSeed                ((ROM_NCL_AES_SetSeed)(0x00010058 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_AES_Update                 ((ROM_NCL_AES_Update)(0x0001005c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_Clear                 ((ROM_NCL_DRBG_Clear)(0x00010060 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_Config                ((ROM_NCL_DRBG_Config)(0x00010064 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_FinalizeContext       ((ROM_NCL_DRBG_FinalizeContext)(0x00010068 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_Generate              ((ROM_NCL_DRBG_Generate)(0x0001006c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_GetContextSize        ((ROM_NCL_DRBG_GetContextSize)(0x00010070 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_Init                  ((ROM_NCL_DRBG_Init)(0x00010074 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_InitContext           ((ROM_NCL_DRBG_InitContext)(0x00010078 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_Instantiate           ((ROM_NCL_DRBG_Instantiate)(0x0001007c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_Power                 ((ROM_NCL_DRBG_Power)(0x00010080 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_Reseed                ((ROM_NCL_DRBG_Reseed)(0x00010084 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_DRBG_UnInstantiate         ((ROM_NCL_DRBG_UnInstantiate)(0x00010088 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_Clear                  ((ROM_NCL_ECC_Clear)(0x0001008c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_ECDHKeyExchange        ((ROM_NCL_ECC_ECDHKeyExchange)(0x00010090 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_ECDSASignData          ((ROM_NCL_ECC_ECDSASignData)(0x00010094 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_ECDSASignHash          ((ROM_NCL_ECC_ECDSASignHash)(0x00010098 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_ECDSAVerifyData        ((ROM_NCL_ECC_ECDSAVerifyData)(0x0001009c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_ECDSAVerifyHash        ((ROM_NCL_ECC_ECDSAVerifyHash)(0x000100a0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_FinalizeContext        ((ROM_NCL_ECC_FinalizeContext)(0x000100a4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_GenKey                 ((ROM_NCL_ECC_GenKey)(0x000100a8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_GetContextSize         ((ROM_NCL_ECC_GetContextSize)(0x000100ac + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_GetCurveData           ((ROM_NCL_ECC_GetCurveData)(0x000100b0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_Init                   ((ROM_NCL_ECC_Init)(0x000100b4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_InitContext            ((ROM_NCL_ECC_InitContext)(0x000100b8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_PointVerify            ((ROM_NCL_ECC_PointVerify)(0x000100bc + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_Power                  ((ROM_NCL_ECC_Power)(0x000100c0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_SetCurve               ((ROM_NCL_ECC_SetCurve)(0x000100c4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_ECC_SetSeed                ((ROM_NCL_ECC_SetSeed)(0x000100c8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_MISC_GetModuleDescription  ((ROM_NCL_MISC_GetModuleDescription)(0x000100cc + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_MISC_GetStatus             ((ROM_NCL_MISC_GetStatus)(0x000100d0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_MISC_Init                  ((ROM_NCL_MISC_Init_A1)(0x000100d4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_MISC_SelfTest              ((ROM_NCL_MISC_SelfTest)(0x000100d8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_MISC_TamperDetected        ((ROM_NCL_MISC_TamperDetected)(0x000100dc + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_Clear                  ((ROM_NCL_RSA_Clear)(0x000100e0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_ClearKey               ((ROM_NCL_RSA_ClearKey)(0x000100e4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_Decrypt                ((ROM_NCL_RSA_Decrypt)(0x000100e8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_Encrypt                ((ROM_NCL_RSA_Encrypt)(0x000100ec + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_FinalizeContext        ((ROM_NCL_RSA_FinalizeContext)(0x000100f0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_GetContextSize         ((ROM_NCL_RSA_GetContextSize)(0x000100f4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_GetStatus              ((ROM_NCL_RSA_GetStatus)(0x000100f8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_Init                   ((ROM_NCL_RSA_Init)(0x000100fc + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_InitContext            ((ROM_NCL_RSA_InitContext)(0x00010100 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_Power                  ((ROM_NCL_RSA_Power)(0x00010104 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_SetKey                 ((ROM_NCL_RSA_SetKey)(0x00010108 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_SignData               ((ROM_NCL_RSA_SignData)(0x0001010c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_SignHash               ((ROM_NCL_RSA_SignHash)(0x00010110 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_VerifyData             ((ROM_NCL_RSA_VerifyData)(0x00010114 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_RSA_VerifyHash             ((ROM_NCL_RSA_VerifyHash)(0x00010118 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_Calc                   ((ROM_NCL_SHA_Calc_A1)(0x0001011c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_Clear                  ((ROM_NCL_SHA_Clear)(0x00010120 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_FinalizeContext        ((ROM_NCL_SHA_FinalizeContext)(0x00010124 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_Finish                 ((ROM_NCL_SHA_Finish)(0x00010128 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_GetContextSize         ((ROM_NCL_SHA_GetContextSize)(0x0001012c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_HKDF_Calc              ((ROM_NCL_SHA_HKDF_Calc)(0x00010130 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_HMAC_Calc              ((ROM_NCL_SHA_HMAC_Calc)(0x00010134 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_HMAC_Finish            ((ROM_NCL_SHA_HMAC_Finish)(0x00010138 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_HMAC_Start             ((ROM_NCL_SHA_HMAC_Start)(0x0001013c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_HMAC_Update            ((ROM_NCL_SHA_HMAC_Update)(0x00010140 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_Init                   ((ROM_NCL_SHA_Init)(0x00010144 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_InitContext            ((ROM_NCL_SHA_InitContext)(0x00010148 + THUMB_BIT)            )
#define TIP_ROM_A1_ADDR_NCL_SHA_Power                  ((ROM_NCL_SHA_Power)(0x0001014c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_Reset                  ((ROM_NCL_SHA_Reset)(0x00010150 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_Start                  ((ROM_NCL_SHA_Start)(0x00010154 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NCL_SHA_Update                 ((ROM_NCL_SHA_Update)(0x00010158 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_ClearInt                  ((ROM_NVIC_ClearInt)(0x0001015c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_ConfigPriority            ((ROM_NVIC_ConfigPriority)(0x00010160 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_ConfigTrapPriority        ((ROM_NVIC_ConfigTrapPriority)(0x00010164 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_Default_HW_Handler_L      ((ROM_NVIC_Default_HW_Handler_L)(0x00010168 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_Default_SW_Handler_L      ((ROM_NVIC_Default_SW_Handler_L)(0x0001016c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_EnableInt                 ((ROM_NVIC_EnableInt)(0x00010170 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_Init                      ((ROM_NVIC_Init )(0x00010174 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_InitTab                   ((ROM_NVIC_InitTab)(0x00010178 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_InstallHwHandler          ((ROM_NVIC_InstallHwHandler)(0x0001017c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_InstallHwTrap             ((ROM_NVIC_InstallHwTrap)(0x00010180 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_InstallSwHandler          ((ROM_NVIC_InstallSwHandler)(0x00010184 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_InstallSwTrap             ((ROM_NVIC_InstallSwTrap)(0x00010188 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_IntEnabled                ((ROM_NVIC_IntEnabled)(0x0001018c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_Isr_L                     ((ROM_NVIC_Isr_L)(0x00010190 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_PendingInt                ((ROM_NVIC_PendingInt)(0x00010194 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_PrintRegs                 ((ROM_NVIC_PrintRegs)(0x00010198 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_PrintVersion              ((ROM_NVIC_PrintVersion)(0x0001019c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_Reset                     ((ROM_NVIC_Reset)(0x000101a0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_BSF_L         ((ROM_NVIC_TrapRouting_BSF_L)(0x000101a4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_DBG_L         ((ROM_NVIC_TrapRouting_DBG_L)(0x000101a8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_HDF_L         ((ROM_NVIC_TrapRouting_HDF_L)(0x000101ac + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_MMNG_L        ((ROM_NVIC_TrapRouting_MMNG_L)(0x000101b0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_NMI_L         ((ROM_NVIC_TrapRouting_NMI_L)(0x000101b4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_PSV_L         ((ROM_NVIC_TrapRouting_PSV_L)(0x000101b8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_RES0_L        ((ROM_NVIC_TrapRouting_RES0_L)(0x000101bc + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_RES10_L       ((ROM_NVIC_TrapRouting_RES10_L)(0x000101c0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_RES13_L       ((ROM_NVIC_TrapRouting_RES13_L)(0x000101c4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_RES7_L        ((ROM_NVIC_TrapRouting_RES7_L)(0x000101c8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_RES8_L        ((ROM_NVIC_TrapRouting_RES8_L)(0x000101cc + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_RES9_L        ((ROM_NVIC_TrapRouting_RES9_L)(0x000101d0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_RST_L         ((ROM_NVIC_TrapRouting_RST_L)(0x000101d4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_SVC_L         ((ROM_NVIC_TrapRouting_SVC_L)(0x000101d8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_SYST_L        ((ROM_NVIC_TrapRouting_SYST_L)(0x000101dc + THUMB_BIT))
#define TIP_ROM_A1_ADDR_NVIC_TrapRouting_USGF_L        ((ROM_NVIC_TrapRouting_USGF_L)(0x000101e0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_ClearPendingSysTickInt     ((ROM_SCS_ClearPendingSysTickInt)(0x000101e4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_DivideByZeroEnable         ((ROM_SCS_DivideByZeroEnable)(0x000101e8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_EnableTrap                 ((ROM_SCS_EnableTrap)(0x000101ec + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_FPEnableAccess             ((ROM_SCS_FPEnableAccess)(0x000101f0 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_FPLazyContextSave          ((ROM_SCS_FPLazyContextSave)(0x000101f4 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_FPStatePreservation        ((ROM_SCS_FPStatePreservation)(0x000101f8 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_GetActiveVectorNumber      ((ROM_SCS_GetActiveVectorNumber)(0x000101fc + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_GetPendingVectorNumber     ((ROM_SCS_GetPendingVectorNumber)(0x00010200 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_PrintRegs                  ((ROM_SCS_PrintRegs)(0x00010204 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_PrintVersion               ((ROM_SCS_PrintVersion)(0x00010208 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_SetTrapPriority            ((ROM_SCS_SetTrapPriority)(0x0001020c + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_SetVectorTable             ((ROM_SCS_SetVectorTable)(0x00010210 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_SCS_TraceEnable                ((ROM_SCS_TraceEnable)(0x00010214 + THUMB_BIT))

#define TIP_ROM_A1_ADDR_RNG_IntHandler                 ((ROM_RNG_IntHandler)(0x00014a94 + THUMB_BIT))
#define TIP_ROM_A1_ADDR_PKA_IntHandler                 ((ROM_PKA_IntHandler)(0x00018b04 + THUMB_BIT))

#define TIP_ROM_A2_ADDR_RNG_IntHandler                 ((ROM_RNG_IntHandler)(0x0001483c + THUMB_BIT))
#define TIP_ROM_A2_ADDR_PKA_IntHandler                 ((ROM_PKA_IntHandler)(0x00018b54 + THUMB_BIT))
#define TIP_ROM_A3_ADDR_RNG_IntHandler                 ((ROM_RNG_IntHandler)(0x00013ce4 + THUMB_BIT))
#define TIP_ROM_A3_ADDR_PKA_IntHandler                 ((ROM_PKA_IntHandler)(0x000182b4 + THUMB_BIT))
#define TIP_ROM_A3_ADDR_NCL_SHA_Calc                   ((ROM_NCL_SHA_Calc_A3)(0x0001011c + THUMB_BIT))

/**
 *  Function pointers to NCL
 */
ROM_NCL_AES_CCM_Decrypt            pROM_NCL_AES_CCM_Decrypt;
ROM_NCL_AES_CCM_Encrypt            pROM_NCL_AES_CCM_Encrypt;
ROM_NCL_AES_CCM_SetParams          pROM_NCL_AES_CCM_SetParams;
ROM_NCL_AES_Clear                  pROM_NCL_AES_Clear;
ROM_NCL_AES_ClearParams            pROM_NCL_AES_ClearParams;
ROM_NCL_AES_CMAC_Generate          pROM_NCL_AES_CMAC_Generate;
ROM_NCL_AES_CMAC_Verify            pROM_NCL_AES_CMAC_Verify;
ROM_NCL_AES_Decrypt                pROM_NCL_AES_Decrypt;
ROM_NCL_AES_Encrypt                pROM_NCL_AES_Encrypt;
ROM_NCL_AES_FinalizeContext        pROM_NCL_AES_FinalizeContext;
ROM_NCL_AES_GCM_Decrypt            pROM_NCL_AES_GCM_Decrypt;
ROM_NCL_AES_GCM_Encrypt            pROM_NCL_AES_GCM_Encrypt;
ROM_NCL_AES_GCM_SetParams          pROM_NCL_AES_GCM_SetParams;
ROM_NCL_AES_GetContextSize         pROM_NCL_AES_GetContextSize;
ROM_NCL_AES_Init                   pROM_NCL_AES_Init;
ROM_NCL_AES_InitContext            pROM_NCL_AES_InitContext;
ROM_NCL_AES_LockKey                pROM_NCL_AES_LockKey;
ROM_NCL_AES_Power                  pROM_NCL_AES_Power;
ROM_NCL_AES_SelectKey              pROM_NCL_AES_SelectKey;
ROM_NCL_AES_SetIv                  pROM_NCL_AES_SetIv;
ROM_NCL_AES_SetKey                 pROM_NCL_AES_SetKey;
ROM_NCL_AES_SetParams              pROM_NCL_AES_SetParams;
ROM_NCL_AES_SetSeed                pROM_NCL_AES_SetSeed;
ROM_NCL_AES_Update                 pROM_NCL_AES_Update;
ROM_NCL_DRBG_Clear                 pROM_NCL_DRBG_Clear;
ROM_NCL_DRBG_FinalizeContext       pROM_NCL_DRBG_FinalizeContext;
ROM_NCL_DRBG_Generate              pROM_NCL_DRBG_Generate;
ROM_NCL_DRBG_GetContextSize        pROM_NCL_DRBG_GetContextSize;
ROM_NCL_DRBG_Init                  pROM_NCL_DRBG_Init;
ROM_NCL_DRBG_InitContext           pROM_NCL_DRBG_InitContext;
ROM_NCL_DRBG_Instantiate           pROM_NCL_DRBG_Instantiate;
ROM_NCL_DRBG_Power                 pROM_NCL_DRBG_Power;
ROM_RNG_IntHandler                 pROM_RNG_IntHandler;
ROM_PKA_IntHandler                 pROM_PKA_IntHandler;
ROM_NCL_DRBG_UnInstantiate         pROM_NCL_DRBG_UnInstantiate;
ROM_NCL_ECC_Clear                  pROM_NCL_ECC_Clear;
ROM_NCL_ECC_ECDHKeyExchange        pROM_NCL_ECC_ECDHKeyExchange;
ROM_NCL_ECC_ECDSASignData          pROM_NCL_ECC_ECDSASignData;
ROM_NCL_ECC_ECDSASignHash          pROM_NCL_ECC_ECDSASignHash;
ROM_NCL_ECC_ECDSAVerifyData        pROM_NCL_ECC_ECDSAVerifyData;
ROM_NCL_ECC_ECDSAVerifyHash        pROM_NCL_ECC_ECDSAVerifyHash;
ROM_NCL_ECC_FinalizeContext        pROM_NCL_ECC_FinalizeContext;
ROM_NCL_ECC_GenKey                 pROM_NCL_ECC_GenKey;
ROM_NCL_ECC_GetContextSize         pROM_NCL_ECC_GetContextSize;
ROM_NCL_ECC_GetCurveData           pROM_NCL_ECC_GetCurveData;
ROM_NCL_ECC_Init                   pROM_NCL_ECC_Init;
ROM_NCL_ECC_InitContext            pROM_NCL_ECC_InitContext;
ROM_NCL_ECC_PointVerify            pROM_NCL_ECC_PointVerify;
ROM_NCL_ECC_Power                  pROM_NCL_ECC_Power;
ROM_NCL_ECC_SetCurve               pROM_NCL_ECC_SetCurve;
ROM_NCL_ECC_SetSeed                pROM_NCL_ECC_SetSeed;
ROM_NCL_MISC_GetModuleDescription  pROM_NCL_MISC_GetModuleDescription;
ROM_NCL_MISC_GetStatus             pROM_NCL_MISC_GetStatus;
ROM_NCL_MISC_Init_Z1               pROM_NCL_MISC_Init_Z1;
ROM_NCL_MISC_Init_A1               pROM_NCL_MISC_Init_A1;
ROM_NCL_MISC_SelfTest              pROM_NCL_MISC_SelfTest;
ROM_NCL_MISC_TamperDetected        pROM_NCL_MISC_TamperDetected;
ROM_NCL_RSA_Clear                  pROM_NCL_RSA_Clear;
ROM_NCL_RSA_ClearKey               pROM_NCL_RSA_ClearKey;
ROM_NCL_RSA_Decrypt                pROM_NCL_RSA_Decrypt;
ROM_NCL_RSA_Encrypt                pROM_NCL_RSA_Encrypt;
ROM_NCL_RSA_FinalizeContext        pROM_NCL_RSA_FinalizeContext;
ROM_NCL_RSA_GetContextSize         pROM_NCL_RSA_GetContextSize;
ROM_NCL_RSA_GetStatus              pROM_NCL_RSA_GetStatus;
ROM_NCL_RSA_Init                   pROM_NCL_RSA_Init;
ROM_NCL_RSA_InitContext            pROM_NCL_RSA_InitContext;
ROM_NCL_RSA_Power                  pROM_NCL_RSA_Power;
ROM_NCL_RSA_SetKey                 pROM_NCL_RSA_SetKey;
ROM_NCL_RSA_SignData               pROM_NCL_RSA_SignData;
ROM_NCL_RSA_SignHash               pROM_NCL_RSA_SignHash;
ROM_NCL_RSA_VerifyData             pROM_NCL_RSA_VerifyData;
ROM_NCL_RSA_VerifyHash             pROM_NCL_RSA_VerifyHash;
ROM_NCL_SHA_Calc_A1                pROM_NCL_SHA_Calc_A1;
ROM_NCL_SHA_Calc_A3                pROM_NCL_SHA_Calc_A3;
ROM_SEC_SHA_CALC                   pROM_SEC_SHA_Calc;
ROM_NCL_SHA_Clear                  pROM_NCL_SHA_Clear;
ROM_NCL_SHA_FinalizeContext        pROM_NCL_SHA_FinalizeContext;
ROM_NCL_SHA_Finish                 pROM_NCL_SHA_Finish;
ROM_NCL_SHA_GetContextSize         pROM_NCL_SHA_GetContextSize;
ROM_NCL_SHA_HKDF_Calc              pROM_NCL_SHA_HKDF_Calc;
ROM_NCL_SHA_HMAC_Calc              pROM_NCL_SHA_HMAC_Calc;
ROM_NCL_SHA_HMAC_Finish            pROM_NCL_SHA_HMAC_Finish;
ROM_NCL_SHA_HMAC_Start             pROM_NCL_SHA_HMAC_Start;
ROM_NCL_SHA_HMAC_Update            pROM_NCL_SHA_HMAC_Update;
ROM_NCL_SHA_Init                   pROM_NCL_SHA_Init;
ROM_NCL_SHA_InitContext            pROM_NCL_SHA_InitContext;
ROM_NCL_SHA_Power                  pROM_NCL_SHA_Power;
ROM_NCL_SHA_Reset                  pROM_NCL_SHA_Reset;
ROM_NCL_SHA_Start                  pROM_NCL_SHA_Start;
ROM_NCL_SHA_Update                 pROM_NCL_SHA_Update;
#ifdef NCL_DFT_MODE
ROM_NVIC_ClearInt                  pROM_NVIC_ClearInt;
ROM_NVIC_ConfigPriority            pROM_NVIC_ConfigPriority;
ROM_NVIC_ConfigTrapPriority        pROM_NVIC_ConfigTrapPriority;
ROM_NVIC_Default_HW_Handler_L      pROM_NVIC_Default_HW_Handler_L;
ROM_NVIC_Default_SW_Handler_L      pROM_NVIC_Default_SW_Handler_L;
ROM_NVIC_EnableInt                 pROM_NVIC_EnableInt;
ROM_NVIC_Init                      pROM_NVIC_Init;
ROM_NVIC_InitTab                   pROM_NVIC_InitTab;
ROM_NVIC_InstallHwHandler          pROM_NVIC_InstallHwHandler;
ROM_NVIC_InstallHwTrap             pROM_NVIC_InstallHwTrap;
ROM_NVIC_InstallSwHandler          pROM_NVIC_InstallSwHandler;
ROM_NVIC_InstallSwTrap             pROM_NVIC_InstallSwTrap;
ROM_NVIC_IntEnabled                pROM_NVIC_IntEnabled;
ROM_NVIC_Isr_L                     pROM_NVIC_Isr_L;
ROM_NVIC_PendingInt                pROM_NVIC_PendingInt;
ROM_NVIC_PrintRegs                 pROM_NVIC_PrintRegs;
ROM_NVIC_PrintVersion              pROM_NVIC_PrintVersion;
ROM_NVIC_Reset                     pROM_NVIC_Reset;
ROM_NVIC_TrapRouting_BSF_L         pROM_NVIC_TrapRouting_BSF_L;
ROM_NVIC_TrapRouting_DBG_L         pROM_NVIC_TrapRouting_DBG_L;
ROM_NVIC_TrapRouting_HDF_L         pROM_NVIC_TrapRouting_HDF_L;
ROM_NVIC_TrapRouting_MMNG_L        pROM_NVIC_TrapRouting_MMNG_L;
ROM_NVIC_TrapRouting_NMI_L         pROM_NVIC_TrapRouting_NMI_L;
ROM_NVIC_TrapRouting_PSV_L         pROM_NVIC_TrapRouting_PSV_L;
ROM_NVIC_TrapRouting_RES0_L        pROM_NVIC_TrapRouting_RES0_L;
ROM_NVIC_TrapRouting_RES10_L       pROM_NVIC_TrapRouting_RES10_L;
ROM_NVIC_TrapRouting_RES13_L       pROM_NVIC_TrapRouting_RES13_L;
ROM_NVIC_TrapRouting_RES7_L        pROM_NVIC_TrapRouting_RES7_L;
ROM_NVIC_TrapRouting_RES8_L        pROM_NVIC_TrapRouting_RES8_L;
ROM_NVIC_TrapRouting_RES9_L        pROM_NVIC_TrapRouting_RES9_L;
ROM_NVIC_TrapRouting_RST_L         pROM_NVIC_TrapRouting_RST_L;
ROM_NVIC_TrapRouting_SVC_L         pROM_NVIC_TrapRouting_SVC_L;
ROM_NVIC_TrapRouting_SYST_L        pROM_NVIC_TrapRouting_SYST_L;
ROM_NVIC_TrapRouting_USGF_L        pROM_NVIC_TrapRouting_USGF_L;
ROM_SCS_ClearPendingSysTickInt     pROM_SCS_ClearPendingSysTickInt;
ROM_SCS_DivideByZeroEnable         pROM_SCS_DivideByZeroEnable;
ROM_SCS_EnableTrap                 pROM_SCS_EnableTrap;
ROM_SCS_FPEnableAccess             pROM_SCS_FPEnableAccess;
ROM_SCS_FPLazyContextSave          pROM_SCS_FPLazyContextSave;
ROM_SCS_FPStatePreservation        pROM_SCS_FPStatePreservation;
ROM_SCS_GetActiveVectorNumber      pROM_SCS_GetActiveVectorNumber;
ROM_SCS_GetPendingVectorNumber     pROM_SCS_GetPendingVectorNumber;
ROM_SCS_PrintRegs                  pROM_SCS_PrintRegs;
ROM_SCS_PrintVersion               pROM_SCS_PrintVersion;
ROM_SCS_SetTrapPriority            pROM_SCS_SetTrapPriority;
ROM_SCS_SetVectorTable             pROM_SCS_SetVectorTable;
ROM_SCS_TraceEnable                pROM_SCS_TraceEnable;
#endif

SECURED_BOOLEAN_T sec_drbgInitialized = SECURED_FALSE;

static NCL_STATUS_T  tip_Security_Sha_Calc_A3(void* context,NCL_SHA_TYPE_T shaType, const uint8_t *data, uint32_t dataLen, uint8_t* hashDigest)
{
	return pROM_NCL_SHA_Calc_A3(context,NCL_SHA_MODULE_0,  shaType, data, dataLen, hashDigest);
}

/**
 *  @function     tip_init_rom_ncl
 *  @brief        Initialize all function pointers to the TIP ROM, inorder to ues the ROM based NCL
 * @return 0 if the ncl engine was initialized successfully or an error code.
 */
int tip_init_rom_ncl (void)
{
	uint32_t chip_version = CHIP_Get_Version ();
	NCL_STATUS_T ncl_stat;
	platform_printf (KGRN "Arbel NCL init" NEWLINE KNRM);
	
	switch (chip_version) {
		case ARBEL_VERSION_A1:
		case ARBEL_VERSION_A2:
		case ARBEL_VERSION_A3:
			
			pROM_NCL_SHA_Calc_A1              = TIP_ROM_A1_ADDR_NCL_SHA_Calc;
			pROM_SEC_SHA_Calc                 = TIP_ROM_A1_ADDR_NCL_SHA_Calc;
			pROM_NCL_MISC_Init_A1             = TIP_ROM_A1_ADDR_NCL_MISC_Init;
			pROM_NCL_MISC_GetStatus           = TIP_ROM_A1_ADDR_NCL_MISC_GetStatus;
			pROM_NCL_MISC_SelfTest            = TIP_ROM_A1_ADDR_NCL_MISC_SelfTest;
			pROM_NCL_MISC_TamperDetected      = TIP_ROM_A1_ADDR_NCL_MISC_TamperDetected;
			pROM_NCL_MISC_GetModuleDescription= TIP_ROM_A1_ADDR_NCL_MISC_GetModuleDescription;
			pROM_NCL_DRBG_GetContextSize      = TIP_ROM_A1_ADDR_NCL_DRBG_GetContextSize;
			pROM_NCL_DRBG_InitContext         = TIP_ROM_A1_ADDR_NCL_DRBG_InitContext;
			pROM_NCL_DRBG_Power               = TIP_ROM_A1_ADDR_NCL_DRBG_Power;
			pROM_NCL_DRBG_FinalizeContext     = TIP_ROM_A1_ADDR_NCL_DRBG_FinalizeContext;
			pROM_NCL_DRBG_Clear               = TIP_ROM_A1_ADDR_NCL_DRBG_Clear;
			pROM_NCL_DRBG_Init                = TIP_ROM_A1_ADDR_NCL_DRBG_Init;
			pROM_NCL_DRBG_UnInstantiate       = TIP_ROM_A1_ADDR_NCL_DRBG_UnInstantiate;
			pROM_NCL_DRBG_Generate            = TIP_ROM_A1_ADDR_NCL_DRBG_Generate;
		 	pROM_NCL_DRBG_Instantiate         = TIP_ROM_A1_ADDR_NCL_DRBG_Instantiate;
			pROM_NCL_RSA_GetContextSize       = TIP_ROM_A1_ADDR_NCL_RSA_GetContextSize;
			pROM_NCL_RSA_InitContext          = TIP_ROM_A1_ADDR_NCL_RSA_InitContext;
			pROM_NCL_RSA_FinalizeContext      = TIP_ROM_A1_ADDR_NCL_RSA_FinalizeContext;
			pROM_NCL_RSA_Clear                = TIP_ROM_A1_ADDR_NCL_RSA_Clear;
			pROM_NCL_RSA_Init                 = TIP_ROM_A1_ADDR_NCL_RSA_Init;
			pROM_NCL_RSA_ClearKey             = TIP_ROM_A1_ADDR_NCL_RSA_ClearKey;
			pROM_NCL_RSA_Power                = TIP_ROM_A1_ADDR_NCL_RSA_Power;
			pROM_NCL_RSA_GetStatus            = TIP_ROM_A1_ADDR_NCL_RSA_GetStatus;
			pROM_NCL_RSA_SignData             = TIP_ROM_A1_ADDR_NCL_RSA_SignData;
			pROM_NCL_RSA_SignHash             = TIP_ROM_A1_ADDR_NCL_RSA_SignHash;
			pROM_NCL_RSA_VerifyData           = TIP_ROM_A1_ADDR_NCL_RSA_VerifyData;
			pROM_NCL_RSA_VerifyHash           = TIP_ROM_A1_ADDR_NCL_RSA_VerifyHash;
			pROM_NCL_RSA_Decrypt              = TIP_ROM_A1_ADDR_NCL_RSA_Decrypt;
			pROM_NCL_RSA_Encrypt              = TIP_ROM_A1_ADDR_NCL_RSA_Encrypt;
			pROM_NCL_RSA_SetKey               = TIP_ROM_A1_ADDR_NCL_RSA_SetKey;
			pROM_NCL_AES_GetContextSize       = TIP_ROM_A1_ADDR_NCL_AES_GetContextSize;
			pROM_NCL_AES_InitContext          = TIP_ROM_A1_ADDR_NCL_AES_InitContext;
			pROM_NCL_AES_FinalizeContext      = TIP_ROM_A1_ADDR_NCL_AES_FinalizeContext;
			pROM_NCL_AES_Init                 = TIP_ROM_A1_ADDR_NCL_AES_Init;
			pROM_NCL_AES_Clear                = TIP_ROM_A1_ADDR_NCL_AES_Clear;
			pROM_NCL_AES_SetKey               = TIP_ROM_A1_ADDR_NCL_AES_SetKey;
			pROM_NCL_AES_SetParams            = TIP_ROM_A1_ADDR_NCL_AES_SetParams;
			pROM_NCL_AES_SetSeed              = TIP_ROM_A1_ADDR_NCL_AES_SetSeed;
			pROM_NCL_AES_SelectKey            = TIP_ROM_A1_ADDR_NCL_AES_SelectKey;
			pROM_NCL_AES_LockKey              = TIP_ROM_A1_ADDR_NCL_AES_LockKey;
			pROM_NCL_AES_ClearParams          = TIP_ROM_A1_ADDR_NCL_AES_ClearParams;
			pROM_NCL_AES_Power                = TIP_ROM_A1_ADDR_NCL_AES_Power;
			pROM_NCL_AES_CCM_Decrypt          = TIP_ROM_A1_ADDR_NCL_AES_CCM_Decrypt;
			pROM_NCL_AES_CCM_Encrypt          = TIP_ROM_A1_ADDR_NCL_AES_CCM_Encrypt;
			pROM_NCL_AES_CCM_SetParams        = TIP_ROM_A1_ADDR_NCL_AES_CCM_SetParams;
			pROM_NCL_AES_CMAC_Generate        = TIP_ROM_A1_ADDR_NCL_AES_CMAC_Generate;
			pROM_NCL_AES_Decrypt              = TIP_ROM_A1_ADDR_NCL_AES_Decrypt;
			pROM_NCL_AES_Encrypt              = TIP_ROM_A1_ADDR_NCL_AES_Encrypt;
			pROM_NCL_AES_GCM_Decrypt          = TIP_ROM_A1_ADDR_NCL_AES_GCM_Decrypt;
			pROM_NCL_AES_GCM_Encrypt          = TIP_ROM_A1_ADDR_NCL_AES_GCM_Encrypt;
			pROM_NCL_AES_GCM_SetParams        = TIP_ROM_A1_ADDR_NCL_AES_GCM_SetParams;
			pROM_NCL_AES_SetIv                = TIP_ROM_A1_ADDR_NCL_AES_SetIv;
			pROM_NCL_AES_CMAC_Verify          = TIP_ROM_A1_ADDR_NCL_AES_CMAC_Verify;
			pROM_NCL_AES_Update               = TIP_ROM_A1_ADDR_NCL_AES_Update;
			pROM_NCL_SHA_GetContextSize       = TIP_ROM_A1_ADDR_NCL_SHA_GetContextSize;
			pROM_NCL_SHA_InitContext          = TIP_ROM_A1_ADDR_NCL_SHA_InitContext;
			pROM_NCL_SHA_FinalizeContext      = TIP_ROM_A1_ADDR_NCL_SHA_FinalizeContext;
			pROM_NCL_SHA_Clear                = TIP_ROM_A1_ADDR_NCL_SHA_Clear;
			pROM_NCL_SHA_Init                 = TIP_ROM_A1_ADDR_NCL_SHA_Init;
			pROM_NCL_SHA_Start                = TIP_ROM_A1_ADDR_NCL_SHA_Start;
			pROM_NCL_SHA_Update               = TIP_ROM_A1_ADDR_NCL_SHA_Update;
			pROM_NCL_SHA_Finish               = TIP_ROM_A1_ADDR_NCL_SHA_Finish;
			pROM_NCL_SHA_Power                = TIP_ROM_A1_ADDR_NCL_SHA_Power;
			pROM_NCL_SHA_Reset                = TIP_ROM_A1_ADDR_NCL_SHA_Reset;
			pROM_NCL_SHA_HKDF_Calc            = TIP_ROM_A1_ADDR_NCL_SHA_HKDF_Calc;
			pROM_NCL_SHA_HMAC_Calc            = TIP_ROM_A1_ADDR_NCL_SHA_HMAC_Calc;
			pROM_NCL_SHA_HMAC_Finish          = TIP_ROM_A1_ADDR_NCL_SHA_HMAC_Finish;
			pROM_NCL_SHA_HMAC_Start           = TIP_ROM_A1_ADDR_NCL_SHA_HMAC_Start;
			pROM_NCL_SHA_HMAC_Update          = TIP_ROM_A1_ADDR_NCL_SHA_HMAC_Update;
			pROM_NCL_ECC_GetContextSize       = TIP_ROM_A1_ADDR_NCL_ECC_GetContextSize;
			pROM_NCL_ECC_InitContext          = TIP_ROM_A1_ADDR_NCL_ECC_InitContext;
			pROM_NCL_ECC_FinalizeContext      = TIP_ROM_A1_ADDR_NCL_ECC_FinalizeContext;
			pROM_NCL_ECC_Clear                = TIP_ROM_A1_ADDR_NCL_ECC_Clear;
			pROM_NCL_ECC_Init                 = TIP_ROM_A1_ADDR_NCL_ECC_Init;
			pROM_NCL_ECC_Power                = TIP_ROM_A1_ADDR_NCL_ECC_Power;
			pROM_NCL_ECC_SetCurve             = TIP_ROM_A1_ADDR_NCL_ECC_SetCurve;
			pROM_NCL_ECC_SetSeed              = TIP_ROM_A1_ADDR_NCL_ECC_SetSeed;
			pROM_NCL_ECC_ECDSAVerifyHash      = TIP_ROM_A1_ADDR_NCL_ECC_ECDSAVerifyHash;
			pROM_NCL_ECC_ECDHKeyExchange      = TIP_ROM_A1_ADDR_NCL_ECC_ECDHKeyExchange;
			pROM_NCL_ECC_ECDSASignData        = TIP_ROM_A1_ADDR_NCL_ECC_ECDSASignData;
			pROM_NCL_ECC_ECDSASignHash        = TIP_ROM_A1_ADDR_NCL_ECC_ECDSASignHash;
			pROM_NCL_ECC_ECDSAVerifyData      = TIP_ROM_A1_ADDR_NCL_ECC_ECDSAVerifyData;
			pROM_NCL_ECC_GenKey               = TIP_ROM_A1_ADDR_NCL_ECC_GenKey;
			pROM_NCL_ECC_GetCurveData         = TIP_ROM_A1_ADDR_NCL_ECC_GetCurveData;
			pROM_NCL_ECC_PointVerify          = TIP_ROM_A1_ADDR_NCL_ECC_PointVerify;
			

			/*	Init RNG module (for devices with empty OTP NCL)*/
			REG_WRITE (RNGMODE(0), 0x03);
			REG_WRITE (RNGROEN(0, 0), 0xFF);
			REG_WRITE (RNGROEN(0, 1), 0xFF);
			REG_WRITE (RNGROEN(0, 2), 0x3F);

			/* depend on OTP bit FUSTRAP2_FIPS */
			if (READ_REG_FIELD (FUSTRAP2, FUSTRAP2_FIPS) == 0) 
			{
				ncl_stat = pROM_NCL_MISC_Init_A1 (NCL_FIPS_MODE, NULL);
				if (ncl_stat != NCL_STATUS_OK)
					return -1;
			}

			break;			
		default:
			return -1;
	}
	
	switch (chip_version) {
		case ARBEL_VERSION_A1:		
			pROM_RNG_IntHandler               = TIP_ROM_A1_ADDR_RNG_IntHandler;
			pROM_PKA_IntHandler               = TIP_ROM_A1_ADDR_PKA_IntHandler;
			break;
		case ARBEL_VERSION_A2:	
			pROM_RNG_IntHandler               = TIP_ROM_A2_ADDR_RNG_IntHandler;
			pROM_PKA_IntHandler               = TIP_ROM_A2_ADDR_PKA_IntHandler;
			break;
		case ARBEL_VERSION_A3:
			pROM_RNG_IntHandler               = TIP_ROM_A3_ADDR_RNG_IntHandler;
			pROM_PKA_IntHandler               = TIP_ROM_A3_ADDR_PKA_IntHandler;
			pROM_NCL_SHA_Calc_A3              = TIP_ROM_A3_ADDR_NCL_SHA_Calc;
			pROM_SEC_SHA_Calc                 = tip_Security_Sha_Calc_A3;
			break;
		default:
			return -1;
	}
	
	/* since we are using NVIC table in RAM set by TIP_ROM, need to register interrupts */
	NVIC_InstallSwHandler (PKA_INTERRUPT, (void*)pROM_PKA_IntHandler);
	NVIC_InstallSwHandler (RNG_INTERRUPT, (void*)pROM_RNG_IntHandler);
	return 0;
}


/**
 *  @function     tip_image_hash
 *  @brief        measrue hash of signed area of image
 *
 *  @param        imgHeaderInRAM  pointer to header in RAM
 *  @param        imageAddr       address to start hash
 *  @param        imageLen        image size
 *  @param        hash  -  (output) returns the hash digest of the image
 *  @return       0 if pass or an error code
 */
int tip_image_hash (HEADER_GENERAL_T *imgHeaderInRAM,
							   uint8_t *imageAddr,
							   uint32_t imageLen,
							   uint8_t *hash)
{
	uint8_t shaCtx[NCL_SHA_CONTEXT_BYTE_SIZE];	__attribute__ ((aligned (16)))
	NCL_STATUS_T nclStat_ret;

	platform_printf (KNRM "hash image len %#010lx image addr %#010x\n", imageLen, imageAddr);

	/* Limit image to 16MB. It's not a real HW limitation, it's just some limit to protect from image header parsing error. */
	if (imageLen >= _16MB_) {
		platform_printf ("Verify failed, image above 16MB\n");
		return DEFS_STATUS_INVALID_DATA_FIELD;
	}
	if ((*(uint32_t *) imgHeaderInRAM->header.signature == 0) ||
		(*(uint32_t *) imgHeaderInRAM->header.signature == 0xFFFFFFFF)) {
		platform_printf (
			"\nVerify failed, no signature on image header. \n\nFor secure boot use IGPS output_binaries\\Secure files\n\n");
		return DEFS_STATUS_INVALID_DATA_FIELD;
	}
	pROM_NCL_SHA_InitContext (shaCtx);
	if (imageLen & 0xF) {
		imageLen = (imageLen & 0xFFFFFFF0) + 0x10;
		platform_printf (KYEL "Image size not aligned. align to 16 bytes (%#010lx) \n" KNRM,
						 imageLen);
	}

	/* calc SHA on any are that is not DRAM first 16MB */
	if (((uint32_t) imageAddr < TIP_DRAM_WIN0_BASE_ADDR) ||
		((uint32_t) imageAddr >= RAM2_BASE_ADDR)) {
		nclStat_ret =
			pROM_SEC_SHA_Calc(shaCtx, NCL_SHA_TYPE_2_512, imageAddr, imageLen, (uint8_t *) hash);
	/* calc SHA in moving windows (for Z1. For A1 device can use pROM_NCL_SHA_Calc) */
	} else {
		uint32_t block_cnt, size = 0;
		uint32_t block_first = 0;
		uint32_t src_cnt = 0;
		uint32_t dst_addr = (uint32_t) imageAddr;
		pROM_NCL_SHA_Reset (&shaCtx);
		pROM_NCL_SHA_Init (&shaCtx);
		pROM_NCL_SHA_Start (&shaCtx, NCL_SHA_TYPE_2_512);
		size = MIN(imageLen, _64KB_);
		if ((dst_addr % _64KB_) != 0) {
			size = MIN(imageLen, (_64KB_ - (dst_addr % _64KB_)));
		}
		
		pROM_NCL_SHA_Update (&shaCtx, (uint8_t *) dst_addr, size);
		src_cnt += size;
		block_first = READ_REG_FIELD(DRAM1WINA, DRAM1WINA_WINSTART);
		block_cnt = block_first + 1;
		while (src_cnt < imageLen) {
			REG_WRITE (DRAM1WINA, BUILD_FIELD_VAL (DRAM1WINA_WINEN, 1) | block_cnt);
			size = MIN (_64KB_, (imageLen - src_cnt));
			pROM_NCL_SHA_Update (&shaCtx, (uint8_t *) TIP_DRAM_WIN1_BASE_ADDR, size);
			src_cnt += size;
			block_cnt++;
		}
		nclStat_ret = pROM_NCL_SHA_Finish (&shaCtx, (uint8_t *) hash);
		REG_WRITE (DRAM1WINA, BUILD_FIELD_VAL (DRAM1WINA_WINEN, 1) | block_first);
	}

	if (nclStat_ret != NCL_STATUS_OK) {
		platform_printf(KRED "hash failed: nclStat_ret = %#010lx\n" KNRM, nclStat_ret);
	}

	pROM_NCL_SHA_FinalizeContext (shaCtx);

	return (nclStat_ret == NCL_STATUS_OK) ? 0 : ECC_ENGINE_VERIFY_FAILED;
}

/**
 * @function    tip_image_auth_ECC
 *
 * @param      imgHeaderInRAM - Pointer to the image header (located in RAM)
 * @param      key   -  number of PK on OTP to use
 * @param      hash  -  (output) returns the hash digest of the image
 * @param      sigOK -  (output) set to SECURED_TRUE if signature is valid, SECURED_FALSE if not
 * @return
 * @brief
 *             Performs a security (authentication) check on the specified image using a spesific key */
int tip_image_auth_ECC (HEADER_GENERAL_T *imgHeaderInRAM,
							  uint8_t *imageAddr,
							  uint32_t imageLen,
							  uint8_t *key,
							  uint8_t *hash,
							  SECURED_BOOLEAN_T *sigOK)
{
	uint8_t eccCtx[NCL_ECC_CONTEXT_BYTE_SIZE];	__attribute__ ((aligned (16)))
	NCL_ECC_CURVE_DATA_T curve;	__attribute__ ((aligned (16)))
	NCL_ECC_POINT_T pubKey = { .X.number = (uint32_t *) key,
							   .X.size = (uint32_t) ECC_KEY_SIZE_DWORD_384,
							   .Y.number = (uint32_t *) (key + ECC_KEY_SIZE_DWORD_384 * sizeof (uint32_t)),
							   .Y.size = (uint32_t) ECC_KEY_SIZE_DWORD_384 };	__attribute__ ((aligned (16)))
	NCL_BN_T rSig = { .number = (uint32_t *) imgHeaderInRAM->header.signature,
					  .size = (uint32_t) ECC_KEY_SIZE_DWORD_384 };	__attribute__ ((aligned (16)))
	NCL_BN_T sSig = { .number = (uint32_t *) (imgHeaderInRAM->header.signature +
											  ECC_KEY_SIZE_DWORD_384 * sizeof (uint32_t)),
					  .size = (uint32_t) ECC_KEY_SIZE_DWORD_384 };	__attribute__ ((aligned (16)))

	NCL_STATUS_T nclStat_ret;
	NCL_STATUS_T nclStat;
	int status = ECC_ENGINE_VERIFY_FAILED;

	*sigOK = SECURED_FALSE;
	/*-----------------------------------------------------------------------------------------------------*/
	/* Init curve parameters                                                                               */
	/*-----------------------------------------------------------------------------------------------------*/
	curve.p = (NCL_BN_T){ (uint32_t *) pArr, sizeof (pArr) / sizeof (uint32_t) };
	curve.a = (NCL_BN_T){ (uint32_t *) aArr, sizeof (aArr) / sizeof (uint32_t) };
	curve.b = (NCL_BN_T){ (uint32_t *) bArr, sizeof (bArr) / sizeof (uint32_t) };
	curve.x = (NCL_BN_T){ (uint32_t *) xArr, sizeof (xArr) / sizeof (uint32_t) };
	curve.y = (NCL_BN_T){ (uint32_t *) yArr, sizeof (yArr) / sizeof (uint32_t) };
	curve.n = (NCL_BN_T){ (uint32_t *) nArr, sizeof (nArr) / sizeof (uint32_t) };
	curve.h = 0x1;

	platform_printf ("\trSig %#010lx-%#010lx-%#010lx-...\n", rSig.number[0], rSig.number[1],
					 rSig.number[2]);
	platform_printf ("\tsSig %#010lx-%#010lx-%#010lx-...\n", sSig.number[0], sSig.number[1],
					 sSig.number[2]);

	/* Limit image to 16MB. It's not a real HW limitation, it's just some limit to protect from
	 * image header parsing error. */
	if (imageLen >= _16MB_) {
		platform_printf ("Verify failed, image > 16MB\n");
		return FIRMWARE_IMAGE_GET_SIZE_FAILED;
	}
	if ((*(uint32_t *) imgHeaderInRAM->header.signature == 0) ||
		(*(uint32_t *) imgHeaderInRAM->header.signature == 0xFFFFFFFF)) {
		platform_printf (
			"\nVerify failed, no signature on image header. \n\nFor secure boot use IGPS output_binaries\\Secure files\n\n");
		return ECC_ENGINE_BAD_SIGNATURE;
	}
	if (imageLen & 0xF) {
		imageLen = (imageLen & 0xFFFFFFF0) + 0x10;
		platform_printf (KYEL "Image size not aligned. align to 16 bytes (%#010lx) \n" KNRM,
						 imageLen);
	}

	pROM_NCL_ECC_InitContext (eccCtx);
	pROM_NCL_ECC_Init (eccCtx);
	pROM_NCL_ECC_SetCurve (eccCtx, &curve);

	nclStat_ret = pROM_NCL_ECC_ECDSAVerifyHash (eccCtx, &nclStat, &rSig, &sSig, NCL_SHA_TYPE_2_512,
												(uint8_t *) hash, &pubKey, &curve);

	if (nclStat_ret == NCL_STATUS_OK) {
		if (nclStat == NCL_STATUS_OK) {
			/* anti glitch */
			if (nclStat_ret != NCL_STATUS_OK || nclStat != NCL_STATUS_OK) {
				*sigOK = SECURED_FALSE;
			} else {
				*sigOK = SECURED_TRUE;
				status = 0;
			}
		} else {
			*sigOK = SECURED_FALSE;
			platform_printf_dbg ("ECDSAVerify ret %#010lx stat %#010lx\n", nclStat_ret,
							 nclStat);
		}
	} else {
		*sigOK = SECURED_FALSE;
	}
	pROM_NCL_ECC_FinalizeContext (eccCtx);
	return status;
}

/**
 *  @function     mbedtls_hardware_poll
 *  @brief
 *
 *  @param        data Data seed for DRBG. not used.
 *  @param        output Output buffer for random data
 *  @param        len Length of random data requesed
 *  @param        olen Length of output data received
 *  @return       0 if pass or an error code
 */
int mbedtls_hardware_poll (void *data, unsigned char *output, size_t len, size_t *olen)
{
	NCL_STATUS_T nclStat = NCL_STATUS_OK;

	nclStat =
		pROM_NCL_DRBG_Generate (NULL, NULL, 0, (uint8_t*)output, (uint32_t) len);

	if (nclStat == NCL_STATUS_OK) {
		*olen = len;
		return 0;
	} else {
		platform_printf ("tip: %s failed. status %#010lx\n", __FUNCTION__, nclStat);
		return RNG_ENGINE_RANDOM_FAILED;
	}
}

/**
 *  @function     tip_ncl_status
 *  @brief        This function translates NCL status values to Cerberus status
 *
 *  @param        status NCL error code
 *  @param        error Cerberus error code
 *  @return       0 if pass or an error code
 */
int tip_ncl_status (NCL_STATUS_T status, int error)
{
	if (status == NCL_STATUS_OK)
		return 0;
	else
		return error; 
}
