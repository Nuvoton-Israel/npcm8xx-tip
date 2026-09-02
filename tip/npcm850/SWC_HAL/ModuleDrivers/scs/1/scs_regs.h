/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2013-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   scs_regs.h                                                                                            */
/*            This file contains The System Control Space (SCS) module registers                           */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef _SCS_REGS_H
#define _SCS_REGS_H

#include __CHIP_H_FROM_DRV()

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            SCS Registers                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* SCB registers                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_CPUID               (SCS_BASE_ADDR + 0xD00), SCS_ACCESS, 32
#define SCS_ICSR                (SCS_BASE_ADDR + 0xD04), SCS_ACCESS, 32
#define SCS_VTOR                (SCS_BASE_ADDR + 0xD08), SCS_ACCESS, 32
#define SCS_AIRCR               (SCS_BASE_ADDR + 0xD0C), SCS_ACCESS, 32
#define SCS_SCR                 (SCS_BASE_ADDR + 0xD10), SCS_ACCESS, 32
#define SCS_CCR                 (SCS_BASE_ADDR + 0xD14), SCS_ACCESS, 32
#define SCS_SHPR(n)             (SCS_BASE_ADDR + 0xD18 + (4 * (n))), SCS_ACCESS, 32
#define SCS_SHCSR               (SCS_BASE_ADDR + 0xD24), SCS_ACCESS, 32
#define SCS_CFSR                (SCS_BASE_ADDR + 0xD28), SCS_ACCESS, 32
#define SCS_UFSR                (SCS_BASE_ADDR + 0xD2A), SCS_ACCESS, 16
#define SCS_HFSR                (SCS_BASE_ADDR + 0xD2C), SCS_ACCESS, 32
#define SCS_DFSR                (SCS_BASE_ADDR + 0xD30), SCS_ACCESS, 32
#define SCS_MMFSR               (SCS_BASE_ADDR + 0xD28), SCS_ACCESS,  8
#define SCS_MMFAR               (SCS_BASE_ADDR + 0xD34), SCS_ACCESS, 32
#define SCS_BFSR                (SCS_BASE_ADDR + 0xD29), SCS_ACCESS,  8
#define SCS_BFAR                (SCS_BASE_ADDR + 0xD38), SCS_ACCESS, 32
#define SCS_AFSR                (SCS_BASE_ADDR + 0xD3C), SCS_ACCESS, 32
#define SCS_CPACR               (SCS_BASE_ADDR + 0xD88), SCS_ACCESS, 32
#define SCS_DEMCR               (SCS_BASE_ADDR + 0xDFC), SCS_ACCESS, 32

/*---------------------------------------------------------------------------------------------------------*/
/* Additional SCB registers for the FP extension                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_FPCCR               (SCS_BASE_ADDR + 0xF34), SCS_ACCESS, 32
#define SCS_FPCAR               (SCS_BASE_ADDR + 0xF38), SCS_ACCESS, 32
#define SCS_FPDSCR              (SCS_BASE_ADDR + 0xF3C), SCS_ACCESS, 32
#define SCS_MVFR0               (SCS_BASE_ADDR + 0xF40), SCS_ACCESS, 32
#define SCS_MVFR1               (SCS_BASE_ADDR + 0xF44), SCS_ACCESS, 32

/*---------------------------------------------------------------------------------------------------------*/
/* System control and ID registers not in the SCB                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_ICTR                (SCS_BASE_ADDR + 0x004), SCS_ACCESS, 32
#define SCS_ACTLR               (SCS_BASE_ADDR + 0x008), SCS_ACCESS, 32
#define SCS_STIR                (SCS_BASE_ADDR + 0xF00), SCS_ACCESS, 32
#define SCS_PID(n)              (SCS_BASE_ADDR + 0xFE0 + (4 * ((n) % 4)) - (0x10 * ((n) / 4))), SCS_ACCESS, 32
#define SCS_CID(n)              (SCS_BASE_ADDR + 0xFF0 + (4 * (n))), SCS_ACCESS, 32


/*---------------------------------------------------------------------------------------------------------*/
/* SCS_CPUID fields                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_CPUID_REVISION      0,   4
#define SCS_CPUID_PARTNO        4,  12
#define SCS_CPUID_ARCHITECTURE  16,  4
#define SCS_CPUID_VARIANT       20,  4
#define SCS_CPUID_IMPLEMENTER   24,  8

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_ICSR fields                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_ICSR_VECTACTIVE     0,   9  // The exception number of the current executing exception
#define SCS_ICSR_RETTOBASE      11,  1
#define SCS_ICSR_VECTPENDING    12,  9
#define SCS_ICSR_ISRPENDING     22,  1
#define SCS_ICSR_ISRPREEMPT     23,  1
#define SCS_ICSR_PENDSTCLR      25,  1
#define SCS_ICSR_PENDSTSET      26,  1
#define SCS_ICSR_PENDSVCLR      27,  1
#define SCS_ICSR_PENDSVSET      28,  1
#define SCS_ICSR_NMIPENDSET     31,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_VTOR fields                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_VTOR_TBLOFF         7,  25  // Bits [31:7] of the vector table address

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_CCR fields                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_CCR_NONBASETHRDENA  0,  1
#define SCS_CCR_USERSETMPEND    1,  1
#define SCS_CCR_UNALIGN_TRP     3,  1
#define SCS_CCR_DIV_0_TRP       4,  1
#define SCS_CCR_BFHFNMIGN       8,  1
#define SCS_CCR_STKALIGN        9,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_SHPR fields                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_SHPR_PRI_N(n)       (8*(n)),  8  // Priority fields 0-3

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_SHCSR fields                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_SHCSR_MEMFAULTACT       0,  1
#define SCS_SHCSR_BUSFAULTACT       1,  1
#define SCS_SHCSR_USGFAULTACT       3,  1
#define SCS_SHCSR_SVCALLACT         7,  1
#define SCS_SHCSR_MONITORACT        8,  1
#define SCS_SHCSR_PENDSVACT         10, 1
#define SCS_SHCSR_SYSTICKACT        11, 1
#define SCS_SHCSR_USGFAULTPENDED    12, 1
#define SCS_SHCSR_MEMFAULTPENDED    13, 1
#define SCS_SHCSR_BUSFAULTPENDED    14, 1
#define SCS_SHCSR_SVCALLPENDED      15, 1
#define SCS_SHCSR_MEMFAULTENA       16, 1
#define SCS_SHCSR_BUSFAULTENA       17, 1
#define SCS_SHCSR_USGFAULTENA       18, 1

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_CFSR fields                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_CFSR_IACCVIOL       0,  1
#define SCS_CFSR_DACCVIOL       1,  1
#define SCS_CFSR_MUNSTKERR      3,  1
#define SCS_CFSR_MSTKERR        4,  1
#define SCS_CFSR_MLSPERR        5,  1
#define SCS_CFSR_MMARVALID      7,  1
#define SCS_CFSR_IBUSERR        8,  1
#define SCS_CFSR_PRECISERR      9,  1
#define SCS_CFSR_IMPRECISERR    10, 1
#define SCS_CFSR_UNSTKERR       11, 1
#define SCS_CFSR_STKERR         12, 1
#define SCS_CFSR_LSPERR         13, 1
#define SCS_CFSR_BFARVALID      15, 1
#define SCS_CFSR_UNDEFINSTR     16, 1
#define SCS_CFSR_INVSTATE       17, 1
#define SCS_CFSR_INVPC          18, 1
#define SCS_CFSR_NOCP           19, 1
#define SCS_CFSR_UNALIGNED      24, 1
#define SCS_CFSR_DIVBYZERO      25, 1

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_HFSR fields                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_HFSR_VECTTBL        1,  1
#define SCS_HFSR_FORCED         30, 1
#define SCS_HFSR_DEBUGEVT       31, 1

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_FPCCR fields                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_FPCCR_LSPEN         30,  1
#define SCS_FPCCR_ASPEN         31,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_CPACR fields                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_CPACR_CP(n)         (2*(n)),  2

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_DEMCR fields                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_DEMCR_TRCENA        24,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_PID fields                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_PID_NUM             8

/*---------------------------------------------------------------------------------------------------------*/
/* SCS_CID fields                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_CID_NUM             4


/*---------------------------------------------------------------------------------------------------------*/
/* SCS_AIRCR fields                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_AIRCR_VECTRESET             0, 1  // reset the whole chip
#define SCS_AIRCR_VECTCLRACTIVE         1, 1  //
#define SCS_AIRCR_SYSRESETREQ           2, 1  //Writing 1 will trigger a system reset resulting in the System Reset Handler getting invoked.
#define SCS_AIRCR_PRIGROUP              8, 3  // This field lets you split exception priorities into two parts known as the group priority and subpriority
#define SCS_AIRCR_ENDIANESS             15, 1  //
#define SCS_AIRCR_VECTKEY               16, 16 // NOTE: In order to issue a write to this register, the VECTKEY field must be set to 0x05FA.



#endif /* _SCS_REGS_H */

