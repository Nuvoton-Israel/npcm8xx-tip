/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2010-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   shm_regs.h                                                                                            */
/*            This file contains Shared Memory (SHM) module registers                                      */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef _SHM_REGS_H
#define _SHM_REGS_H

#include __CHIP_H_FROM_DRV()

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                    Shared Memory (SHM) Core Registers                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define ESHM_BASE_ADDR          (SHM_BASE_ADDR + 0x080)

#define SMC_STS                 (SHM_BASE_ADDR + 0x000), SHM_ACCESS, 8
#define SMC_CTL                 (SHM_BASE_ADDR + 0x001), SHM_ACCESS, 8
#define SHM_CTL                 (SHM_BASE_ADDR + 0x002), SHM_ACCESS, 8
#ifdef SHM_CAPABILITY_INDIRECT_MEMORY_ACCESS
#define IMA_WIN_SIZE            (SHM_BASE_ADDR + 0x005), SHM_ACCESS, 8
#endif
#define WIN_SIZE                (SHM_BASE_ADDR + 0x007), SHM_ACCESS, 8
#define SHAW_SEM(n)             (((n) < 2 ? (SHM_BASE_ADDR + (n)) : (ESHM_BASE_ADDR + (n-2))) + 0x008), SHM_ACCESS, 8
#ifdef SHM_CAPABILITY_INDIRECT_MEMORY_ACCESS
#define IMA_SEM                 (SHM_BASE_ADDR + 0x00B), SHM_ACCESS, 8
#endif
#define SHCFG                   (SHM_BASE_ADDR + 0x00E), SHM_ACCESS, 16
#ifdef SHM_CAPABILITY_EXTENDED_8_AREA_PROTECTION
#define WIN_WR_PROT(n)          (((n) < 2 ? (SHM_BASE_ADDR + ((n) * 2L)) : (ESHM_BASE_ADDR + ((n-2) * 2L))) + 0x010), SHM_ACCESS, 8
#define WIN_RD_PROT(n)          (((n) < 2 ? (SHM_BASE_ADDR + ((n) * 2L)) : (ESHM_BASE_ADDR + ((n-2) * 2L))) + 0x011), SHM_ACCESS, 8
#endif
#ifdef SHM_CAPABILITY_INDIRECT_MEMORY_ACCESS
#define IMA_WR_PROT             (SHM_BASE_ADDR + 0x016), SHM_ACCESS, 8
#define IMA_RD_PROT             (SHM_BASE_ADDR + 0x017), SHM_ACCESS, 8
#endif
#define WIN_BASE(n)             (((n) < 2 ? (SHM_BASE_ADDR + ((n) * 4L)) : (ESHM_BASE_ADDR + ((n-2) * 4L))) + 0x020), SHM_ACCESS, 32
#ifdef SHM_CAPABILITY_INDIRECT_MEMORY_ACCESS
#define IMA_BASE                (SHM_BASE_ADDR + 0x02C), SHM_ACCESS, 32
#endif
#define RST_CFG                 (SHM_BASE_ADDR + 0x03A), SHM_ACCESS, 8
#ifdef SHM_CAPABILITY_OFFSET
#define HOFS_STS                (SHM_BASE_ADDR + 0x048), SHM_ACCESS, 8
#define HOFS_CTL                (SHM_BASE_ADDR + 0x049), SHM_ACCESS, 8
#define COFS(n)                 (((n) < 2 ? (SHM_BASE_ADDR - ((n) * 2L)) : (ESHM_BASE_ADDR - ((n-2) * 2L))) + 0x04C), SHM_ACCESS, 16
#endif
#ifdef SHM_CAPABILITY_HOST_OFFSET
#define IHOFS(n)                (((n) < 2 ? (SHM_BASE_ADDR - ((n) * 2L)) : (ESHM_BASE_ADDR - ((n-2) * 2L))) + 0x052), SHM_ACCESS, 16
#endif
#ifdef SHM_CAPABILITY_VERSION
#define SHM_VER                 (SHM_BASE_ADDR + 0x07F), SHM_ACCESS, 8
#endif
#ifdef SHM_CAPABILITY_EXTENDED_SHM
#define SMCE_STS                (ESHM_BASE_ADDR + 0x000), SHM_ACCESS, 8
#define SMCE_CTL                (ESHM_BASE_ADDR + 0x001), SHM_ACCESS, 8
#define WINE_SIZE               (ESHM_BASE_ADDR + 0x007), SHM_ACCESS, 8
#define HOFSE_STS               (ESHM_BASE_ADDR + 0x048), SHM_ACCESS, 8
#define HOFSE_CTL               (ESHM_BASE_ADDR + 0x049), SHM_ACCESS, 8
#endif

#ifdef SHM_CAPABILITY_HW_HOST_EVENT_WIN_3_4
#define SHM_HOST_EV_BASE_ADDR   0x100FE000
/* n = 2 or 3                                                                                              */
#define HEVn_STS(n)             (SHM_HOST_EV_BASE_ADDR + (((n) - 2) * 0x10) + 0x000), SHM_ACCESS, 8
#define HEVn_EV(n)              (SHM_HOST_EV_BASE_ADDR + (((n) - 2) * 0x10) + 0x001), SHM_ACCESS, 8
#define HEVn_IEN(n)             (SHM_HOST_EV_BASE_ADDR + (((n) - 2) * 0x10) + 0x002), SHM_ACCESS, 8
#define HEVn_SEN(n)             (SHM_HOST_EV_BASE_ADDR + (((n) - 2) * 0x10) + 0x003), SHM_ACCESS, 8
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                    Shared Memory (SHM) Host Registers                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifdef SHM_CAPABILITY_HW_HOST_EVENT_WIN_3_4
/* n = 2 or 3                                                                                              */
#define HOST_HEVn_STS(n)        (SHM_WIN_BASE_ADDR(n) + 0x000), SHM_HOST_ACCESS, 8
#define HOST_HEVn_EV(n)         (SHM_WIN_BASE_ADDR(n) + 0x001), SHM_HOST_ACCESS, 8
#define HOST_HEVn_IEN(n)        (SHM_WIN_BASE_ADDR(n) + 0x002), SHM_HOST_ACCESS, 8
#define HOST_HEVn_SEN(n)        (SHM_WIN_BASE_ADDR(n) + 0x003), SHM_HOST_ACCESS, 8
#endif

#define SEM_CTS                 SIB_ADDRESS(SIB_SMEM_DEV, 0x0A), SHM_HOST_ACCESS, 8
#ifdef SHM_CAPABILITY_EXTENDED_SHM
#define SEME_CTS                SIB_ADDRESS(SIB_ESHM_DEV, 0x0A), SHM_HOST_ACCESS, 8
#endif

#ifdef SHM_CAPABILITY_OFFSET
#ifdef SHM_CAPABILITY_REMAP_HOST_REGS
#define HOST_HOFS2L             SIB_ADDRESS(SIB_SMEM_DEV, 0x10), SHM_HOST_ACCESS, 8
#define HOST_HOFS2H             SIB_ADDRESS(SIB_SMEM_DEV, 0x11), SHM_HOST_ACCESS, 8
#define HOST_HOFS1L             SIB_ADDRESS(SIB_SMEM_DEV, 0x12), SHM_HOST_ACCESS, 8
#define HOST_HOFS1H             SIB_ADDRESS(SIB_SMEM_DEV, 0x13), SHM_HOST_ACCESS, 8
#define HOST_COFS_CTL           SIB_ADDRESS(SIB_SMEM_DEV, 0x0F), SHM_HOST_ACCESS, 8
#else
#define HOST_HOFS2L             SIB_ADDRESS(SIB_SMEM_DEV, 0x24), SHM_HOST_ACCESS, 8
#define HOST_HOFS2H             SIB_ADDRESS(SIB_SMEM_DEV, 0x25), SHM_HOST_ACCESS, 8
#define HOST_HOFS1L             SIB_ADDRESS(SIB_SMEM_DEV, 0x26), SHM_HOST_ACCESS, 8
#define HOST_HOFS1H             SIB_ADDRESS(SIB_SMEM_DEV, 0x27), SHM_HOST_ACCESS, 8
#define HOST_COFS_CTL           SIB_ADDRESS(SIB_SMEM_DEV, 0x2F), SHM_HOST_ACCESS, 8
#endif
#ifdef SHM_CAPABILITY_EXTENDED_SHM
#define HOST_HOFS4L             SIB_ADDRESS(SIB_ESHM_DEV, 0x10), SHM_HOST_ACCESS, 8
#define HOST_HOFS4H             SIB_ADDRESS(SIB_ESHM_DEV, 0x11), SHM_HOST_ACCESS, 8
#define HOST_HOFS3L             SIB_ADDRESS(SIB_ESHM_DEV, 0x12), SHM_HOST_ACCESS, 8
#define HOST_HOFS3H             SIB_ADDRESS(SIB_ESHM_DEV, 0x13), SHM_HOST_ACCESS, 8
#define HOST_COFSE_CTL          SIB_ADDRESS(SIB_ESHM_DEV, 0x0F), SHM_HOST_ACCESS, 8
#endif
#endif


/*---------------------------------------------------------------------------------------------------------*/
/* SMC_STS register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMC_STS_HRERR           0,  1
#define SMC_STS_HWERR           1,  1
#define SMC_STS_HSEM_IMAW       3,  1
#define SMC_STS_HSEM1W          4,  1
#define SMC_STS_HSEM2W          5,  1
#define SMC_STS_SHM_ACC         6,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMC_CTL register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SMC_CTL_HERES           0,  2   /* Bits 0-1: Host Error Response.                                  */
#define SMC_CTL_HERR_IE         2,  1   /* Bit    2: Enable Interrupt by Host Access Errors.               */
#define SMC_CTL_HSEM1_IE        3,  1   /* Bit    3: Enable Interrupt by Host Semaphore 1 Written.         */
#define SMC_CTL_HSEM2_IE        4,  1   /* Bit    4: Enable Interrupt by Host Semaphore 2 Written.         */
#define SMC_CTL_ACC_IE          5,  1   /* Bit    5: Enable Interrupt by Host Access to Shared Memory.     */
#define SMC_CTL_HSEM_IMA_IE     6,  1   /* Bit    6: Enable Interrupt by Host Indirect Memory Access Writ. */
#define SMC_CTL_HOSTWAIT        7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SHM_CTL register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SHM_CTL_STALL_HOST      6,  1

#ifdef SHM_CAPABILITY_INDIRECT_MEMORY_ACCESS
/*---------------------------------------------------------------------------------------------------------*/
/* IMA_WIN_SIZE register values                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define IMA_WIN_SIZE_IMA_SIZE   0,  4
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* WIN_SIZE register values                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define WIN_SIZE_RWIN_SIZE(win_num) (win_num * 4), 4

/*---------------------------------------------------------------------------------------------------------*/
/* SHAW1_SEM, SHAW2_SEM register values                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define SHAWx_SEM_HSEM          0,  4
#define SHAWx_SEM_CSEM          4,  4

#ifdef SHM_CAPABILITY_INDIRECT_MEMORY_ACCESS
/*---------------------------------------------------------------------------------------------------------*/
/* IMA_SEM register values                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define IMA_SEM_HSEM            0,  4
#define IMA_SEM_CSEM            4,  4
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* SHCFG register fields                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define SHCFG_IMAEN             0,  1

#ifdef SHM_CAPABILITY_EXTENDED_8_AREA_PROTECTION

/*---------------------------------------------------------------------------------------------------------*/
/* WIN1_WR_PROT register values                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define WIN1_WR_PROT_WR1_WP(n)  n,  1

/*---------------------------------------------------------------------------------------------------------*/
/* WIN1_RD_PROT register values                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define WIN1_RD_PROT_WR1_RP(n)  n,  1

/*---------------------------------------------------------------------------------------------------------*/
/* WIN2_WR_PROT register values                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define WIN2_WR_PROT_WR2_WP(n)  n,  1

/*---------------------------------------------------------------------------------------------------------*/
/* WIN2_RD_PROT register values                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define WIN2_RD_PROT_WR2_RP(n)  n,  1

/*---------------------------------------------------------------------------------------------------------*/
/* Genral macro for protection bits for WIN1/WIN2/IAM                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define WINx_WR_PROT_WP(n)      n,  1
#define WINx_RD_PROT_RP(n)      n,  1

#endif

#ifdef SHM_CAPABILITY_INDIRECT_MEMORY_ACCESS

/*---------------------------------------------------------------------------------------------------------*/
/* IMA_WR_PROT register values                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define IMA_WR_PROT_IMA_WP(n)   n,  1

/*---------------------------------------------------------------------------------------------------------*/
/* IMA_RD_PROT register values                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define IMA_RD_PROT_IAM_RP(n)   n,  1

#endif

/*---------------------------------------------------------------------------------------------------------*/
/* RST_CFG register fields                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define RST_CFG_LRESET_DIS      7,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SEM_CTS register values                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define SEM_CTS_CSEM1W          0,  1
#define SEM_CTS_CSEM2W          1,  1
#ifdef SHM_CAPABILITY_SEMAPHORE_SMI_GENERATION
#define SEM_CTS_CSEM1_IE        2,  1
#define SEM_CTS_CSEM2_IE        3,  1
#define SEM_CTS_CSEM1_SE        4,  1
#define SEM_CTS_CSEM2_SE        5,  1
#endif
#define SEM_CTS_RWIN1_SEM       6,  1
#define SEM_CTS_RWIN2_SEM       7,  1

#ifdef SHM_CAPABILITY_OFFSET
/*---------------------------------------------------------------------------------------------------------*/
/* HOFS_STS register values                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define HOFS_STS_HOFS1RW        0,  2
#define HOFS_STS_HOFS2RW        2,  2

/*---------------------------------------------------------------------------------------------------------*/
/* HOFS_CTL register values                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define HOFS_CTL_HOFS1RW_IE     0,  2
#define HOFS_CTL_HOFS2RW_IE     2,  2

/*---------------------------------------------------------------------------------------------------------*/
/* COFS_CTL Host register values                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define COFS_CTL_COFS1R_IE      0,  1
#define COFS_CTL_COFS1W_IE      1,  1
#define COFS_CTL_COFS2R_IE      2,  1
#define COFS_CTL_COFS2W_IE      3,  1
#define COFS_CTL_COFS1R_SE      4,  1
#define COFS_CTL_COFS1W_SE      5,  1
#define COFS_CTL_COFS2R_SE      6,  1
#define COFS_CTL_COFS2W_SE      7,  1
#endif

#ifdef SHM_CAPABILITY_EXTENDED_SHM
/*---------------------------------------------------------------------------------------------------------*/
/* SMCE_STS register fields                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define SMCE_STS_HSEM3W         4,  1
#define SMCE_STS_HSEM4W         5,  1

/*---------------------------------------------------------------------------------------------------------*/
/* SMCE_CTL register fields                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef SHM_CAPABILITY_HW_HOST_EVENT_WIN_3_4
#define SMCE_CTL_HEVREG3_EN     0,  1
#define SMCE_CTL_HEVREG4_EN     1,  1
#endif
#define SMCE_CTL_HSEM3_IE       3,  1
#define SMCE_CTL_HSEM4_IE       4,  1

/*---------------------------------------------------------------------------------------------------------*/
/* WINE_SIZE register values                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define WINE_SIZE_RWIN_SIZE(win_num) ((win_num-2) * 4), 4

/*---------------------------------------------------------------------------------------------------------*/
/* HOFSE_STS register values                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define HOFSE_STS_HOFS3RW       0,  2
#define HOFSE_STS_HOFS4RW       2,  2

/*---------------------------------------------------------------------------------------------------------*/
/* HOFSE_CTL register values                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define HOFSE_CTL_HOFS3RW_IE    0,  2
#define HOFSE_CTL_HOFS4RW_IE    2,  2

/*---------------------------------------------------------------------------------------------------------*/
/* SEME_CTS register values                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define SEME_CTS_CSEM3W         0,  1
#define SEME_CTS_CSEM4W         1,  1
#ifdef SHM_CAPABILITY_SEMAPHORE_SMI_GENERATION
#define SEME_CTS_CSEM3_IE       2,  1
#define SEME_CTS_CSEM4_IE       3,  1
#define SEME_CTS_CSEM3_SE       4,  1
#define SEME_CTS_CSEM4_SE       5,  1
#endif
#define SEME_CTS_RWIN3_SEM      6,  1
#define SEME_CTS_RWIN4_SEM      7,  1
#endif  /* SHM_CAPABILITY_EXTENDED_SHM */

#endif /* _SHM_REGS_H */

