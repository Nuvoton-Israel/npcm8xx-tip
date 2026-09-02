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

#define FUSE_WRAPPER_ARRAY_SIZE                 8192
#define FUSE_WRAPPER_NUM_OF_BLOCKS              64
#define FUSE_WRAPPER_NUM_OF_ECC_KEYS            9
#define FUSE_WRAPPER_NUM_OF_AES_KEYS            4

#define _FUSE_WRPR_PROP_ADDRESS(addr,len,ecc)   addr
#define _FUSE_WRPR_PROP_SIZE(addr,len,ecc)      len
#define _FUSE_WRPR_PROP_ECC(addr,len,ecc)       ecc
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
#define SPARE_FLAGS_PROPERTY            28,         3,          FUSE_ECC_MAJORITY

#define FUSE_BLOCK1_OFFSET              31
#define LIFE_CYCLE_ENC_PROPERTY         31,         1,          FUSE_ECC_NONE

#define FUSE_BLOCK2_OFFSET              32
#define DAC_CAL_PROPERTY                32,         8,          FUSE_ECC_NIBBLE_PARITY
#define ADC_CAL_INT_PROPERTY            40,         8,          FUSE_ECC_NIBBLE_PARITY
#define ADC_CAL_EXT_PROPERTY            48,         8,          FUSE_ECC_NIBBLE_PARITY

#define FUSE_BLOCK3_OFFSET              56
#define ORP_PROPERTY                    112,        24,         FUSE_ECC_MAJORITY
#define OLP_PROPERTY                    136,        24,         FUSE_ECC_MAJORITY

#define FUSE_BLOCK4_OFFSET              160
#define DERIVATIVE_PROPERTY             160,        8,          FUSE_ECC_NIBBLE_PARITY
#define DIE_LOCATION_PROPERTY           168,        5,          FUSE_ECC_NONE
#define DEVICE_ID_PROPERTY              168,        5,          FUSE_ECC_NONE
#define OEM_IDENTIFIER_CODE             173,        2,          FUSE_ECC_NIBBLE_PARITY
#define RNG_CONFIGURATION_PROPERTY      175,        10,         FUSE_ECC_NIBBLE_PARITY
#define SIGMA_OTP_VERSION_PROPERTY      185,        2,          FUSE_ECC_NONE
#define DIE_INFORMATION_PROPERTY        187,        5,          FUSE_ECC_NONE
#define DIE_LOT_WEEK_NUMBER_PROPERTY    187,        2,          FUSE_ECC_NONE
#define DIE_WAFER_NUMBER_PROPERTY       189,        1,          FUSE_ECC_NONE
#define DIE_X_Y_YEAR_PROPERTY           190,        2,          FUSE_ECC_NONE

#define DIE_INFORMATION_ALL             168,        5,          (1/0, FUSE_ECC_NONE)    // this is deprecated use DEVICE_ID_PROPERTY
#define DIE_LOT_WEEK_NUMBER             168,        2,          (1/0, FUSE_ECC_NONE)    // this is deprecated use DIE_LOT_WEEK_NUMBER_PROPERTY
#define DIE_WAFER_NUMBER                170,        1,          (1/0, FUSE_ECC_NONE)    // this is deprecated use DIE_WAFER_NUMBER_PROPERTY
#define DIE_X_Y_COORDINATES_YEAR        171,        2,          (1/0, FUSE_ECC_NONE)    // this is deprecated use DIE_X_Y_YEAR_PROPERTY

#define FUSE_BLOCK5_OFFSET              192
#define DICE_UDS_VALID_PROPERTY         192,        2,          FUSE_ECC_NIBBLE_PARITY
#define DME0_UDS_VALID_PROPERTY         194,        2,          FUSE_ECC_NIBBLE_PARITY
#define DME1_UDS_VALID_PROPERTY         196,        2,          FUSE_ECC_NIBBLE_PARITY
#define DME2_UDS_VALID_PROPERTY         198,        2,          FUSE_ECC_NIBBLE_PARITY
#define DMEn_UDS_VALID_PROPERTY(n)      194 + 2 * (n), 4,       FUSE_ECC_NIBBLE_PARITY
#define TIP_AES_KEY0_VALID_PROPERTY     200,        2,          FUSE_ECC_NIBBLE_PARITY
#define TIP_AES_KEY1_VALID_PROPERTY     202,        2,          FUSE_ECC_NIBBLE_PARITY
#define SPI0_KMT_OFFSET_PROPERTY        210,        8,          FUSE_ECC_NIBBLE_PARITY
#define TIP_AES_FW_ENC_VERSION_PROPERTY 226,        4,          FUSE_ECC_NONE
#define TIP_AES_KEY0_VERSION_PROPERTY   230,        4,          FUSE_ECC_NONE
#define DME0_UDS_VERSION_PROPERTY       234,        4,          FUSE_ECC_NONE
#define DME1_UDS_VERSION_PROPERTY       238,        4,          FUSE_ECC_NONE
#define DME2_UDS_VERSION_PROPERTY       242,        4,          FUSE_ECC_NONE
#define DMEn_UDS_VERSION_PROPERTY(n)    234 + 4 * (n), 4,       FUSE_ECC_NONE

// BLOCKS 6-9
#define AESKEY0_PROPERTY                256,        64,         FUSE_ECC_NIBBLE_PARITY
#define AESKEY1_PROPERTY                320,        64,         FUSE_ECC_NIBBLE_PARITY
#define AESKEY2_PROPERTY                384,        64,         FUSE_ECC_NIBBLE_PARITY
#define AESKEY3_PROPERTY                448,        64,         FUSE_ECC_NIBBLE_PARITY

// BLOCKS 10-13
#define FUSE_BLOCK10_OFFSET             512
#define KMT_VERSION_PROPERTY            512,        128,        FUSE_ECC_NONE
#define SKMT_VERSION_PROPERTY           640,        128,        FUSE_ECC_NONE
#define TIP_FW_L1_VERSION_PROPERTY      768,        128,        FUSE_ECC_NONE
#define BMC_VERSION_PROPERTY            896,        128,        FUSE_ECC_NONE    /* shared by bootblock, BL31, OpTee, uboot */

#define FUSE_BLOCK14_OFFSET             1024
#define DICE_UDS_PROPERTY               1024,       128,        FUSE_ECC_NIBBLE_PARITY

#define FUSE_BLOCK15_OFFSET             1152
#define TIP_SPI0_FW_POINTER_PROPERTY    1152,       8,          FUSE_ECC_NIBBLE_PARITY
#define A35_MODE_VAL_PROPERTY           1160,       8,          FUSE_ECC_NIBBLE_PARITY
#define PQA_OTP_ERASE_PROPERTY          1168,       16,         FUSE_ECC_NONE
#define PQA_OTP_RD_PROTECT_PROPERTY     1184,       16,         FUSE_ECC_NONE
#define REVOCATION_MDLR_PROPERTY        1200,       8,          FUSE_ECC_NIBBLE_PARITY

#define FUSE_BLOCK16_OFFSET             1280
#define DME0_UDS_PROPERTY               1280,       128,        FUSE_ECC_NIBBLE_PARITY

#define FUSE_BLOCK17_OFFSET             1408
#define DME1_UDS_PROPERTY               1408,       128,        FUSE_ECC_NIBBLE_PARITY

#define FUSE_BLOCK18_OFFSET             1536
#define DME2_UDS_PROPERTY               1536,       128,        FUSE_ECC_NIBBLE_PARITY
#define DMEn_UDS_PROPERTY(n)            1280 + 128 * (n), 128,  FUSE_ECC_NIBBLE_PARITY

#define FUSE_BLOCK19_OFFSET             1664
#define TIP_AES_KEY0_PROPERTY           1664,       128,        FUSE_ECC_NIBBLE_PARITY

#define FUSE_BLOCK20_OFFSET             1792
#define TIP_AES_KEY1_PROPERTY           1792,       64,         FUSE_ECC_NIBBLE_PARITY

// BLOCKS 20-30 (General Purpose 20-30)
#define GP_20_30_LENGTH                 (128)
#define GP_20_30_OFFSET(n)              (1792 + (n) * GP_20_30_LENGTH)
#define GP_20_30_PROPERTY(n)            GP_20_30_OFFSET(n), 128, FUSE_ECC_NONE

#define FUSE_BLOCK31_OFFSET             3200
#define TFT_VERSION_PROPERTY            3200,       128,        FUSE_ECC_NONE    /* TIP_FW L0 */

// BLOCKS 32-39 (Authentication Keys 0-7)
#define oPKn_LENGTH                     (108)
#define oPKn_OFFSET(n)                  (3328 + (n) * 128)
#define oPKn_PROPERTY(n)                oPKn_OFFSET(n),       oPKn_LENGTH,    FUSE_ECC_64_72
#define oPKnVal_LENGTH                  (2)
#define oPKnVAL_PROPERTY(n)             oPKn_OFFSET(n) + 126, oPKnVal_LENGTH, FUSE_ECC_NIBBLE_PARITY

#define FUSE_BLOCK48_OFFSET             5376
#define VOTP_VDD_CAL_PROPERTY           5376,       28,         FUSE_ECC_NONE
#define REG_CFG_PTR1_PROPERTY           5404,       2,          FUSE_ECC_NONE
#define REG_CFG_PTR2_PROPERTY           5406,       2,          FUSE_ECC_NONE
#define REG_CFG_PTR3_PROPERTY           5408,       2,          FUSE_ECC_NONE
#define REG_CFG_PTRn_PROPERTY(n)        5404 + ((n)-1) * 2, 2,  FUSE_ECC_NONE
#define REG_CFG_TABLE_PROPERTY          5410,       94,         FUSE_ECC_NONE

#define FUSE_BLOCK49_OFFSET             5504
#define OTP_PQA_PROPERTY(n)             5504 + (n) * 32,  30,   FUSE_ECC_NIBBLE_PARITY
#define OTP_PQA_VAL_PROPERTY(n)         5534 + (n) * 32,  2,    FUSE_ECC_NIBBLE_PARITY

// BLOCKS 50-57 (General Purpose 50-57)
#define FUSE_BLOCK50_OFFSET             5632
#define GP_50_57_OFFSET(n)              (5632 + ((n) - 50) * 128)
#define GP_50_57_PROPERTY(n)            GP_50_57_OFFSET(n), 128, FUSE_ECC_NONE

// BLOCKS 58-61 (General Purpose 58-61)
#define FUSE_BLOCK58_OFFSET             6656
#define GP_58_61_OFFSET(n)              (6656 + ((n) - 58) * 256)
#define GP_58_61_PROPERTY(n)            GP_58_61_OFFSET(n), 256, FUSE_ECC_NONE


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

DEFS_STATUS FUSE_WRPR_set            (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc, UINT8* value);
DEFS_STATUS FUSE_WRPR_get            (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc, UINT8* value);
DEFS_STATUS FUSE_WRPR_get_CP_Fustrap (UINT8* value);
DEFS_STATUS FUSE_64_72_SECDEC_Decode (UINT8 *datain, UINT8 *dataout, UINT32 encoded_size);
DEFS_STATUS FUSE_64_72_SECDEC_Encode (UINT8 *datain, UINT8 *dataout, UINT32 encoded_size);
DEFS_STATUS FUSE_WRPR_EraseBlock     (UINT16 block);

#endif // _FUSE_WRAPPER_
#endif // #ifdef FUSE_MODULE_TYPE

