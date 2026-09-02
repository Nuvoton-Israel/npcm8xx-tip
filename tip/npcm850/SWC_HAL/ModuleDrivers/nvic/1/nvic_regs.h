/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2013-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   nvic_regs.h                                                                                           */
/*            This file contains NVIC module registers                                                     */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef _NVIC_REGS_H
#define _NVIC_REGS_H

#include __CHIP_H_FROM_DRV()

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                 Interrupt Control Unit (NVIC) Registers                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define NVIC_ISER(n)    (NVIC_BASE_ADDR + 0x000 + (4*(n))), NVIC_ACCESS, 32  // Set-Enable Registers 0-15
#define NVIC_ICER(n)    (NVIC_BASE_ADDR + 0x080 + (4*(n))), NVIC_ACCESS, 32  // Clear-Enable Registers 0-15
#define NVIC_ISPR(n)    (NVIC_BASE_ADDR + 0x100 + (4*(n))), NVIC_ACCESS, 32  // Set-Pending Registers 0-15
#define NVIC_ICPR(n)    (NVIC_BASE_ADDR + 0x180 + (4*(n))), NVIC_ACCESS, 32  // Clear-Pending Registers 0-15
#define NVIC_IABR(n)    (NVIC_BASE_ADDR + 0x200 + (4*(n))), NVIC_ACCESS, 32  // Active Bit Registers 0-15
#define NVIC_IPR(n)     (NVIC_BASE_ADDR + 0x300 + (4*(n))), NVIC_ACCESS, 32  // Priority Registers 0-123

/*---------------------------------------------------------------------------------------------------------*/
/* NVIC_IPR fields                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_IPR_PRI_N(n)       (8*(n)),  8  // Priority fields 0-3

#endif /* _NVIC_REGS_H */

