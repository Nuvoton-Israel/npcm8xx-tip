/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2021 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   npcm850_tip_regs.h                                                                                    */
/*            This file contains Chip Configuration registers                                              */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef __NPCM850_TIP_REGS_H__
#define __NPCM850_TIP_REGS_H__

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                      Chip Configuration Registers                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************/
/*    BMC to TIP Status Register 0 (B2TIPST0)                                                              */
/***********************************************************************************************************/
#define B2TIPST0                             (TIP_CTRL_BASE_ADDR + 0x00),  MEM,    16         /* Offset: 00h */
#define B2TIPST0_B2TIPSTAT15_0                0,    16               /* 0-15 B2TIPSTAT15_0 (BMC_to_TIP Status Bits 15_0) */

/***********************************************************************************************************/
/*    BMC to TIP Status Register 1 (B2TIPST1)                                                              */
/***********************************************************************************************************/
#define B2TIPST1                             (TIP_CTRL_BASE_ADDR + 0x02),  MEM,    16         /* Offset: 02h */
#define B2TIPST1_B2TIPSTAT31_16               0,    16               /* 0-15 B2TIPSTAT31_16 (BMC_to_TIP Status Bits 31_16) */

/***********************************************************************************************************/
/*    TIP to BMC Notification Register 0 (TIP2BNT0)                                                        */
/***********************************************************************************************************/
#define TIP2BNT0                             (TIP_CTRL_BASE_ADDR + 0x04),  MEM,    16         /* Offset: 04h */
#define TIP2BNT0_TIP2BNOT15_0                 0,    16               /* 0-15 TIP2BNOT15_0 (TIP_to_BMC Notification Bits 15_0) */

/***********************************************************************************************************/
/*    TIP to BMC Notification Register 1 (TIP2BNT1)                                                        */
/***********************************************************************************************************/
#define TIP2BNT1                             (TIP_CTRL_BASE_ADDR + 0x06),  MEM,    16         /* Offset: 06h */
#define TIP2BNT1_TIP2BNOT31_16                0,    16               /* 0-15 TIP2BNOT31_16 (TIP_to_BMC Notification Bits 31_16) */

/***********************************************************************************************************/
/*    TIP Control Status Register (TIP_CTL_STS)                                                            */
/***********************************************************************************************************/
#define TIP_CTL_STS                          (TIP_CTRL_BASE_ADDR + 0x08),  MEM,    16         /* Offset: 08h */
#define TIP_CTL_STS_TIP_EXE_BLK               15,   1               /* 15 TIP_EXE_BLK (TIP Execution Block) */
#define TIP_CTL_STS_TIP_SW_RST                14,   1               /* 14 TIP_SW_RST (TIP Software Reset) */
#ifdef _ARBEL_Z1_
#define TIP_CTL_STS_TIP_RST_CTL               13,   1               /* 13 TIP_RST_CTL (TIP Reset Control) */
#endif
#define TIP_CTL_STS_CPID                      11,   2               /* 11-12 CPID (Coprocessor ID) */
#define TIP_CTL_STS_DBGRST_STS                10,   1               /* 10 DBGRST_STS (Debugger Reset Status) */
#define TIP_CTL_STS_BMC_CRST_EV               9,    1               /* 9 BMC_CRST_EV (BMC Core Reset Event) */
#define TIP_CTL_STS_BMC_CRST_STS              8,    1               /* 8 BMC_CRST_STS (BMC Core Reset Status) */
#define TIP_CTL_STS_TIPnSPILOAD_EN            7,    1               /* 7 oTIPnSPILOAD_EN (Enable pulse generation on LKGPO2.) */
#define TIP_CTL_STS_BOOT_CFGLK                4,    1               /* 4 BOOT_CFGLK (Boot Configuration Lock) */
#define TIP_CTL_STS_BSPI30_DIS                3,    1               /* 1 BSPI30_DIS (Boot From SPI3 CS0 Disabled) */
#define TIP_CTL_STS_BSPI10_DIS                2,    1               /* 0 BSPI10_DIS (Boot From SPI1 CS0 Disabled) */
#define TIP_CTL_STS_BSPI01_DIS                1,    1               /* 1 BSPI01_DIS (Boot From SPI0 CS1 Disabled) */
#define TIP_CTL_STS_BSPI00_DIS_UNUSED         0,    1               /* 0 BSPI00_DIS (Boot From SPI0 CS0 Disabled) */

/***********************************************************************************************************/
/*    TIP Configuration Register (TIPCFGR)                                                                 */
/***********************************************************************************************************/
#define TIPCFGR                              (TIP_CTRL_BASE_ADDR + 0x0A),  MEM,    16         /* Offset: 0Ah */
#define TIPCFGR_NVICTESTEN                    15,   1               /* 15 NVICTESTEN (NVIC Test Enable) */
#define TIPCFGR_TWD_TM                        14,   1               /* 14 TWD_TM (TWD Test Enable) */
#define TIPCFGR_AUWDI                         13,   1               /* 13 AUWDI (Watchdog Forcing Test Enable) */
#define TIPCFGR_STCFEN                        12,   1               /* 12 STCFEN (STC Freeze Enable) */
#define TIPCFGR_APB_CLK_DIV                   4,    4               /* 4-7 APB_CLK_DIV (APB Clock Divider) */
#define TIPCFGR_EN_FL_INIT                    1,    1               /* 0 EN_FL_INIT (Enable Flash Initialization) */
#define TIPCFGR_CKGATE                        0,    1               /* 0 CKGATE (Clock Gating Enable) */

/***********************************************************************************************************/
/*    Window Lock Register 1 (LKREG1)                                                                      */
/***********************************************************************************************************/
#define LKREG1                               (TIP_CTRL_BASE_ADDR + 0x0C),  MEM,    16         /* Offset: 0Ch */
#define LKREG1_DRWLK                          14,   1               /* 14 DRWLK (DRAM Access Window Lock) */
#define LKREG1_SRWLK                          13,   1               /* 13 SRWLK (SRAM Access Window Lock) */
#define LKREG1_S4WLK                          12,   1               /* 12 S4WLK (System Access Window 4 Lock) */
#define LKREG1_S3WLK                          11,   1               /* 11 S3WLK (System Access Window 3 Lock) */
#define LKREG1_S2WLK                          10,   1               /* 10 S2WLK (System Access Window 2 Lock) */
#define LKREG1_S1WLK                          9,    1               /* 9 S1WLK (System Access Window 1 Lock) */
#define LKREG1_S0WLK                          8,    1               /* 8 S0WLK (System Access Window 0 Lock) */
#define LKREG1_P7WLK                          7,    1               /* 7 P7WLK (Peripheral Access Window 7 Lock) */
#define LKREG1_P6WLK                          6,    1               /* 6 P6WLK (Peripheral Access Window 6 Lock) */
#define LKREG1_P5WLK                          5,    1               /* 5 P5WLK (Peripheral Access Window 5 Lock) */
#define LKREG1_P4WLK                          4,    1               /* 4 P4WLK (Peripheral Access Window 4 Lock) */
#define LKREG1_P3WLK                          3,    1               /* 3 P3WLK (Peripheral Access Window 3 Lock) */
#define LKREG1_P2WLK                          2,    1               /* 2 P2WLK (Peripheral Access Window 2 Lock) */
#define LKREG1_P1WLK                          1,    1               /* 1 P1WLK (Peripheral Access Window 1 Lock) */
#define LKREG1_P0WLK                          0,    1               /* 0 P0WLK (Peripheral Access Window 0 Lock) */

/***********************************************************************************************************/
/*    Window Lock Register 2 (LKREG2)                                                                      */
/***********************************************************************************************************/
#define LKREG2                               (TIP_CTRL_BASE_ADDR + 0x0E),  MEM,    16         /* Offset: 0Eh */
#define LKREG2_MISCWLK                        3,    1               /* 3 MISCWLK (Miscellaneous Access Window Lock) */
#define LKREG2_SPXWLK                         2,    1               /* 2 SPXWLK (SPIX Access Window Lock) */
#define LKREG2_SP3WLK                         1,    1               /* 1 SP3WLK (SPI3 Access Window Lock) */
#define LKREG2_SP0WLK                         0,    1               /* 0 SP0WLK (SPI0 Access Window Lock) */

/***********************************************************************************************************/
/*    TIP to Coprocessor Interrupt Register (TIP2CP_INT)                                                   */
/***********************************************************************************************************/
#define TIP2CP_INT                           (TIP_CTRL_BASE_ADDR + 0x10),  MEM,    16         /* Offset: 10h */
#define TIP2CP_INT_CP_INT2                    2,    1               /* 2 CP_INT2 (Coprocessor Interrupt 2) */
#define TIP2CP_INT_CP_INT1                    1,    1               /* 1 CP_INT1 (Coprocessor Interrupt 1) */
#define TIP2CP_INT_CP_INT0                    0,    1               /* 0 CP_INT0 (Coprocessor Interrupt 0) */

/***********************************************************************************************************/
/*    SRAM Access Window Control Register (SRAMWINC)                                                       */
/***********************************************************************************************************/
#define SRAMWINC                             (TIP_CTRL_BASE_ADDR + 0x14),  MEM,    16         /* Offset: 14h */
#define SRAMWINC_WINEN                        15,   1               /* 15 WINEN (Window Enable) */
#define SRAMWINC_WINSIZE                      8,    7               /* 8-14 WINSIZE (Window Size) */
#define SRAMWINC_WINWRDIS                     7,    1               /* 7 WINWRDIS (Window Write Disable) */
#define SRAMWINC_WINSTART                     0,    7               /* 0-6 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    SPI0 and SPI1 Access Window Control Register (SPI01WINC)                                             */
/***********************************************************************************************************/
#define SPI01WINC                            (TIP_CTRL_BASE_ADDR + 0x16),  MEM,    16         /* Offset: 16h */
#define SPI01WINC_SPI1RWDIS                   15,   1               /* 15 SPI1RWDIS (SPI1 Module Registers Window Write Disable) */
#define SPI01WINC_SPI1RRDIS                   14,   1               /* 14 SPI1RRDIS (SPI1 Module Registers Window Read Disable) */
#define SPI01WINC_S1CS3RDIS                   13,   1               /* 13 S1CS3RDIS (SPI1 Chip Select 3 Window Access Disable) */
#define SPI01WINC_S1CS2RDIS                   12,   1               /* 12 S1CS2RDIS (SPI1 Chip Select 2 Window Access Disable) */
#define SPI01WINC_S1CS1RDIS                   11,   1               /* 11 S1CS1RDIS (SPI1 Chip Select 1 Window Access Disable) */
#define SPI01WINC_S1CS0RDIS                   10,   1               /* 10 S1CS0RDIS (SPI1 Chip Select 0 Window Access Disable) */
#define SPI01WINC_SPI0RWDIS                   9,    1               /* 9 SPI0RWDIS (SPI0 Module Registers Window Write Disable) */
#define SPI01WINC_SPI0RRDIS                   8,    1               /* 8 SPI0RRDIS (SPI0 Module Registers Window Read Disable) */
#define SPI01WINC_S0CS1WDIS                   5,    1               /* 5 S0CS1WDIS (SPI0 Chip Select 1 Window Write Disable) */
#define SPI01WINC_S0CS0WDIS                   4,    1               /* 4 S0CS0WDIS (SPI0 Chip Select 0 Window Write Disable) */
#define SPI01WINC_S0CS1RDIS                   1,    1               /* 1 S0CS1RDIS (SPI0 Chip Select 1 Window Read Disable) */
#define SPI01WINC_S0CS0RDIS                   0,    1               /* 0 S0CS0RDIS (SPI0 Chip Select 0 Window Read Disable) */

/***********************************************************************************************************/
/*    SPI3 Access Window Control Register (SPI3WINC)                                                       */
/***********************************************************************************************************/
#define SPI3WINC                             (TIP_CTRL_BASE_ADDR + 0x18),  MEM,    16         /* Offset: 18h */
#define SPI3WINC_SHMWDIS                      13,   1               /* 13 SHMWDIS (SHM Module Registers Window Write Disable) */
#define SPI3WINC_SHMRRDIS                     12,   1               /* 12 SHMRRDIS (SHM Module Registers Window Read Disable) */
#define SPI3WINC_RAM0WDIS                     11,   1               /* 11 RAM0WDIS (RAM3 Window Write Disable) */
#define SPI3WINC_RAM0RDIS                     10,   1               /* 10 RAM0RDIS (RAM3 Window Read Disable) */
#define SPI3WINC_SPI3RWDIS                    9,    1               /* 9 SPI3RWDIS (SPI3 Module Registers Window Write Disable) */
#define SPI3WINC_SPI3RRDIS                    8,    1               /* 8 SPI3RRDIS (SPI3 Module Registers Window Read Disable) */
#define SPI3WINC_S3CS3WDIS                    7,    1               /* 7 S3CS3WDIS (SPI3 Chip Select 3 Window Write Disable) */
#define SPI3WINC_S3CS2WDIS                    6,    1               /* 6 S3CS2WDIS (SPI3 Chip Select 2 Window Write Disable) */
#define SPI3WINC_S3CS1WDIS                    5,    1               /* 5 S3CS1WDIS (SPI3 Chip Select 1 Window Write Disable) */
#define SPI3WINC_S3CS0WDIS                    4,    1               /* 4 S3CS0WDIS (SPI3 Chip Select 0 Window Write Disable) */
#define SPI3WINC_S3CS3RDIS                    3,    1               /* 3 S3CS3RDIS (SPI3 Chip Select 3 Window Read Disable) */
#define SPI3WINC_S3CS2RDIS                    2,    1               /* 2 S3CS2RDIS (SPI3 Chip Select 2 Window Read Disable) */
#define SPI3WINC_S3CS1RDIS                    1,    1               /* 1 S3CS1RDIS (SPI3 Chip Select 1 Window Read Disable) */
#define SPI3WINC_S3CS0RDIS                    0,    1               /* 0 S3CS0RDIS (SPI3 Chip Select 0 Window Read Disable) */

/***********************************************************************************************************/
/*    SPIX Access Window Control Register (SPIXWINC)                                                       */
/***********************************************************************************************************/
#define SPIXWINC                             (TIP_CTRL_BASE_ADDR + 0x1A),  MEM,    16         /* Offset: 1Ah */
#define SPIXWINC_SPIXRWDIS                    9,    1               /* 9 SPIXRWDIS (SPIX Module Registers Window Write Disable) */
#define SPIXWINC_SPIXRRDIS                    8,    1               /* 8 SPIXRRDIS (SPIX Module Registers Window Read Disable) */
#define SPIXWINC_SXCS1WDIS                    5,    1               /* 5 SXCS1WDIS (SPIX Chip Select 1 Window Write Disable) */
#define SPIXWINC_SXCS0WDIS                    4,    1               /* 4 SXCS0WDIS (SPIX Chip Select 0 Window Write Disable) */
#define SPIXWINC_SXCS1RDIS                    1,    1               /* 1 SXCS1RDIS (SPIX Chip Select 1 Window Read Disable) */
#define SPIXWINC_SXCS0RDIS                    0,    1               /* 0 SXCS0RDIS (SPIX Chip Select 0 Window Read Disable) */

/***********************************************************************************************************/
/*    Miscellaneous Access Windows Control Register (MISCWINC)                                             */
/***********************************************************************************************************/
#define MISCWINC                             (TIP_CTRL_BASE_ADDR + 0x1C),  MEM,    16         /* Offset: 1Ch */
#define MISCWINC_GLBLEN                       8,    1               /* 8 GLBLEN (Global System Access Enable) */
#define MISCWINC_VDMXWDIS                     5,    1               /* 5 VDMXWDIS (VDMX Registers Window Write Disable) */
#define MISCWINC_VDMAWDIS                     4,    1               /* 4 VDMAWDIS (VDMA Registers Window Write Disable) */
#define MISCWINC_GDMA3WDIS                    3,    1               /* 3 GDMA3WDIS (GDMA3 Registers Window Write Disable) */
#define MISCWINC_GDMA2WDIS                    2,    1               /* 2 GDMA2WDIS (GDMA2 Registers Window Write Disable) */
#define MISCWINC_GDMA1WDIS                    1,    1               /* 1 GDMA1WDIS (GDMA1 Registers Window Write Disable) */
#define MISCWINC_GDMA0WDIS                    0,    1               /* 0 GDMA0WDIS (GDMA0 Registers Window Write Disable) */

/***********************************************************************************************************/
/*    Peripheral Access Window 0 Control 1 Register (PWIN0C1)                                              */
/***********************************************************************************************************/
#define PWIN0C1                              (TIP_CTRL_BASE_ADDR + 0x20),  MEM,    16         /* Offset: 20h */
#define PWIN0C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define PWIN0C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define PWIN0C1_WINSIZE                       0,    5               /* 0-4 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    Peripheral Access Window 0 Control 2 Register (PWIN0C2)                                              */
/***********************************************************************************************************/
#define PWIN0C2                              (TIP_CTRL_BASE_ADDR + 0x22),  MEM,    16         /* Offset: 22h */
#define PWIN0C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    Peripheral Access Window 1 Control 1 Register (PWIN1C1)                                              */
/***********************************************************************************************************/
#define PWIN1C1                              (TIP_CTRL_BASE_ADDR + 0x24),  MEM,    16         /* Offset: 24h */
#define PWIN1C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define PWIN1C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define PWIN1C1_WINSIZE                       0,    5               /* 0-4 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    Peripheral Access Window 1 Control 2 Register (PWIN1C2)                                              */
/***********************************************************************************************************/
#define PWIN1C2                              (TIP_CTRL_BASE_ADDR + 0x26),  MEM,    16         /* Offset: 26h */
#define PWIN1C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    Peripheral Access Window 2 Control 1 Register (PWIN2C1)                                              */
/***********************************************************************************************************/
#define PWIN2C1                              (TIP_CTRL_BASE_ADDR + 0x28),  MEM,    16         /* Offset: 28h */
#define PWIN2C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define PWIN2C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define PWIN2C1_WINSIZE                       0,    5               /* 0-4 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    Peripheral Access Window 2 Control 2 Register (PWIN2C2)                                              */
/***********************************************************************************************************/
#define PWIN2C2                              (TIP_CTRL_BASE_ADDR + 0x2A),  MEM,    16         /* Offset: 2Ah */
#define PWIN2C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    Peripheral Access Window 3 Control 1 Register (PWIN3C1)                                              */
/***********************************************************************************************************/
#define PWIN3C1                              (TIP_CTRL_BASE_ADDR + 0x2C),  MEM,    16         /* Offset: 2Ch */
#define PWIN3C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define PWIN3C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define PWIN3C1_WINSIZE                       0,    5               /* 0-4 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    Peripheral Access Window 3 Control 2 Register (PWIN3C2)                                              */
/***********************************************************************************************************/
#define PWIN3C2                              (TIP_CTRL_BASE_ADDR + 0x2E),  MEM,    16         /* Offset: 2Eh */
#define PWIN3C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    Peripheral Access Window 4 Control 1 Register (PWIN4C1)                                              */
/***********************************************************************************************************/
#define PWIN4C1                              (TIP_CTRL_BASE_ADDR + 0x30),  MEM,    16         /* Offset: 30h */
#define PWIN4C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define PWIN4C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define PWIN4C1_WINSIZE                       0,    5               /* 0-4 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    Peripheral Access Window 4 Control 2 Register (PWIN4C2)                                              */
/***********************************************************************************************************/
#define PWIN4C2                              (TIP_CTRL_BASE_ADDR + 0x32),  MEM,    16         /* Offset: 32h */
#define PWIN4C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    Peripheral Access Window 5 Control 1 Register (PWIN5C1)                                              */
/***********************************************************************************************************/
#define PWIN5C1                              (TIP_CTRL_BASE_ADDR + 0x34),  MEM,    16         /* Offset: 34h */
#define PWIN5C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define PWIN5C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define PWIN5C1_WINSIZE                       0,    5               /* 0-4 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    Peripheral Access Window 5 Control 2 Register (PWIN5C2)                                              */
/***********************************************************************************************************/
#define PWIN5C2                              (TIP_CTRL_BASE_ADDR + 0x36),  MEM,    16         /* Offset: 36h */
#define PWIN5C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    Peripheral Access Window 6 Control 1 Register (PWIN6C1)                                              */
/***********************************************************************************************************/
#define PWIN6C1                              (TIP_CTRL_BASE_ADDR + 0x38),  MEM,    16         /* Offset: 38h */
#define PWIN6C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define PWIN6C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define PWIN6C1_WINSIZE                       0,    5               /* 0-4 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    Peripheral Access Window 6 Control 2 Register (PWIN6C2)                                              */
/***********************************************************************************************************/
#define PWIN6C2                              (TIP_CTRL_BASE_ADDR + 0x3A),  MEM,    16         /* Offset: 3Ah */
#define PWIN6C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    Peripheral Access Window 7 Control 1 Register (PWIN7C1)                                              */
/***********************************************************************************************************/
#define PWIN7C1                              (TIP_CTRL_BASE_ADDR + 0x3C),  MEM,    16         /* Offset: 3Ch */
#define PWIN7C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define PWIN7C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define PWIN7C1_WINSIZE                       0,    5               /* 0-4 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    Peripheral Access Window 7 Control 2 Register (PWIN7C2)                                              */
/***********************************************************************************************************/
#define PWIN7C2                              (TIP_CTRL_BASE_ADDR + 0x3E),  MEM,    16         /* Offset: 3Eh */
#define PWIN7C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    System Access Window 0 Control 1 Register (SWIN0C1)                                                  */
/***********************************************************************************************************/
#define SWIN0C1                              (TIP_CTRL_BASE_ADDR + 0x40),  MEM,    16         /* Offset: 40h */
#define SWIN0C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define SWIN0C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define SWIN0C1_WINSIZE                       0,    9               /* 0-8 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    System Access Window 0 Control 2 Register (SWIN0C2)                                                  */
/***********************************************************************************************************/
#define SWIN0C2                              (TIP_CTRL_BASE_ADDR + 0x42),  MEM,    16         /* Offset: 42h */
#define SWIN0C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    System Access Window 1 Control 1 Register (SWIN1C1)                                                  */
/***********************************************************************************************************/
#define SWIN1C1                              (TIP_CTRL_BASE_ADDR + 0x44),  MEM,    16         /* Offset: 44h */
#define SWIN1C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define SWIN1C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define SWIN1C1_WINSIZE                       0,    9               /* 0-8 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    System Access Window 1 Control 2 Register (SWIN1C2)                                                  */
/***********************************************************************************************************/
#define SWIN1C2                              (TIP_CTRL_BASE_ADDR + 0x46),  MEM,    16         /* Offset: 46h */
#define SWIN1C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    System Access Window 2 Control 1 Register (SWIN2C1)                                                  */
/***********************************************************************************************************/
#define SWIN2C1                              (TIP_CTRL_BASE_ADDR + 0x48),  MEM,    16         /* Offset: 48h */
#define SWIN2C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define SWIN2C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define SWIN2C1_WINSIZE                       0,    9               /* 0-8 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    System Access Window 2 Control 2 Register (SWIN2C2)                                                  */
/***********************************************************************************************************/
#define SWIN2C2                              (TIP_CTRL_BASE_ADDR + 0x4A),  MEM,    16         /* Offset: 4Ah */
#define SWIN2C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    System Access Window 3 Control 1 Register (SWIN3C1)                                                  */
/***********************************************************************************************************/
#define SWIN3C1                              (TIP_CTRL_BASE_ADDR + 0x4C),  MEM,    16         /* Offset: 4Ch */
#define SWIN3C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define SWIN3C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define SWIN3C1_WINSIZE                       0,    9               /* 0-8 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    System Access Window 3 Control 2 Register (SWIN3C2)                                                  */
/***********************************************************************************************************/
#define SWIN3C2                              (TIP_CTRL_BASE_ADDR + 0x4E),  MEM,    16         /* Offset: 4Eh */
#define SWIN3C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    System Access Window 4 Control 1 Register (SWIN4C1)                                                  */
/***********************************************************************************************************/
#define SWIN4C1                              (TIP_CTRL_BASE_ADDR + 0x50),  MEM,    16         /* Offset: 50h */
#define SWIN4C1_WINEN                         15,   1               /* 15 WINEN (Window Enable) */
#define SWIN4C1_WINWRDIS                      14,   1               /* 14 WINWRDIS (Window Write Disable) */
#define SWIN4C1_WINSIZE                       0,    9               /* 0-8 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    System Access Window 4 Control 2 Register (SWIN4C2)                                                  */
/***********************************************************************************************************/
#define SWIN4C2                              (TIP_CTRL_BASE_ADDR + 0x52),  MEM,    16         /* Offset: 52h */
#define SWIN4C2_WINSTART                      0,    16               /* 0-15 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    Peripheral Access Windows and System Access Windows (General)                                        */
/***********************************************************************************************************/
#define PWINnC1(n)               (REG_ADDR(PWIN0C1) + 4*(n)), MEM, 16   /*  Peripheral Access Window n Control 1 Register (PWINnC1) reset val: 801Fh   */
#define PWINnC2(n)               (REG_ADDR(PWIN0C2) + 4*(n)), MEM, 16   /*  Peripheral Access Window n Control 2 Register (PWINnC2) reset val: 0000h   */
#define PWIN_NUM                 8                                      /*  Number of Peripheral windows */
#define SWINnC1(n)               (REG_ADDR(SWIN0C1) + 4*(n)), MEM, 16   /*  System Access Window n Control 1 Register (SWINnC1) reset val: 81FFh   */
#define SWINnC2(n)               (REG_ADDR(SWIN0C2) + 4*(n)), MEM, 16   /*  System Access Window n Control 2 Register (SWINnC2) reset val: 0000h   */
#define SWIN_NUM                 5                                      /*  Number of System windows */

/***********************************************************************************************************/
/*    DRAM Access Window 0 Address Register (DRAM0WINA)                                                    */
/***********************************************************************************************************/
#define DRAM0WINA                            (TIP_CTRL_BASE_ADDR + 0x54),  MEM,    16         /* Offset: 54h */
#define DRAM0WINA_WINEN                       15,   1               /* 15 WINEN (Window Enable) */
#define DRAM0WINA_WINSTART                    0,    15               /* 0-14 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    DRAM Access Window 0 Control Register (DRAM0WINC)                                                    */
/***********************************************************************************************************/
#define DRAM0WINC                            (TIP_CTRL_BASE_ADDR + 0x56),  MEM,    16         /* Offset: 56h */
#define DRAM0WINC_WINWRDIS                    15,   1               /* 15 WINWRDIS (Window Write Disable) */
#define DRAM0WINC_WINSIZE                     0,    14               /* 0-13 WINSIZE (Window Size) */

/***********************************************************************************************************/
/*    DRAM Access Window 1 Address Register (DRAM1WINA)                                                    */
/***********************************************************************************************************/
#define DRAM1WINA                            (TIP_CTRL_BASE_ADDR + 0x58),  MEM,    16         /* Offset: 58h */
#define DRAM1WINA_WINEN                       15,   1               /* 15 WINEN (Window Enable) */
#define DRAM1WINA_WINSTART                    0,    15               /* 0-14 WINSTART (Window Start Address) */

/***********************************************************************************************************/
/*    DRAM Access Window 1 Control Register (DRAM1WINC)                                                    */
/***********************************************************************************************************/
#define DRAM1WINC                            (TIP_CTRL_BASE_ADDR + 0x5A),  MEM,    16         /* Offset: 5Ah */
#define DRAM1WINC_WINWRDIS                    15,   1               /* 15 WINWRDIS (Window Write Disable) */
#define DRAM1WINC_WINSIZE                     0,    14               /* 0-13 WINSIZE (Window Size) */

/**************************************************************************************************************************/
/*   GPIO Data Out and Direction Register (GPIODOD)                                                                       */
/**************************************************************************************************************************/
#define GPIODOD                              (TIP_CTRL_BASE_ADDR + 0x60),  MEM,    16         /* Offset: 60h */
#define GPIODOD_GPOE7                         15,   1               /* 15 GPOE7 (TIP GPIO 7 Data Enable) */
#define GPIODOD_GPOE6                         14,   1               /* 14 GPOE6 (TIP GPIO 6 Data Enable) */
#define GPIODOD_GPOE5                         13,   1               /* 13 GPOE5 (TIP GPIO 5 Data Enable) */
#define GPIODOD_GPOE4                         12,   1               /* 12 GPOE4 (TIP GPIO 4 Data Enable) */
#define GPIODOD_GPOE3                         11,   1               /* 11 GPOE3 (TIP GPIO 3 Data Enable) */
#define GPIODOD_GPOE2                         10,   1               /* 10 GPOE2 (TIP GPIO 2 Data Enable) */
#define GPIODOD_GPOE1                         9,    1               /* 9 GPOE1 (TIP GPIO 1 Data Enable) */
#define GPIODOD_GPOE0                         8,    1               /* 8 GPOE0 (TIP GPIO 0 Data Enable) */
#define GPIODOD_GPOE(n)                       ((n)+8),1             /* 15-8 GPOEn (TIP GPIO n Data Enable) */
#define GPIODOD_GPDO7                         7,    1               /* 7 GPDO7 (TIP GPIO 7 Data Out) */
#define GPIODOD_GPDO6                         6,    1               /* 6 GPDO6 (TIP GPIO 6 Data Out) */
#define GPIODOD_GPDO5                         5,    1               /* 5 GPDO5 (TIP GPIO 5 Data Out) */
#define GPIODOD_GPDO4                         4,    1               /* 4 GPDO4 (TIP GPIO 4 Data Out) */
#define GPIODOD_GPDO3                         3,    1               /* 3 GPDO3 (TIP GPIO 3 Data Out) */
#define GPIODOD_GPDO2                         2,    1               /* 2 GPDO2 (TIP GPIO 2 Data Out) */
#define GPIODOD_GPDO1                         1,    1               /* 1 GPDO1 (TIP GPIO 1 Data Out) */
#define GPIODOD_GPDO0                         0,    1               /* 0 GPDO0 (TIP GPIO 0 Data Out) */
#define GPIODOD_GPDO(n)                       (n),  1               /* 7-0 GPDOn (TIP GPIO n Data Out) */

/***********************************************************************************************************/
/*    GPIO Data In Register (GPIODIN)                                                                      */
/***********************************************************************************************************/
#define GPIODIN                              (TIP_CTRL_BASE_ADDR + 0x62),  MEM,    16         /* Offset: 62h */
#define GPIODIN_GPI7                          7,    1               /* 7 GPI7 (TIP GPIO 7 Data Input) */
#define GPIODIN_GPI6                          6,    1               /* 6 GPI6 (TIP GPIO 6 Data Input) */
#define GPIODIN_GPI5                          5,    1               /* 5 GPI5 (TIP GPIO 5 Data Input) */
#define GPIODIN_GPI4                          4,    1               /* 4 GPI4 (TIP GPIO 4 Data Input) */
#define GPIODIN_GPI3                          3,    1               /* 3 GPI3 (TIP GPIO 3 Data Input) */
#define GPIODIN_GPI2                          2,    1               /* 2 GPI2 (TIP GPIO 2 Data Input) */
#define GPIODIN_GPI1                          1,    1               /* 1 GPI1 (TIP GPIO 1 Data Input) */
#define GPIODIN_GPI0                          0,    1               /* 0 GPI0 (TIP GPIO 0 Data Input) */
#define GPIODIN_GPI(n)                        (n),  1               /* 7-0 GPIn (TIP GPIO n Data Input) */

/***********************************************************************************************************/
/*    GPIO Pull_Up and Pull_Down Register (GPIOPUD)                                                        */
/***********************************************************************************************************/
#define GPIOPUD                              (TIP_CTRL_BASE_ADDR + 0x64),  MEM,    16         /* Offset: 64h */
#define GPIOPUD_GPPU7                         15,   1               /* 15 GPPU7 (TIP GPIO 7 Pull_Up) */
#define GPIOPUD_GPPU6                         14,   1               /* 14 GPPU6 (TIP GPIO 6 Pull_Up) */
#define GPIOPUD_GPPU5                         13,   1               /* 13 GPPU5 (TIP GPIO 5 Pull_Up) */
#define GPIOPUD_GPPU4                         12,   1               /* 12 GPPU4 (TIP GPIO 4 Pull_Up) */
#define GPIOPUD_GPPU3                         11,   1               /* 11 GPPU3 (TIP GPIO 3 Pull_Up) */
#define GPIOPUD_GPPU2                         10,   1               /* 10 GPPU2 (TIP GPIO 2 Pull_Up) */
#define GPIOPUD_GPPU1                         9,    1               /* 9 GPPU1 (TIP GPIO 1 Pull_Up) */
#define GPIOPUD_GPPU0                         8,    1               /* 8 GPPU0 (TIP GPIO 0 Pull_Up) */
#define GPIOPUD_GPPU(n)                       ((n)+8),1               /* 15-8 GPPUn (TIP GPIO n Pull-Up) */
#define GPIOPUD_GPPD7                         7,    1               /* 7 GPPD7 (TIP GPIO 7 Pull_Down) */
#define GPIOPUD_GPPD6                         6,    1               /* 6 GPPD6 (TIP GPIO 6 Pull_Down) */
#define GPIOPUD_GPPD5                         5,    1               /* 5 GPPD5 (TIP GPIO 5 Pull_Down) */
#define GPIOPUD_GPPD4                         4,    1               /* 4 GPPD4 (TIP GPIO 4 Pull_Down) */
#define GPIOPUD_GPPD3                         3,    1               /* 3 GPPD3 (TIP GPIO 3 Pull_Down) */
#define GPIOPUD_GPPD2                         2,    1               /* 2 GPPD2 (TIP GPIO 2 Pull_Down) */
#define GPIOPUD_GPPD1                         1,    1               /* 1 GPPD1 (TIP GPIO 1 Pull_Down) */
#define GPIOPUD_GPPD0                         0,    1               /* 0 GPPD0 (TIP GPIO 0 Pull_Down) */
#define GPIOPUD_GPPD(n)                       (n),  1               /* 7-0 GPPDn (TIP GPIO n Pull-Down) */

/***********************************************************************************************************/
/*    GPIO VDD_Powered Register (GPIOVDD)                                                                  */
/***********************************************************************************************************/
#define GPIOVDD                              (TIP_CTRL_BASE_ADDR + 0x66),  MEM,    16         /* Offset: 66h */
#define GPIOVDD_GPVD7                         7,    1               /* 7 GPVD7 (TIP GPIO 7 VDD Powered) */
#define GPIOVDD_GPVD6                         6,    1               /* 6 GPVD6 (TIP GPIO 6 VDD Powered) */
#define GPIOVDD_GPVD5                         5,    1               /* 5 GPVD5 (TIP GPIO 5 VDD Powered) */
#define GPIOVDD_GPVD4                         4,    1               /* 4 GPVD4 (TIP GPIO 4 VDD Powered) */
#define GPIOVDD_GPVD3                         3,    1               /* 3 GPVD3 (TIP GPIO 3 VDD Powered) */
#define GPIOVDD_GPVD2                         2,    1               /* 2 GPVD2 (TIP GPIO 2 VDD Powered) */
#define GPIOVDD_GPVD1                         1,    1               /* 1 GPVD1 (TIP GPIO 1 VDD Powered) */
#define GPIOVDD_GPVD0                         0,    1               /* 0 GPVD0 (TIP GPIO 0 VDD Powered) */
#define GPIOVDD_GPVD(n)                       (n),  1               /* 7-0 GPVDn (TIP GPIO n VDD Powered) */

/***********************************************************************************************************/
/*    GPIO Interrupt Enable and Polarity Register (GPIOIEP)                                                */
/***********************************************************************************************************/
#define GPIOIEP                              (TIP_CTRL_BASE_ADDR + 0x68),  MEM,    16         /* Offset: 68h */
#define GPIOIEP_GPPL7                         15,   1               /* 15 GPPL7 (TIP GPIO 7 Polarity) */
#define GPIOIEP_GPPL6                         14,   1               /* 14 GPPL6 (TIP GPIO 6 Polarity) */
#define GPIOIEP_GPPL5                         13,   1               /* 13 GPPL5 (TIP GPIO 5 Polarity) */
#define GPIOIEP_GPPL4                         12,   1               /* 12 GPPL4 (TIP GPIO 4 Polarity) */
#define GPIOIEP_GPPL3                         11,   1               /* 11 GPPL3 (TIP GPIO 3 Polarity) */
#define GPIOIEP_GPPL2                         10,   1               /* 10 GPPL2 (TIP GPIO 2 Polarity) */
#define GPIOIEP_GPPL1                         9,    1               /* 9 GPPL1 (TIP GPIO 1 Polarity) */
#define GPIOIEP_GPPL0                         8,    1               /* 8 GPPL0 (TIP GPIO 0 Polarity) */
#define GPIOIEP_GPPL(n)                       ((n)+8),1               /* 15-8 GPPLn (TIP GPIO n Polarity) */
#define GPIOIEP_GPIE7                         7,    1               /* 7 GPIE7 (TIP GPIO 7 Interrupt Enable) */
#define GPIOIEP_GPIE6                         6,    1               /* 6 GPIE6 (TIP GPIO 6 Interrupt Enable) */
#define GPIOIEP_GPIE5                         5,    1               /* 5 GPIE5 (TIP GPIO 5 Interrupt Enable) */
#define GPIOIEP_GPIE4                         4,    1               /* 4 GPIE4 (TIP GPIO 4 Interrupt Enable) */
#define GPIOIEP_GPIE3                         3,    1               /* 3 GPIE3 (TIP GPIO 3 Interrupt Enable) */
#define GPIOIEP_GPIE2                         2,    1               /* 2 GPIE2 (TIP GPIO 2 Interrupt Enable) */
#define GPIOIEP_GPIE1                         1,    1               /* 1 GPIE1 (TIP GPIO 1 Interrupt Enable) */
#define GPIOIEP_GPIE0                         0,    1               /* 0 GPIE0 (TIP GPIO 0 Interrupt Enable) */
#define GPIOIEP_GPIE(n)                       (n),  1               /* 7-0 GPIEn (TIP GPIO n Interrupt Enable) */

/***********************************************************************************************************/
/*    GPIO Interrupt Level or Edge Register (GPIOILG)                                                      */
/***********************************************************************************************************/
#define GPIOILG                              (TIP_CTRL_BASE_ADDR + 0x6A),  MEM,    16         /* Offset: 6Ah */
#define GPIOILG_GPBE7                         15,   1               /* 15 GPBE7 (TIP GPIO 7 Both Edges) */
#define GPIOILG_GPBE6                         14,   1               /* 14 GPBE6 (TIP GPIO 6 Both Edges) */
#define GPIOILG_GPBE5                         13,   1               /* 13 GPBE5 (TIP GPIO 5 Both Edges) */
#define GPIOILG_GPBE4                         12,   1               /* 12 GPBE4 (TIP GPIO 4 Both Edges) */
#define GPIOILG_GPBE3                         11,   1               /* 11 GPBE3 (TIP GPIO 3 Both Edges) */
#define GPIOILG_GPBE2                         10,   1               /* 10 GPBE2 (TIP GPIO 2 Both Edges) */
#define GPIOILG_GPBE1                         9,    1               /* 9 GPBE1 (TIP GPIO 1 Both Edges) */
#define GPIOILG_GPBE0                         8,    1               /* 8 GPBE0 (TIP GPIO 0 Both Edges) */
#define GPIOILG_GPBE(n)                       ((n)+8),1               /* 15-8 GPBEn (TIP GPIO n Both Edges) */
#define GPIOILG_GPLG7                         7,    1               /* 7 GPLG7 (TIP GPIO 7 Level or Edge Select) */
#define GPIOILG_GPLG6                         6,    1               /* 6 GPLG6 (TIP GPIO 6 Level or Edge Select) */
#define GPIOILG_GPLG5                         5,    1               /* 5 GPLG5 (TIP GPIO 5 Level or Edge Select) */
#define GPIOILG_GPLG4                         4,    1               /* 4 GPLG4 (TIP GPIO 4 Level or Edge Select) */
#define GPIOILG_GPLG3                         3,    1               /* 3 GPLG3 (TIP GPIO 3 Level or Edge Select) */
#define GPIOILG_GPLG2                         2,    1               /* 2 GPLG2 (TIP GPIO 2 Level or Edge Select) */
#define GPIOILG_GPLG1                         1,    1               /* 1 GPLG1 (TIP GPIO 1 Level or Edge Select) */
#define GPIOILG_GPLG0                         0,    1               /* 0 GPLG0 (TIP GPIO 0 Level or Edge Select) */
#define GPIOILG_GPLG(n)                       (n),  1               /* 7-0 GPLGn (TIP GPIO n Level or Edge Select) */

/***********************************************************************************************************/
/*    GPIO Interrupt Status Register (GPIOIST)                                                             */
/***********************************************************************************************************/
#define GPIOIST                              (TIP_CTRL_BASE_ADDR + 0x6C),  MEM,    16         /* Offset: 6Ch */
#define GPIOIST_GPST7                         7,    1               /* 7 GPST7 (TIP GPIO 7 Interrupt Status) */
#define GPIOIST_GPST6                         6,    1               /* 6 GPST6 (TIP GPIO 6 Interrupt Status) */
#define GPIOIST_GPST5                         5,    1               /* 5 GPST5 (TIP GPIO 5 Interrupt Status) */
#define GPIOIST_GPST4                         4,    1               /* 4 GPST4 (TIP GPIO 4 Interrupt Status) */
#define GPIOIST_GPST3                         3,    1               /* 3 GPST3 (TIP GPIO 3 Interrupt Status) */
#define GPIOIST_GPST2                         2,    1               /* 2 GPST2 (TIP GPIO 2 Interrupt Status) */
#define GPIOIST_GPST1                         1,    1               /* 1 GPST1 (TIP GPIO 1 Interrupt Status) */
#define GPIOIST_GPST0                         0,    1               /* 0 GPST0 (TIP GPIO 0 Interrupt Status) */
#define GPIOIST_GPST(n)                       (n),  1               /* 7-0 GPSTn (TIP GPIO n Interrupt Status) */

/***********************************************************************************************************/
/*    Trusted Integrated Processor Debug Control Register (TIP_DBG_CTL)                                    */
/***********************************************************************************************************/
#define TIP_DBG_CTL                          (TIP_SEC_BASE_ADDR + 0x00),  MEM,    16         /* Offset: 00h */
#ifdef _ARBEL_Z1_
#define TIP_DBG_CTL_UART_DUMP_EN              12,   4               /* 12-15 UART_DUMP_EN (UART Dump Enable) */
#define TIP_DBG_CTL_PCIMBX_DUMP_EN            8,    4               /* 8-11 PCIMBX_DUMP_EN (PCI Mail Box Dump Enable) */
#endif
#define TIP_DBG_CTL_BMC_JEN                   4,    4               /* 4-7 BMC_JEN (BMC JTAG Enable) */
#define TIP_DBG_CTL_TIP_JEN                   0,    4               /* 0-3 TIP_JEN (TIP JTAG Enable) */

/***********************************************************************************************************/
/*    Trusted Integrated Processor Test Control Register (TIP_TST_CTL)                                     */
/***********************************************************************************************************/
#define TIP_TST_CTL                          (TIP_SEC_BASE_ADDR + 0x02),  MEM,    16         /* Offset: 02h */
#define TIP_TST_CTL_TST_CTL_LK                15,   1               /* 15 TST_CTL_LK (TIP_TST_CTL Lock) */

/***********************************************************************************************************/
/*    TIP Memory Control Register (TIP_MEM_CTL)                                                            */
/***********************************************************************************************************/
#define TIP_MEM_CTL                          (TIP_SEC_BASE_ADDR + 0x04),  MEM,    16         /* Offset: 04h */
#define TIP_MEM_CTL_SEC_EV_ERR_REN            8,    4               /* 8-11 RSRV_EN (Reserved Enable) */
#define TIP_MEM_CTL_ROM_PAR_EN                4,    4               /* 4-7 ROM_PAR_EN (ROM Parity Enable) */
#define TIP_MEM_CTL_RAM_PAR_EN                0,    4               /* 0-3 RAM_PAR_EN (RAM Parity Enable) */

/***********************************************************************************************************/
/*    Security Event Register (SEC_EV)                                                                     */
/***********************************************************************************************************/
#define SEC_EV                               (TIP_SEC_BASE_ADDR + 0x08),  MEM,    16         /* Offset: 08h */
#define SEC_EV_RAM_PERR                       7,    1               /* 7 RAM_PERR (RAM Parity Check Enable Error) */
#define SEC_EV_ROM_PERR                       6,    1               /* 6 ROM_PERR (ROM Parity Check Enable Error) */
#define SEC_EV_TM_ERR                         1,    1               /* 1 TM_ERR (Test Mode Security Error) */
#define SEC_EV_DBGE_ERR                       0,    1               /* 0 DBGE_ERR (Debug Enable Error) */

/***********************************************************************************************************/
/*    Security Error Register (SEC_ERR)                                                                    */
/***********************************************************************************************************/
#define SEC_ERR                              (TIP_SEC_BASE_ADDR + 0x0A),  MEM,    16         /* Offset: 0Ah */
#define SEC_ERR_SEC_QUAL                      8,    8               /* 8-15 SEC_QUAL (Security Qualifier) */
#define SEC_ERR_SW_SEC_ERR                    7,    1               /* 7 SW_SEC_ERR (Software Security Error) */
#define SEC_ERR_CLR_SEC_ERR                   6,    1               /* 6 CLR_SEC_ERR (Clear Security Error) */
#define SEC_ERR_SEC_EV_IEN                    5,    1               /* 5 SEC_ERR_IEN (Security Error Interrupt Enable) */
#define SEC_ERR_SEC_EV_ERR                    0,    4               /* 0-3 SEC_EV_ERR (Security Event Error) */

/***********************************************************************************************************/
/*    TIP PQA Control Register (TIP_PQA_CTL)                                                               */
/***********************************************************************************************************/
#define TIP_PQA_CTL                          (TIP_SEC_BASE_ADDR + 0x0C),  MEM,    16         /* Offset: 0Ch */
#define TIP_PQA_CTL_TIP_PQA_STS               8,    4               /* 8-11 TIP_PQA_STS (TIP PQA Status) */
#define TIP_PQA_CTL_TIP_PQA_ACT               4,    4               /* 4-7 TIP_PQA_ACT (TIP PQA Active) */
#define TIP_PQA_CTL_TIP_PQA_RD_DIS            0,    4               /* 0-3 TIP_PQA_RD_DIS (TIP PQA Read Disabled) */

/***********************************************************************************************************/
/*    TIP Security Control Register (TIP_SEC_CTL)                                                          */
/***********************************************************************************************************/
#define TIP_SEC_CTL                          (TIP_SEC_BASE_ADDR + 0x10),  MEM,    16         /* Offset: 10h */
#define TIP_SEC_CTL_TIP_SECBOOT               8,    4               /* 8-11 TIP_SECBOOT (TIP Secure Boot) */
#define TIP_SEC_CTL_AES_WR_DIS                6,    1               /* 6 AES_WR_DIS (AES Write Disable) */
#define TIP_SEC_CTL_AES_KEY_LOAD              5,    1               /* 5 AES_KEY_LOAD (AES Key LOAD) */
#define TIP_SEC_CTL_AES_KEY_SEL               4,    1               /* 4 AES_KEY_SEL (AES Key Select) */
#define TIP_SEC_CTL_TIPWD_EN                  3,    1               /* 3 TIPWD_EN (TIP Watchdog Enable) */

/***********************************************************************************************************/
/*    TIP BMC Reset Control Register (TIP_BMC_RCTL)                                                        */
/***********************************************************************************************************/
#define TIP_BMC_RCTL                         (TIP_SEC_BASE_ADDR + 0x12),  MEM,    16         /* Offset: 12h */
#define TIP_BMC_RCTL_RCTL_QUAL                8,    8               /* 8-15 RCTL_QUAL (Reset Control Qualifier) */
#define TIP_BMC_RCTL_SWRST3                   7,    1               /* 7 SWRST3 (Software Reset Control Bit 3) */
#define TIP_BMC_RCTL_SWRST2                   6,    1               /* 6 SWRST2 (Software Reset Control Bit 2) */
#define TIP_BMC_RCTL_SWRST1                   5,    1               /* 5 SWRST1 (Software Reset Control Bit 1) */
#define TIP_BMC_RCTL_A35_ASYNC_RST            4,    1               /* 4 A35_ASYNC_RST (A35 Asynchronous reset Reset) */
#define TIP_BMC_RCTL_TIP_PORST                3,    1               /* 2 TIP_PORST (TIP Generated Power On Reset) */
#define TIP_BMC_RCTL_SET_BMC_CRST             2,    1               /* 2 SET_BMC_CRST (Set BMC Core Reset) */
#define TIP_BMC_RCTL_TIP_RST_EN               1,    1               /* 1 TIP_RST_EN (TIP Reset Enable) */
#define TIP_BMC_RCTL_A35_AFTR_CRST            0,    1               /* 0 A35_AFTR_CRST (A35 After Core Reset) */

/***********************************************************************************************************/
/*    TIP ROM Control Register (TIP_ROM_RCTL)                                                              */
/***********************************************************************************************************/
#define TIP_ROM_RCTL                         (TIP_SEC_BASE_ADDR + 0x14),  MEM,    16              /* Offset: 14h */
#define TIP_ROM_RCTL_BLOCK_CODE_Q             10,   6               /* 10-15 Block_Code_Q (Block Code Qualifier) */
#define TIP_ROM_RCTL_RR_BLK_EN                9,    1               /* 9 RR_BLK_EN (ROM Read Block Enable)       */
#define TIP_ROM_RCTL_RR_BLK_S                 0,    9               /* 0-8 RR_BLK_S (ROM Read Block Size)        */

/***********************************************************************************************************/
/*    TIP ROM Exception Code Register (TIP_ROM_EX_C)                                                       */
/***********************************************************************************************************/
#define TIP_ROM_EX_C                         (TIP_SEC_BASE_ADDR + 0x16),  MEM,    16              /* Offset: 16h */

/***********************************************************************************************************/
/*    TIP Scratchpad 0 Register (TIP_SCR0)                                                                 */
/***********************************************************************************************************/
#define TIP_SCR0                            (TIP_SEC_BASE_ADDR + 0x30),  MEM,    16         /* Offset: 30h */
#define TIP_SCR0_TIP_SCR0                   0,    16               /* 0-15 TIP_SCR0 (TIP Scratchpad 0) */

/* TIP_SCR0 is used by the ROM Code to store the where to start seraching images, will be set by TIP ROM or TIP FW */
#define TIP_SEARCH_HDR_START                TIP_SCR0
#define TIP_SEARCH_HDR_ADDR                 3   ,13                  /* Upper dword of Physical address to start the search */
                                                                     /*  on the next reset (first 3 bit are 0)  */
                                                                     /* meaning the next reset search will start from */
                                                                     /* TIP_SCR0 << 16  */
                                                                     /* address must in be flash address range */


/***********************************************************************************************************/
/*    TIP Scratchpad 0 Register (TIP_SCR1)                                                                 */
/***********************************************************************************************************/
#define TIP_SCR1                             (TIP_SEC_BASE_ADDR + 0x32),  MEM,    16         /* Offset: 32h */
#define TIP_SCR1_TIP_SCR1                     0,    16               /* 0-15 TIP_SCR1 (TIP Scratchpad 1) */

/* TIP_SCR1 is used by the TIP to store the last reset type indication */
#define TIP_RST_INDICATION                  TIP_SCR1
#define TIP_RST_INDICATION_WDRST_STS        15,   1                /* 15 TIP_WDRST_STS (TIP WD Reset Status), used by TIP ROM code */
#define TIP_RST_INDICATION_TIP_SWRST_STS    14,   1                /* 14 TIP_SWRST_STS (TIP SW Reset Status), used by TIP ROM code */
#define TIP_RST_INDICATION_DBGRST_STS       13,   1                /* 13 DBGRST_STS (Debugger Reset Status), used by TIP ROM code */
#define TIP_RST_INDICATION_SW_SEC_ERR       12,   1                /* 12 SW_SEC_ERR (Software Security Error), used by TIP ROM code */
#define TIP_RST_INDICATION_CFGDONE          10,   1                /* 10 Configuration done */
#define TIP_RST_INDICATION_RESSR_23_31      0,    9                /* 0-8 bits 23-31 of RESSR */
#define TIP_RST_INDICATION_PORST            8,    1               /* 31 PORST (Standby Power On Reset Status) */
#define TIP_RST_INDICATION_CORST            7,    1               /* 30 CORST (Core Domain Reset Status) */
#define TIP_RST_INDICATION_WD0RST           6,    1               /* 29 WD0RST (Watchdog 0 Reset Status) */
#define TIP_RST_INDICATION_SWRST1           5,    1               /* 28 SWRST1 (Software Reset 1 Status) */
#define TIP_RST_INDICATION_SWRST2           4,    1               /* 27 SWRST2 (Software Reset 2 Status) */
#define TIP_RST_INDICATION_SWRST3           3,    1               /* 26 SWRST3 (Software Reset 3 Status) */
#define TIP_RST_INDICATION_TIP_RESET        2,    1               /* 25 TIP_RESET */
#define TIP_RST_INDICATION_WD1RST           1,    1               /* 24 WD1RST (Watchdog 1 Reset Status) */
#define TIP_RST_INDICATION_WD2RST           0,    1               /* 23 WD2RST (Watchdog 2 Reset Status) */

/***********************************************************************************************************/
/*    TIP Scratchpad 0 Register (TIP_SCR2)                                                                 */
/***********************************************************************************************************/
#define TIP_SCR2                             (TIP_SEC_BASE_ADDR + 0x34),  MEM,    16         /* Offset: 34h */
#define TIP_SCR2_TIP_SCR2                     0,    16               /* 0-15 TIP_SCR2 (TIP Scratchpad 2) */

/***********************************************************************************************************/
/*    TIP Scratchpad 0 Register (TIP_SCR3)                                                                 */
/***********************************************************************************************************/
#define TIP_SCR3                             (TIP_SEC_BASE_ADDR + 0x36),  MEM,    16         /* Offset: 36h */
#define TIP_SCR3_TIP_SCR3                     0,    16               /* 0-15 TIP_SCR3 (TIP Scratchpad 3) */

/***********************************************************************************************************/
/*    TIP Scratchpad 4 Register (TIP_SCR4)                                                                 */
/***********************************************************************************************************/
#define TIP_SCR4                             (TIP_SEC_BASE_ADDR + 0x38),  MEM,    16         /* Offset: 38h */
#define TIP_SCR4_TIP_SCR4                     0,    16               /* 0-15 TIP_SCR4 (TIP Scratchpad 4) */

/***********************************************************************************************************/
/*    TIP Scratchpad 5 Register (TIP_SCR5)                                                                 */
/***********************************************************************************************************/
#define TIP_SCR5                             (TIP_SEC_BASE_ADDR + 0x3A),  MEM,    16         /* Offset: 3Ah */
#define TIP_SCR5_TIP_SCR5                     0,    16               /* 0-15 TIP_SCR5 (TIP Scratchpad 5) */

/***********************************************************************************************************/
/*    TIP Scratchpad 6 Register (TIP_SCR6)                                                                 */
/***********************************************************************************************************/
#define TIP_SCR6                             (TIP_SEC_BASE_ADDR + 0x3C),  MEM,    16         /* Offset: 3Ch */
#define TIP_SCR6_TIP_SCR6                     0,    16               /* 0-15 TIP_SCR6 (TIP Scratchpad 6) */

#ifdef _PALLADIUM_
/* TIP_SCR6 is used by the CLK module to hold the actual Palladium PLL0 (value between 0-2048) */
#define TIP_PD_ACTUAL_PLL0                  TIP_SCR6
#endif

/***********************************************************************************************************/
/*    TIP Scratchpad 7 Register (TIP_SCR7)                                                                 */
/***********************************************************************************************************/
#define TIP_SCR7                             (TIP_SEC_BASE_ADDR + 0x3E),  MEM,    16         /* Offset: 3Eh */
#define TIP_SCR7_TIP_SCR7                     0,    16               /* 0-15 TIP_SCR7 (TIP Scratchpad 7) */
#define TIP_SCR7_TIP_SCR7_5                   5,    1                /* 0-15 TIP_SCR7_5: Used in PD to raute TIP UART to SI2.
                                                                        Not used in production version */

/***********************************************************************************************************/
/*    TIP PQA 0 Register (TIP_PQA0)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA0                             (TIP_SEC_BASE_ADDR + 0x40),  MEM,    16         /* Offset: 40h */
#define TIP_PQA0_TIP_PQA0                     0,    16               /* 0-15 TIP_PQA0 (TIP PQA 0) */

/***********************************************************************************************************/
/*    TIP PQA 1 Register (TIP_PQA1)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA1                             (TIP_SEC_BASE_ADDR + 0x42),  MEM,    16         /* Offset: 42h */
#define TIP_PQA1_TIP_PQA0                     0,    16               /* 0-15 TIP_PQA0 (TIP PQA 1) */

/***********************************************************************************************************/
/*    TIP PQA 2 Register (TIP_PQA2)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA2                             (TIP_SEC_BASE_ADDR + 0x44),  MEM,    16         /* Offset: 44h */
#define TIP_PQA2_TIP_PQA2                     0,    16               /* 0-15 TIP_PQA2 (TIP PQA 2) */

/***********************************************************************************************************/
/*    TIP PQA 3 Register (TIP_PQA3)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA3                             (TIP_SEC_BASE_ADDR + 0x46),  MEM,    16         /* Offset: 46h */
#define TIP_PQA3_TIP_PQA3                     0,    16               /* 0-15 TIP_PQA3 (TIP PQA 3) */

/***********************************************************************************************************/
/*    TIP PQA 4 Register (TIP_PQA4)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA4                             (TIP_SEC_BASE_ADDR + 0x48),  MEM,    16         /* Offset: 48h */
#define TIP_PQA4_TIP_PQA4                     0,    16               /* 0-15 TIP_PQA4 (TIP PQA 4) */

/***********************************************************************************************************/
/*    TIP PQA 5 Register (TIP_PQA5)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA5                             (TIP_SEC_BASE_ADDR + 0x4A),  MEM,    16         /* Offset: 4Ah */
#define TIP_PQA5_TIP_PQA5                     0,    16               /* 0-15 TIP_PQA5 (TIP PQA 5) */

/***********************************************************************************************************/
/*    TIP PQA 6 Register (TIP_PQA6)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA6                             (TIP_SEC_BASE_ADDR + 0x4C),  MEM,    16         /* Offset: 4Ch */
#define TIP_PQA6_TIP_PQA6                     0,    16               /* 0-15 TIP_PQA6 (TIP PQA6) */

/***********************************************************************************************************/
/*    TIP PQA 7 Register (TIP_PQA7)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA7                             (TIP_SEC_BASE_ADDR + 0x4E),  MEM,    16         /* Offset: 4Eh */
#define TIP_PQA7_TIP_PQA7                     0,    16               /* 0-15 TIP_PQA7 (TIP PQA7) */

/***********************************************************************************************************/
/*    TIP PQA 8 Register (TIP_PQA8)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA8                             (TIP_SEC_BASE_ADDR + 0x50),  MEM,    16         /* Offset: 50h */
#define TIP_PQA8_TIP_PQA8                     0,    16               /* 0-15 TIP_PQA8 (TIP PQA8) */

/***********************************************************************************************************/
/*    TIP PQA 9 Register (TIP_PQA9)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA9                             (TIP_SEC_BASE_ADDR + 0x52),  MEM,    16         /* Offset: 52h */
#define TIP_PQA9_TIP_PQA9                     0,    16               /* 0-15 TIP_PQA9 (TIP PQA9) */

/***********************************************************************************************************/
/*    TIP PQA 10 Register (TIP_PQA10)                                                                      */
/***********************************************************************************************************/
#define TIP_PQA10                            (TIP_SEC_BASE_ADDR + 0x54),  MEM,    16         /* Offset: 54h */
#define TIP_PQA10_TIP_PQA10                   0,    16               /* 0-15 TIP_PQA10 (TIP PQA10) */

/***********************************************************************************************************/
/*    TIP PQA 11 Register (TIP_PQA11)                                                                      */
/***********************************************************************************************************/
#define TIP_PQA11                            (TIP_SEC_BASE_ADDR + 0x56),  MEM,    16         /* Offset: 56h */
#define TIP_PQA11_TIP_PQA11                   0,    16               /* 0-15 TIP_PQA11 (TIP PQA11) */

/***********************************************************************************************************/
/*    TIP PQA 12 Register (TIP_PQA12)                                                                      */
/***********************************************************************************************************/
#define TIP_PQA12                            (TIP_SEC_BASE_ADDR + 0x58),  MEM,    16         /* Offset: 58h */
#define TIP_PQA12_TIP_PQA12                   0,    16               /* 0-15 TIP_PQA12 (TIP PQA12) */

/***********************************************************************************************************/
/*    TIP PQA 13 Register (TIP_PQA13)                                                                      */
/***********************************************************************************************************/
#define TIP_PQA13                            (TIP_SEC_BASE_ADDR + 0x5A),  MEM,    16         /* Offset: 5Ah */
#define TIP_PQA13_TIP_PQA13                   0,    16               /* 0-15 TIP_PQA13 (TIP PQA13) */

/***********************************************************************************************************/
/*    TIP PQA 14 Register (TIP_PQA14)                                                                      */
/***********************************************************************************************************/
#define TIP_PQA14                            (TIP_SEC_BASE_ADDR + 0x5C),  MEM,    16         /* Offset: 5Ch */
#define TIP_PQA14_TIP_PQA14                   0,    16               /* 0-15 TIP_PQA14 (TIP PQA14) */

/***********************************************************************************************************/
/*    TIP PQA 15 Register (TIP_PQA15)                                                                      */
/***********************************************************************************************************/
#define TIP_PQA15                            (TIP_SEC_BASE_ADDR + 0x5E),  MEM,    16         /* Offset: 5Eh */
#define TIP_PQA15_TIP_PQA15                   0,    16               /* 0-15 TIP_PQA15 (TIP PQA15) */

/***********************************************************************************************************/
/*    TIP PQA n Register (TIP_PQAn)                                                                        */
/***********************************************************************************************************/
#define TIP_PQA(n)                           (TIP_SEC_BASE_ADDR + 0x40 + (n) * 2),  MEM,    16  /* Offset: 40h + n*2 */

/***********************************************************************************************************/
/*    AES Key Output 0 Register (AES_KEY_OUT0)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT0                         (TIP_SEC_BASE_ADDR + 0x60),  MEM,    16         /* Offset: 60h */
#define AES_KEY_OUT0_AES_KEY_OUT0             0,    16               /* 0-15 AES_KEY_OUT0 (AES Key Output 0) */

/***********************************************************************************************************/
/*    AES Key Output 1 Register (AES_KEY_OUT1)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT1                         (TIP_SEC_BASE_ADDR + 0x62),  MEM,    16         /* Offset: 62h */
#define AES_KEY_OUT1_AES_KEY_OUT1             0,    16               /* 0-15 AES_KEY_OUT1 (AES Key Output 1) */

/***********************************************************************************************************/
/*    AES Key Output 2 Register (AES_KEY_OUT2)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT2                         (TIP_SEC_BASE_ADDR + 0x64),  MEM,    16         /* Offset: 64h */
#define AES_KEY_OUT2_AES_KEY_OUT2             0,    16               /* 0-15 AES_KEY_OUT2 (AES Key Output 2) */

/***********************************************************************************************************/
/*    AES Key Output 3 Register (AES_KEY_OUT3)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT3                         (TIP_SEC_BASE_ADDR + 0x66),  MEM,    16         /* Offset: 66h */
#define AES_KEY_OUT3_AES_KEY_OUT3             0,    16               /* 0-15 AES_KEY_OUT3 (AES Key Output 3) */

/***********************************************************************************************************/
/*    AES Key Output 4 Register (AES_KEY_OUT4)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT4                         (TIP_SEC_BASE_ADDR + 0x68),  MEM,    16         /* Offset: 68h */
#define AES_KEY_OUT4_AES_KEY_OUT4             0,    16               /* 0-15 AES_KEY_OUT4 (AES Key Output 4) */

/***********************************************************************************************************/
/*    AES Key Output 5 Register (AES_KEY_OUT5)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT5                         (TIP_SEC_BASE_ADDR + 0x6A),  MEM,    16         /* Offset: 6Ah */
#define AES_KEY_OUT5_AES_KEY_OUT5             0,    16               /* 0-15 AES_KEY_OUT5 (AES Key Output 5) */

/***********************************************************************************************************/
/*    AES Key Output 6 Register (AES_KEY_OUT6)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT6                         (TIP_SEC_BASE_ADDR + 0x6C),  MEM,    16         /* Offset: 6Ch */
#define AES_KEY_OUT6_AES_KEY_OUT6             0,    16               /* 0-15 AES_KEY_OUT6 (AES Key Output 6) */

/***********************************************************************************************************/
/*    AES Key Output 7 Register (AES_KEY_OUT7)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT7                         (TIP_SEC_BASE_ADDR + 0x6E),  MEM,    16         /* Offset: 6Eh */
#define AES_KEY_OUT7_AES_KEY_OUT7             0,    16               /* 0-15 AES_KEY_OUT7 (AES Key Output 7) */

/***********************************************************************************************************/
/*    AES Key Output 8 Register (AES_KEY_OUT8)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT8                         (TIP_SEC_BASE_ADDR + 0x70),  MEM,    16         /* Offset: 70h */
#define AES_KEY_OUT8_AES_KEY_OUT8             0,    16               /* 0-15 AES_KEY_OUT8 (AES Key Output 8) */

/***********************************************************************************************************/
/*    AES Key Output 9 Register (AES_KEY_OUT9)                                                             */
/***********************************************************************************************************/
#define AES_KEY_OUT9                         (TIP_SEC_BASE_ADDR + 0x72),  MEM,    16         /* Offset: 72h */
#define AES_KEY_OUT9_AES_KEY_OUT9             0,    16               /* 0-15 AES_KEY_OUT9 (AES Key Output 9) */

/***********************************************************************************************************/
/*    AES Key Output 10 Register (AES_KEY_OUT10)                                                           */
/***********************************************************************************************************/
#define AES_KEY_OUT10                        (TIP_SEC_BASE_ADDR + 0x74),  MEM,    16         /* Offset: 74h */
#define AES_KEY_OUT10_AES_KEY_OUT10           0,    16               /* 0-15 AES_KEY_OUT10 (AES Key Output 10) */

/***********************************************************************************************************/
/*    AES Key Output 11 Register (AES_KEY_OUT11)                                                           */
/***********************************************************************************************************/
#define AES_KEY_OUT11                        (TIP_SEC_BASE_ADDR + 0x76),  MEM,    16         /* Offset: 76h */
#define AES_KEY_OUT11_AES_KEY_OUT11           0,    16               /* 0-15 AES_KEY_OUT11 (AES Key Output 11) */

/***********************************************************************************************************/
/*    AES Key Output 12 Register (AES_KEY_OUT12)                                                           */
/***********************************************************************************************************/
#define AES_KEY_OUT12                        (TIP_SEC_BASE_ADDR + 0x78),  MEM,    16         /* Offset: 78h */
#define AES_KEY_OUT12_AES_KEY_OUT12           0,    16               /* 0-15 AES_KEY_OUT12 (AES Key Output 12) */

/***********************************************************************************************************/
/*    AES Key Output 13 Register (AES_KEY_OUT13)                                                           */
/***********************************************************************************************************/
#define AES_KEY_OUT13                        (TIP_SEC_BASE_ADDR + 0x7A),  MEM,    16         /* Offset: 7Ah */
#define AES_KEY_OUT13_AES_KEY_OUT13           0,    16               /* 0-15 AES_KEY_OUT13 (AES Key Output 13) */

/***********************************************************************************************************/
/*    AES Key Output 14 Register (AES_KEY_OUT14)                                                           */
/***********************************************************************************************************/
#define AES_KEY_OUT14                        (TIP_SEC_BASE_ADDR + 0x7C),  MEM,    16         /* Offset: 7Ch */
#define AES_KEY_OUT14_AES_KEY_OUT14           0,    16               /* 0-15 AES_KEY_OUT14 (AES Key Output 14) */

/***********************************************************************************************************/
/*    AES Key Output 15 Register (AES_KEY_OUT15)                                                           */
/***********************************************************************************************************/
#define AES_KEY_OUT15                        (TIP_SEC_BASE_ADDR + 0x7E),  MEM,    16         /* Offset: 7Eh */
#define AES_KEY_OUT15_AES_KEY_OUT15           0,    16               /* 0-15 AES_KEY_OUT15 (None) */

#include "../npcm850/npcm850_regs.h"

#endif // __NPCM850_TIP_REGS_H__

