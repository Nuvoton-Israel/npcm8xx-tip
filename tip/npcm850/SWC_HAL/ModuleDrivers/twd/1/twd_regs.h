/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2010-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   twd_regs.h                                                                                            */
/*            This file contains Timer Watch Dog (TWD) module registers                                    */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef _TWD_REGS_H
#define _TWD_REGS_H

#include __CHIP_H_FROM_DRV()

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                     Timer Watch Dog (TWD) Registers                                     */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define TWCFG                   (TWD_BASE_ADDR + 0x000), TWD_ACCESS, 8
#define TWCP                    (TWD_BASE_ADDR + 0x002), TWD_ACCESS, 8
#define TWDT0                   (TWD_BASE_ADDR + 0x004), TWD_ACCESS, 16
#define T0CSR                   (TWD_BASE_ADDR + 0x006), TWD_ACCESS, 8
#define WDCNT                   (TWD_BASE_ADDR + 0x008), TWD_ACCESS, 8
#define WDSDM                   (TWD_BASE_ADDR + 0x00A), TWD_ACCESS, 8
#ifdef TWD_CAPABILITY_TIMER_READ_COUNTER
#define TWMT0                   (TWD_BASE_ADDR + 0x00C), TWD_ACCESS, 16
#endif
#ifdef TWD_CAPABILITY_WD_READ_COUNTER
#define TWMWD                   (TWD_BASE_ADDR + 0x00E), TWD_ACCESS, 8
#endif
#define WDCP                    (TWD_BASE_ADDR + 0x010), TWD_ACCESS, 8
#define TWD_VER                 (TWD_BASE_ADDR + 0x014), TWD_ACCESS, 8

/*---------------------------------------------------------------------------------------------------------*/
/* TWCFG register fields                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define TWCFG_LTWCFG            0,  1
#define TWCFG_LTWCP             1,  1
#define TWCFG_LTWDT0            2,  1
#define TWCFG_LWDCNT            3,  1
#define TWCFG_WDCT0I            4,  1
#define TWCFG_WDSDME            5,  1
#ifdef TWD_CAPABILITY_VCC_POR_RESET
#define TWCFG_WDRST_MODE        6,  2
#else
#define TWCFG_WDRST_MODE        6,  1
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* TWCP register fields                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define TWCP_MDIV               0,  4

/*---------------------------------------------------------------------------------------------------------*/
/* T0CSR register fields                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define T0CSR_RST               0,  1
#define T0CSR_TC                1,  1
#ifdef TWD_CAPABILITY_TWD_INTERRUPT_EN
#define T0CSR_INTEN             2,  1
#endif
#define T0CSR_WDLTD             3,  1
#define T0CSR_WDRST_STS         4,  1
#define T0CSR_WD_RUN            5,  1
#define T0CSR_TESDIS            7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* WDCP register fields                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define WDCP_WDIV               0,  4

#endif /* _TWD_REGS_H */

