/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2010-2019 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   smb_regs.h                                                                                            */
/*            This file contains System Management Bus (SMB) module registers                              */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef _SMB_REGS_H
#define _SMB_REGS_H

#include __CHIP_H_FROM_DRV()

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                  System Management Bus (SMB) Registers                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Common registers                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBSDA(n)               (SMB_BASE_ADDR(n) + 0x000), SMB_ACCESS, 8
#define SMBST(n)                (SMB_BASE_ADDR(n) + 0x002), SMB_ACCESS, 8
#define SMBCST(n)               (SMB_BASE_ADDR(n) + 0x004), SMB_ACCESS, 8
#define SMBCTL1(n)              (SMB_BASE_ADDR(n) + 0x006), SMB_ACCESS, 8
#define SMBADDR1(n)             (SMB_BASE_ADDR(n) + 0x008), SMB_ACCESS, 8
#define SMBCTL2(n)              (SMB_BASE_ADDR(n) + 0x00A), SMB_ACCESS, 8
#define SMBADDR2(n)             (SMB_BASE_ADDR(n) + 0x00C), SMB_ACCESS, 8
#define SMBCTL3(n)              (SMB_BASE_ADDR(n) + 0x00E), SMB_ACCESS, 8
#define SMBCST2(n)              (SMB_BASE_ADDR(n) + 0x018), SMB_ACCESS, 8
#define SMBCST3(n)              (SMB_BASE_ADDR(n) + 0x019), SMB_ACCESS, 8
#define SMB_VER(n)              (SMB_BASE_ADDR(n) + 0x01F), SMB_ACCESS, 8

/*---------------------------------------------------------------------------------------------------------*/
/* BANK 0 registers                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBADDR3(n)             (SMB_BASE_ADDR(n) + 0x010), SMB_ACCESS, 8
#define SMBADDR7(n)             (SMB_BASE_ADDR(n) + 0x011), SMB_ACCESS, 8
#define SMBADDR4(n)             (SMB_BASE_ADDR(n) + 0x012), SMB_ACCESS, 8
#define SMBADDR8(n)             (SMB_BASE_ADDR(n) + 0x013), SMB_ACCESS, 8
#define SMBADDR5(n)             (SMB_BASE_ADDR(n) + 0x014), SMB_ACCESS, 8
#define SMBADDR9(n)             (SMB_BASE_ADDR(n) + 0x015), SMB_ACCESS, 8
#define SMBADDR6(n)             (SMB_BASE_ADDR(n) + 0x016), SMB_ACCESS, 8
#define SMBADDR10(n)            (SMB_BASE_ADDR(n) + 0x017), SMB_ACCESS, 8
#define SMBADDR(n, i)           (SMB_BASE_ADDR(n) + 0x008 + (UINT32)(((int)i*4) +\
                                (((int)i < 2) ? 0 : ((int)i-2)*(-2)) + (((int)i < 6) ? 0 : (-7)))), SMB_ACCESS, 8
#define SMBCTL4(n)              (SMB_BASE_ADDR(n) + 0x01A), SMB_ACCESS, 8
#define SMBCTL5(n)              (SMB_BASE_ADDR(n) + 0x01B), SMB_ACCESS, 8
#define SMBSCLLT(n)             (SMB_BASE_ADDR(n) + 0x01C), SMB_ACCESS, 8  // SMB SCL Low Time (Fast-Mode)
#define SMBFIF_CTL(n)           (SMB_BASE_ADDR(n) + 0x01D), SMB_ACCESS, 8  // FIFO Control
#define SMBSCLHT(n)             (SMB_BASE_ADDR(n) + 0x01E), SMB_ACCESS, 8  // SMB SCL High Time (Fast-Mode)

/*---------------------------------------------------------------------------------------------------------*/
/* BANK 1 registers                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBFIF_CTS(n)           (SMB_BASE_ADDR(n) + 0x010), SMB_ACCESS, 8  // FIFO Control and Status
#define SMBTXF_CTL(n)           (SMB_BASE_ADDR(n) + 0x012), SMB_ACCESS, 8  // Tx-FIFO Control
#ifdef SMB_CAPABILITY_START_TO_STOP_TIMEOUT_SUPPORT
#define SMB_FRTO(n)             (SMB_BASE_ADDR(n) + 0x013), SMB_ACCESS, 8  // SMB Frame Timeout
#endif
#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
#define SMBT_OUT(n)             (SMB_BASE_ADDR(n) + ((SMB_status[n].fifo_use) ? 0x014 : 0x00F)), SMB_ACCESS, 8  // Bus Time-Out
#endif
#ifdef SMB_CAPABILITY_HW_PEC_SUPPORT
#define SMBPEC(n)               (SMB_BASE_ADDR(n) + 0x016), SMB_ACCESS, 8  // PEC Data
#endif
#define SMBTXF_STS(n)           (SMB_BASE_ADDR(n) + 0x01A), SMB_ACCESS, 8  // Tx-FIFO Status
#define SMBRXF_STS(n)           (SMB_BASE_ADDR(n) + 0x01C), SMB_ACCESS, 8  // Rx-FIFO Status
#define SMBRXF_CTL(n)           (SMB_BASE_ADDR(n) + 0x01E), SMB_ACCESS, 8  // Rx-FIFO Control

#ifdef SMB_CAPABILITY_WAKEUP_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* GLUE registers                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define SMB_SBD                 (GLUE_BASE_ADDR + 0x002), GLUE_ACCESS, 8
#define SMB_EEN                 (GLUE_BASE_ADDR + 0x003), GLUE_ACCESS, 8
#endif


/*---------------------------------------------------------------------------------------------------------*/
/* SMBST register fields                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBST_XMIT              0,  1
#define SMBST_MASTER            1,  1
#define SMBST_NMATCH            2,  1
#define SMBST_STASTR            3,  1
#define SMBST_NEGACK            4,  1
#define SMBST_BER               5,  1
#define SMBST_SDAST             6,  1
#define SMBST_SLVSTP            7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBCST register fields                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBCST_BUSY             0,  1
#define SMBCST_BB               1,  1
#define SMBCST_MATCH            2,  1
#define SMBCST_GCMATCH          3,  1
#define SMBCST_TSDA             4,  1
#define SMBCST_TGSCL            5,  1
#define SMBCST_MATCHAF          6,  1
#define SMBCST_ARPMATCH         7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBCTL1 register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBCTL1_START           0,  1
#define SMBCTL1_STOP            1,  1
#define SMBCTL1_INTEN           2,  1
#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
#define SMBCTL1_EOBINTE         3,  1
#endif
#define SMBCTL1_ACK             4,  1
#define SMBCTL1_GCMEN           5,  1
#define SMBCTL1_NMINTE          6,  1
#define SMBCTL1_STASTRE         7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBADDRx register fields                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBADDRx_ADDR           0,  7
#define SMBADDRx_SAEN           7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBCTL2 register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBCTL2_ENABLE          0,  1
#define SMBCTL2_SCLFRQ6_0       1,  7

/*---------------------------------------------------------------------------------------------------------*/
/* SMBCTL3 register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBCTL3_SCLFRQ8_7       0,  2
#define SMBCTL3_ARPMEN          2,  1
#define SMBCTL3_IDL_START       3,  1
#define SMBCTL3_400K_MODE       4,  1
#define SMBCTL3_BNK_SEL         5,  1
#define SMBCTL3_SDA_LVL         6,  1
#define SMBCTL3_SCL_LVL         7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBCST2 register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBCST2_MATCHA1F        0,  1
#define SMBCST2_MATCHA2F        1,  1
#define SMBCST2_MATCHA3F        2,  1
#define SMBCST2_MATCHA4F        3,  1
#define SMBCST2_MATCHA5F        4,  1
#define SMBCST2_MATCHA6F        5,  1
#define SMBCST2_MATCHA7F        6,  1
#define SMBCST2_INTSTS          7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBCST3 register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBCST3_MATCHA8F        0,  1
#define SMBCST3_MATCHA9F        1,  1
#define SMBCST3_MATCHA10F       2,  1
#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
#define SMBCST3_EO_BUSY         7,  1
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* SMBCTL4 register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBCTL4_HLDT            0,  6
#ifdef SMB_CAPABILITY_FORCE_SCL_SDA
#define SMBCTL4_LVL_WE          7,  1
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* SMBCTL5 register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBCTL5_DBNCT           0,  4

/*---------------------------------------------------------------------------------------------------------*/
/* SMBFIF_CTS register fields                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBFIF_CTS_RXF_TXE      1,  1
#define SMBFIF_CTS_RFTE_IE      3,  1
#define SMBFIF_CTS_CLR_FIFO     6,  1
#define SMBFIF_CTS_SLVRSTR      7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBTXF_CTL register fields                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef SMB_CAPABILITY_32B_FIFO
#define SMBTXF_CTL_TX_THR       0,  6
#else
#define SMBTXF_CTL_TX_THR       0,  5
#endif
#define SMBTXF_CTL_THR_TXIE     6,  1

#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* SMBT_OUT register fields                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBT_OUT_TO_CKDIV       0,  6
#define SMBT_OUT_T_OUTIE        6,  1
#define SMBT_OUT_T_OUTST        7,  1
#endif

#ifdef SMB_CAPABILITY_START_TO_STOP_TIMEOUT_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* SMB_FRTO register fields                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define SMB_FRTO_FR_LEN_TO      0,  6
#define SMB_FRTO_FRTOIE         6,  1
#define SMB_FRTO_FRTOST         7,  1
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* SMBTXF_STS register fields                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef SMB_CAPABILITY_32B_FIFO
#define SMBTXF_STS_TX_BYTES     0,  6
#else
#define SMBTXF_STS_TX_BYTES     0,  5
#endif
#define SMBTXF_STS_TX_THST      6,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBRXF_STS register fields                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef SMB_CAPABILITY_32B_FIFO
#define SMBRXF_STS_RX_BYTES     0,  6
#else
#define SMBRXF_STS_RX_BYTES     0,  5
#endif
#define SMBRXF_STS_RX_THST      6,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBFIF_CTL register fields                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBFIF_CTL_FIFO_EN      4,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMBRXF_CTL register fields                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef SMB_CAPABILITY_32B_FIFO
#define SMBRXF_CTL_RX_THR       0,  6
#define SMBRXF_CTL_THR_RXIE     6,  1
#define SMBRXF_CTL_LAST_PEC     7,  1
#else
#define SMBRXF_CTL_RX_THR       0,  5
#define SMBRXF_CTL_LAST_PEC     5,  1
#define SMBRXF_CTL_THR_RXIE     6,  1
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* SMB_VER register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMB_VER_VERSION         0,  7
#define SMB_VER_FIFO_EN         7,  1

#ifdef SMB_CAPABILITY_WAKEUP_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* SMB_SBD register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMB_SBD_SMBnSBD(n)      (n),  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMB_EEN register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMB_EEN_SMBnEEN(n)      (n),  1
#endif // SMB_CAPABILITY_WAKEUP_SUPPORT

#endif /* _SMB_REGS_H */

