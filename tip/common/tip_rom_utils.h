/*
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2023 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 *<<<---------------------------------------------------------------------------------
 * File Contents:
 *   tip_rom_utils.h
 *            This file contains TIP Rom definitions and declerations
 * Project:
 *            Arbel
 */

#ifndef __TIP_ROM_UTILS_H__
#define __TIP_ROM_UTILS_H__

#include "hal_regs.h"

#define ECC_KEY_SIZE  128/* 128 bytes per ecc key */
#define LMS_KEY_SIZE  80 /* 80 bytes per key */

/* Types & Definitions */

#define TIP_SECBOOT_IS_INACTIVE()                   (READ_REG_FIELD(TIP_SEC_CTL, TIP_SEC_CTL_TIP_SECBOOT) == (UINT)TIP_SEC_FALSE)
#define TIP_SECBOOT_IS_ACTIVE()                     (!TIP_SECBOOT_IS_INACTIVE())

#define TIP_CORE_RESET_INDICATION()                 (READ_REG_MASK(TIP_CTL_STS, MASK_FIELD(TIP_CTL_STS_TIP_SW_RST) | MASK_FIELD(TIP_CTL_STS_DBGRST_STS)) || READ_REG_FIELD(T0CSR, T0CSR_WDRST_STS))
#define BMC_CORE_RESET_INDICATION()                 READ_REG_MASK(RESSR, ~MASK_FIELD(RESSR_PORST))
#define CORE_RESET_INDICATION()                     (BMC_CORE_RESET_INDICATION() || TIP_CORE_RESET_INDICATION())
#define PWR_UP_RESET_OCCURED()                      READ_REG_FIELD(RESSR, RESSR_PORST)

#define FCFGn_BLOCK_MASK(block)                     (BUILD_FIELD_VAL(FCFG0_7_FRDLK, MASK_BIT((block) % 8))      |   \
                                                     BUILD_FIELD_VAL(FCFG0_7_FPRGDIS, MASK_BIT((block) % 8))    |   \
                                                     BUILD_FIELD_VAL(FCFG0_7_FCFGLK, MASK_BIT((block) % 8)))

#define BMC_FCFG_BLOCK_IS_LOCKED(block)             ((REG_READ(BMC_FCFG0_7((block) / 8)) & FCFGn_BLOCK_MASK(block)) == FCFGn_BLOCK_MASK(block))
#define TIP_FCFG_BLOCK_IS_LOCKED(block)             ((REG_READ(TIP_FCFG0_7((block) / 8)) & FCFGn_BLOCK_MASK(block)) == FCFGn_BLOCK_MASK(block))

#define FCFG_BLOCK_IS_LOCKED(block)                 (BMC_FCFG_BLOCK_IS_LOCKED(block) && TIP_FCFG_BLOCK_IS_LOCKED(block))

#define TIP_FCFG0_7(n)                              FCFG0_7(n)
#define TIP_FCFG_LOCK_MASK(n, r, w, l)              REG_WRITE(  TIP_FCFG0_7(n), REG_READ(TIP_FCFG0_7(n))    |   \
                                                                BUILD_FIELD_VAL(FCFG0_7_FRDLK, (r))         |   \
                                                                BUILD_FIELD_VAL(FCFG0_7_FPRGDIS, (w))       |   \
                                                                BUILD_FIELD_VAL(FCFG0_7_FCFGLK, (l)))

#define BMC_FCFG0_7(n)                              (FUSE_PHYS_BASE_ADDR + (0x30 + (4 * (n)))),  MEM,    32
#define BMC_FCFG_LOCK_MASK(n, r, w, l)              REG_WRITE(  BMC_FCFG0_7(n), REG_READ(BMC_FCFG0_7(n))    |   \
                                                                BUILD_FIELD_VAL(FCFG0_7_FRDLK, (r))         |   \
                                                                BUILD_FIELD_VAL(FCFG0_7_FPRGDIS, (w))       |   \
                                                                BUILD_FIELD_VAL(FCFG0_7_FCFGLK, (l)))

#define FCFG_LOCK_ACCESS_MASK(n, r, w, l)           {                                       \
                                                        DEFS_SEC_RUN_TWICE(TIP_FCFG_LOCK_MASK(n, r, w, l));     \
                                                        DEFS_SEC_RUN_TWICE(BMC_FCFG_LOCK_MASK(n, r, w, l));     \
                                                    }

#define FCFG_LOCK_BLOCK(block)                      {                                                                                                   \
                                                        DEFS_SEC_RUN_TWICE(REG_WRITE(TIP_FCFG0_7((block)/8), REG_READ(TIP_FCFG0_7((block)/8)) | FCFGn_BLOCK_MASK(block)));  \
                                                        DEFS_SEC_RUN_TWICE(REG_WRITE(BMC_FCFG0_7((block)/8), REG_READ(BMC_FCFG0_7((block)/8)) | FCFGn_BLOCK_MASK(block)));  \
                                                    }
#endif  // __TIP_ROM_UTILS_H__
