/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2020 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*                         fuse_wrapper.h                                                                  */
/*            This file contains fuse wrapper implementation. it wraps all access to the otp.              */
/*            For the user: call FUSE_WRPR_set or FUSE_WRPR_get with the property fields.                  */
/*  Project:  Arbel                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

#if defined (FUSE_MODULE_TYPE)
#ifndef _FUSE_WRAPPER_
#define _FUSE_WRAPPER_

#include "defs.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define FUSE_WRAPPER_NUM_OF_ECC_KEYS            9

#define _FUSE_WRPR_PROP_ADDRESS(addr, len, ecc)   addr
#define _FUSE_WRPR_PROP_SIZE(addr, len, ecc)      len
#define _FUSE_WRPR_PROP_ECC(addr, len, ecc)       ecc
#define _FUSE_WRPR_DEC_SIZE(addr,len,ecc)       ((ecc) == FUSE_ECC_MAJORITY ? ((len) / 3) :             \
                                                ((ecc) == FUSE_ECC_NIBBLE_PARITY ? ((len) / 2) :        \
                                                ((ecc) == FUSE_ECC_64_72 ? ((len) - (len) / 8) : (len))))
#define _FUSE_WRPR_PROP_BLOCK(addr,len,ecc)     ((addr) >= FUSE_BLOCK58_OFFSET ? (58 + ((addr) - FUSE_BLOCK58_OFFSET) / 256) :  \
                                                 (addr) >= FUSE_BLOCK10_OFFSET ? (10 + ((addr) - FUSE_BLOCK10_OFFSET) / 128) :  \
                                                 (addr) >= FUSE_BLOCK5_OFFSET  ? (5  + ((addr) - FUSE_BLOCK5_OFFSET ) / 64)  :  \
                                                 (addr) >= FUSE_BLOCK4_OFFSET  ? 4  :                                           \
                                                 (addr) >= FUSE_BLOCK3_OFFSET  ? 3  :                                           \
                                                 (addr) >= FUSE_BLOCK2_OFFSET  ? 2  :                                           \
                                                 (addr) >= FUSE_BLOCK1_OFFSET  ? 1  : 0)
#define FUSE_WRPR_BLOCK_ADDR(block)             ((block) >= 58 ? (FUSE_BLOCK58_OFFSET + ((block) - 58) * 256) : \
                                                 (block) >= 10 ? (FUSE_BLOCK10_OFFSET + ((block) - 10) * 128) : \
                                                 (block) >= 5  ? (FUSE_BLOCK5_OFFSET  + ((block) - 5)  * 64)  : \
                                                 (block) >= 4  ? (FUSE_BLOCK4_OFFSET) :                         \
                                                 (block) >= 3  ? (FUSE_BLOCK3_OFFSET) :                         \
                                                 (block) >= 2  ? (FUSE_BLOCK2_OFFSET) :                         \
                                                 (block) >= 1  ? (FUSE_BLOCK1_OFFSET) : 0)
#define FUSE_WRPR_BLOCK_LEN(block)              ((block) >= 58 ? 256 :  \
                                                 (block) >= 10 ? 128 :  \
                                                 (block) >= 5  ? 64  :  \
                                                 (block) >= 4  ? 32  :  \
                                                 (block) >= 3  ? 104 :  \
                                                 (block) >= 2  ? 24  :  \
                                                 (block) >= 1  ? 1   : 31)
#define FUSE_WRPR_PROP_ADDRESS(prop)            (_FUSE_WRPR_PROP_ADDRESS(prop))
#define FUSE_WRPR_PROP_SIZE(prop)               (_FUSE_WRPR_PROP_SIZE(prop))
#define FUSE_WRPR_PROP_DEC_SIZE(prop)           (_FUSE_WRPR_DEC_SIZE(prop))
#define FUSE_WRPR_PROP_BLOCK(prop)              (_FUSE_WRPR_PROP_BLOCK(prop))

#define FUSE_WRPR_PROP_BLOCK_ADDR(addr)     ((addr) >= FUSE_BLOCK58_OFFSET ? (58 + ((addr) - FUSE_BLOCK58_OFFSET) / 256) :  \
                                                 (addr) >= FUSE_BLOCK10_OFFSET ? (10 + ((addr) - FUSE_BLOCK10_OFFSET) / 128) :  \
                                                 (addr) >= FUSE_BLOCK5_OFFSET  ? (5  + ((addr) - FUSE_BLOCK5_OFFSET ) / 64)  :  \
                                                 (addr) >= FUSE_BLOCK4_OFFSET  ? 4  :                                           \
                                                 (addr) >= FUSE_BLOCK3_OFFSET  ? 3  :                                           \
                                                 (addr) >= FUSE_BLOCK2_OFFSET  ? 2  :                                           \
                                                 (addr) >= FUSE_BLOCK1_OFFSET  ? 1  : 0)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           FUSE ARRAY ELEMENTS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define FUSE_BLOCK0_OFFSET              0
#define FUSTRAP1_PROPERTY               0,          12,         FUSE_ECC_MAJORITY
#define CP_FUSTRAP_PROPERTY             12,         3,          FUSE_ECC_MAJORITY
#define FUSTRAP2_PROPERTY               16,         12,         FUSE_ECC_MAJORITY

#define FUSE_BLOCK1_OFFSET              31
#define LIFE_CYCLE_ENC_PROPERTY         31,         1,          FUSE_ECC_NONE

#define FUSE_BLOCK2_OFFSET              32

#define FUSE_BLOCK3_OFFSET              56

#define FUSE_BLOCK4_OFFSET              160
#define DIE_LOCATION_PROPERTY           168,        5,          FUSE_ECC_NONE
#define OEM_IDENTIFIER_CODE             173,        2,          FUSE_ECC_NIBBLE_PARITY
#define DIE_LOT_WEEK_NUMBER_PROPERTY    187,        2,          FUSE_ECC_NONE
#define DIE_WAFER_NUMBER_PROPERTY       189,        1,          FUSE_ECC_NONE
#define DIE_X_Y_YEAR_PROPERTY           190,        2,          FUSE_ECC_NONE

#define FUSE_BLOCK5_OFFSET              192
#define TIP_AES_KEY0_VALID_PROPERTY     200,        2,          FUSE_ECC_NIBBLE_PARITY

// BLOCKS 6-9
// BLOCKS 10-13
#define FUSE_BLOCK10_OFFSET             512
#define KMT_VERSION_PROPERTY            512,        128,        FUSE_ECC_NONE
#define SKMT_VERSION_PROPERTY           640,        128,        FUSE_ECC_NONE
#define TIP_FW_L1_VERSION_PROPERTY      768,        128,        FUSE_ECC_NONE
#define BMC_VERSION_PROPERTY            896,        128,        FUSE_ECC_NONE	/* shared by bootblock, BL31, OpTee, uboot */

#define FUSE_BLOCK31_OFFSET             3200
#define TFT_VERSION_PROPERTY            3200,       128,        FUSE_ECC_NONE	/* TIP_FW L0 */

// BLOCKS 32-39 (Authentication Keys 0-8)
#define oPKn_LENGTH                     (108)
#define oPKn_OFFSET(n)                  (3328 + (n) * 128)
#define oPKn_PROPERTY(n)                oPKn_OFFSET(n),       oPKn_LENGTH,    FUSE_ECC_64_72
#define oPKnVal_LENGTH                  (2)
#define oPKnVAL_PROPERTY(n)             oPKn_OFFSET(n) + 126, oPKnVal_LENGTH, FUSE_ECC_NIBBLE_PARITY

#define REG_CFG_PTR1_PROPERTY           5404,       2,          FUSE_ECC_NONE
#define REG_CFG_TABLE_PROPERTY          5410,       94,         FUSE_ECC_NONE

// BLOCKS 50-58 (Authentication LMS Keys 0-8)
#define FUSE_BLOCK58_OFFSET             6656
#define oLMS_PKn_LENGTH                 (63)
#define oLMS_PKn_OFFSET(n)              (5632 + (n) * 128)
#define oLMS_PKn_PROPERTY(n)            oLMS_PKn_OFFSET(n),       oLMS_PKn_LENGTH,    FUSE_ECC_64_72
#define oLMS_PKnVal_LENGTH              (2)
#define oLMS_PKnVAL_PROPERTY(n)         oLMS_PKn_OFFSET(n) + 126, oLMS_PKnVal_LENGTH, FUSE_ECC_NIBBLE_PARITY

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

DEFS_STATUS FUSE_WRPR_set            (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc, UINT8* value);
DEFS_STATUS FUSE_WRPR_get            (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc, UINT8* value);
DEFS_STATUS FUSE_WRPR_get_CP_Fustrap (UINT8 *value);
DEFS_STATUS FUSE_WRPR_EraseBlock     (UINT16 block);

#endif	// _FUSE_WRAPPER_


#endif	// #ifdef FUSE_MODULE_TYPE
