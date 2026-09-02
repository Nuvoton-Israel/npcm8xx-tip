/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2023 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*       hal_cfg_L1.h                                                                                      */
/*            Arbel TIP FW L1 HAL main modules                                                             */
/* Project:   Arbel TIP FW                                                                                 */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef _TIP_FW_HAL_CFG_H_
#define _TIP_FW_HAL_CFG_H_

/* special flag for fusing the OTP into RAM3 area instead of actual OTP , for debug purposes */
#ifndef TIP_DUMMY_OTP_ON_RAM
#define TIP_DUMMY_OTP_ON_RAM 0 /* in production should be 0 */
#endif

#ifndef FUSE_OTP
#define FUSE_OTP 0 /* in production should be 1 */
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             UNUSED MODULES                                              */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifndef __GNUC__
#define __GNUC__
#endif

#ifndef __thumb__
#define __thumb__
#endif

#define NO_LIBC

/*---------------------------------------------------------------------------------------------------------*/
/* Module Drivers exclusion                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#undef  GIC_MODULE_TYPE
#undef  STRP_MODULE_TYPE
#undef  PM_CHAN_MODULE_TYPE
#undef  OTP_MODULE_TYPE
#undef  EWOC_MODULE_TYPE
#undef  ICU_MODULE_TYPE
#undef  SHI_MODULE_TYPE
#undef  MFT_MODULE_TYPE
#undef  DCU_MODULE_TYPE
#undef  ITIM8_MODULE_TYPE
#undef  PWM_MODULE_TYPE
#undef  SWC_MODULE_TYPE
#undef  HFCG_MODULE_TYPE
#undef  STI_MODULE_TYPE
#undef  KBC_HOST_MODULE_TYPE
#undef  SMB_MODULE_TYPE
#undef  ADC_MODULE_TYPE
#undef  PMC_MODULE_TYPE
#undef  SIO_MODULE_TYPE
#undef  GDMA_MODULE_TYPE
#undef  LFCG_MODULE_TYPE
#undef  SHM_MODULE_TYPE
#undef  SCFG_MODULE_TYPE
#undef  CR_UART_MODULE_TYPE
#undef  MTC_MODULE_TYPE
#undef  DAC_MODULE_TYPE
#undef  KBSCAN_MODULE_TYPE
#undef  SYST_MODULE_TYPE
#undef  MIWU_MODULE_TYPE
#undef  RNG_MODULE_TYPE
#undef  SIB_MODULE_TYPE
#undef  CPS_MODULE_TYPE
#undef  MSWC_MODULE_TYPE
#undef  PS2_MODULE_TYPE
#undef  PECI_MODULE_TYPE
#undef  DES_MODULE_TYPE
#undef  EMC_MODULE_TYPE
#undef  VCD_MODULE_TYPE
#undef  ESPI_MODULE_TYPE
#undef  SPI_MODULE_TYPE
#undef  SD_MODULE_TYPE
#undef  MC_MODULE_TYPE
#undef  ECE_MODULE_TYPE
#undef  EMC_MODULE_TYPE
#undef  GMAC_MODULE_TYPE
// #undef  TMC_MODULE_TYPE
#undef  JTM_MODULE_TYPE
#undef  FLM_MODULE_TYPE

/* NVIC capabilites */
#define DYNAMIC_SW_DISPATCH_TABLE

/*---------------------------------------------------------------------------------------------------------*/
/* Logical Drivers exclusion                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#undef  CIRLED_MODULE_TYPE
#undef  FLASH_DEV_MODULE_TYPE
#undef  TACHO_MODULE_TYPE
#undef  ACPI_MODULE_TYPE
#undef  TIMER_MODULE_TYPE
#undef  CIR_MODULE_TYPE
#undef  FLASH_MODULE_TYPE
#undef  RTC_MODULE_TYPE

#define GPIO_EVENT_SUPPORT
#define GPIO_TIP_EVENT_SUPPORT


/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOGICAL DRIVERS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* NCL Module                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#undef NCL_MODULE_TYPE
#undef RSA_MODULE_TYPE
#undef RNG_MODULE_TYPE
#undef AES_MODULE_TYPE
#undef SHA_MODULE_TYPE
#undef DRBG_MODULE_TYPE
#undef HKDF_MODULE_TYPE
#undef HMAC_MODULE_TYPE
#undef PKA_MODULE_TYPE

/* NCL lib selectable features */
#define NCL_ECC
#define NCL_RNG
#define NCL_SHA
#define NCL_DRBG
#define NCL_AES
#define NCL_RSA
#ifdef LMS_ENABLE
//for A2 - - use HAL functions directly 
#define LMS_MODULE_TYPE                 hw_sha
//for A3 - use NCL LMS function 
#define NCL_LMS

#endif

/* Print function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define _DEBUG_
#ifdef _DEBUG_
#include "./serial_printf/serial_printf.h"
#define HAL_PRINT_CAPABILITY
#define HAL_PRINT  serial_printf
#else
#define HAL_PRINT(...)
#endif

#endif /* _TIP_FW_HAL_CFG_H_ */

