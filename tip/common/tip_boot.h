/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *   tip_boot.h
 *            This file contains TIP FW boot definitions and declarations
 *  Project:  Arbel
 *------------------------------------------------------------------------*/

#ifndef TIP_BOOT_H_
#define TIP_BOOT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal.h"
#include "tip_ncl_rom_if.h"
#include "crypto/hash.h"


#define SEC_DME_NONCE_LENGTH        SHA512_HASH_LENGTH
#define SEC_DME_CHALLENGE_LENGTH    SHA512_HASH_LENGTH
#define SEC_PCR0_LENGTH             SHA512_HASH_LENGTH
#define SEC_PUB_KEY_SIZE            (2 * ECC_KEY_SIZE_DWORD_384 * sizeof(uint32_t))

/**
 * Boot image level
 */
typedef enum
{
	BOOT_IMG_KMT = 0,   /**< Key Manifest Table */
	BOOT_IMG_TFT,       /**< TIP Firmware Table */
	BOOT_IMG_NUM,
} BOOT_IMG_LEVEL_T;

/**
 * RESET type, bitwise. bit numbers are selected according to TIP_SCR1. 
 * can be multiple types of resets at the same time.
 */
typedef enum
{
	RESET_WD2 = 0x0001,
	RESET_WD1 = 0x0002,
	RESET_TIP = 0x0004,
	RESET_SW3 = 0x0008,
	RESET_SW2 = 0x0010,
	RESET_SW1 = 0x0020,
	RESET_WD0 = 0x0040,
	RESET_CORST = 0x0080,
	RESET_PORST = 0x0100,
}  RESET_TYPE_T;

/**
 * Image structure header : used by KMT and TFT (TIP_FW L0) only
 */
#pragma pack(push, 1)
typedef struct TIP_HEADER_STRUCT_Tag
{                                                     /* Offset    Size(bytes)  */
	uint32_t  anchor;                                 /* 0         4            */
	uint32_t  ext_anchor;                             /* 4         4            */
	uint32_t  reserved;                               /* 8         4            */
	uint32_t  img_crc;                                /* 12        4            */
	uint8_t   signature[96];                          /* 16        96           */
	uint8_t   spi0_flash_clk;                         /* 112       1            */
	uint8_t   spi1_flash_clk;                         /* 113       1            */
	uint8_t   spi3_flash_clk;                         /* 114       1            */
	uint8_t   reserved_0[3];                          /* 115       3            */
	uint16_t  spi_flash_rd_mode;                      /* 118       2            */
	uint32_t  load_start_addr;                        /* 120       4            */
	uint32_t  bmc_bb_pointer;                         /* 124       4            */
	uint32_t  bmc_bb_length;                          /* 128       4            */
	uint32_t  img_length;                             /* 132       4            */
	uint8_t   reserved_1;                             /* 136       1            */
	uint8_t   reserved_2[3];                          /* 137       3            */
	uint32_t  key_index;                              /* 140       4            */
	uint32_t  key_invalid;                            /* 144       4            */
	uint32_t  reserved_3;                             /* 148       4            */
	uint16_t  otp_version;                            /* 152       2            */
	uint16_t  minor_version;                          /* 154       2            */
	uint32_t  active_img_table;                       /* 156       4            */
	uint32_t  copy1_img_table;                        /* 160       4            */
	uint32_t  copy2_img_table;                        /* 164       4            */
	uint32_t  fw_table_pointer;                       /* 168       4            */
	uint8_t   aes_cbc_iv[16];                         /* 172       16           */
	uint32_t  timestamp;                              /* 188       4            */
	uint8_t   reserved_4[60];                         /* 176       72           */

}  TIP_HEADER_STRUCT_T;
#pragma pack(pop)

/**
 * General purpose image header structure
 */
#pragma pack(push, 1)
typedef union HEADER_GENERAL_tag {
	struct {
		                                         /* Offset       Size(bytes) Description                                                       */
		uint8_t   startTag[8];                   /* 0            8           AA55_0850h, 424F_4F54h (‘BOOT’)                                   */
		uint32_t  reserved;                      /* 8            4                                                                             */
		uint32_t  img_crc;                       /* 12           4                                                                             */
		uint8_t   signature[96];                 /* 16           96         Boot Block ECC signature decrypted with Customer’s Private Key     */

		/* Start signed area: */
		uint8_t   reservedSigned[28];            /* 0x70        0x188        Reserved, signed.                                                 */
		uint32_t  KeyIndex;                      /* 0x8C         140                                                                           */
		uint8_t   reservedSigned1[8];            /* 0x90         8        Reserved, signed.                                                    */
		uint16_t  version;                       /* 0x98                                                                                       */
		uint8_t   reservedSigned2[0x22];         /* 0x9A         ( challenge, IV..  ROM only)                                                  */
		uint32_t  timestamp;                     /* 0xBC                                                                                       */
		uint8_t   reservedSigned3[0x138];        /* 0xC0         0x138       Reserved, signed.                                                 */
		uint32_t  destAddr;                      /* 0x1F8                                                                                      */
		uint32_t  codeSize;                      /* 0x1FC                                                                                      */
	} header;

	uint8_t    bytes[512];
	uint32_t   words[128];

}  HEADER_GENERAL_T;
#pragma pack(pop)

/**
 * TFT (L0) image header
 */
#pragma pack(push, 1)
typedef union IMG_HEADER_tag {
	TIP_HEADER_STRUCT_T header;

	uint8_t bytes[256];
	uint16_t words[128];
	uint32_t dwords[64];
} IMG_HEADER_T;
#pragma pack(pop)

/**
 * TIP image stats table Structure
 */
#pragma pack(push, 1)
typedef struct {
									/* Offset   Size(bytes)                                                                 */
	uint8_t img_header_copy[256]; 	/* 0        256           <= reporpuse this area for 2 KMT keys (only two keys used).   */
	uint8_t img_hash[64];			/* 256      64                                                                          */
	uint32_t img_crc;				/* 320      4                                                                           */
	uint32_t img_table_address; 	/* 324      4                                                                           */
	uint32_t img_boot_sts;			/* 328      4                                                                           */
	uint32_t img_sec_sts;			/* 332      4                                                                           */
	uint8_t reserved[48];			/* 336      48                                                                          */
} IMG_STS_TABLE_T;
#pragma pack(pop)

/**
 * TIP Boot stats table structure
 */
#pragma pack(push, 1)
typedef struct {
									/* Offset   Size(bytes)  */
	uint32_t boot_sts;			 	/* 0        4            */
	uint32_t boot_sec_sts;		 	/* 4        4            */
	uint8_t boot_reset_sts;		 	/* 8        1            */
	uint8_t boot_prev_reset_sts; 	/* 8        1            */
	uint8_t reserved_0[22];		 	/* 8        22           */
} BOOT_STS_TABLE_T;
#pragma pack(pop)

/**
 * TIP Boot log table structure
 */
#pragma pack(push, 1)
typedef struct {
	IMG_STS_TABLE_T img_logs[BOOT_IMG_NUM];
	BOOT_STS_TABLE_T boot_log;
} BOOT_LOG_TABLE_T;
#pragma pack(pop)

/**
 * SEC PCR structure (From ROM)
 */
#pragma pack(push, 1)
typedef struct {				/* offset */
	uint16_t tip_dbg_ctl;		/* 0      */
	uint16_t tip_mem_ctl;		/* 2      */
	uint16_t tip_pqa_ctl;		/* 4      */
	uint16_t tip_sec_boot;		/* 6      */
	uint32_t fustrap1;			/* 8      */
	uint32_t fustrap2;			/* 12     */
	uint32_t tip_fcfg[8];		/* 16     */
	uint32_t bmc_fcfg[8];		/* 48     */
	uint8_t otp_life_cycle; 	/* 80     */
	uint8_t uds_valid[3];		/* 81     */
	uint8_t uds_version[3]; 	/* 84     */
	uint8_t padding[1];			/* 87     */
} SEC_PRE_PCR0_T;
#pragma pack(pop)

/**
 * SEC DME DICE structure (from A1 TIP ROM)
 */
#pragma pack(push, 1)
typedef struct SEC_DME_DICE_A1_tag {
	uint8_t dme_pub_key[SEC_PUB_KEY_SIZE];
	uint8_t dme_signature[SEC_PUB_KEY_SIZE];
	uint8_t dme_hash[SHA512_HASH_LENGTH];
	uint8_t dme_nonce[SEC_DME_NONCE_LENGTH];
	uint8_t dme_pcr0[SEC_PCR0_LENGTH];
	uint8_t dice_pcr0[SEC_PCR0_LENGTH];
	SEC_PRE_PCR0_T dme_pre_pcr0;
	SEC_PRE_PCR0_T dice_pre_pcr0;
	uint8_t cdi[SHA512_HASH_LENGTH];
	uint8_t dice_pub_key[SEC_PUB_KEY_SIZE];
} SEC_DME_DICE_T_A1;
#pragma pack(pop)

/**
 * SEC DME DICE structure (from A2 TIP ROM)
 */
#pragma pack(1)
typedef struct {
	uint8_t dme_pub_key[SEC_PUB_KEY_SIZE];
	uint8_t dme_signature[SEC_PUB_KEY_SIZE];
	uint8_t dme_hash[SHA512_HASH_LENGTH];
	uint8_t dme_nonce[SEC_DME_NONCE_LENGTH];
	uint8_t dme_challenge[SEC_DME_CHALLENGE_LENGTH];
	uint8_t dme_pcr0[SEC_PCR0_LENGTH];
	uint8_t dice_pcr0[SEC_PCR0_LENGTH];
	SEC_PRE_PCR0_T dme_pre_pcr0;
	SEC_PRE_PCR0_T dice_pre_pcr0;
	uint8_t cdi[SHA512_HASH_LENGTH];
	uint8_t dice_pub_key[SEC_PUB_KEY_SIZE];
} SEC_DME_DICE_T_A2;
#pragma pack()

/**
 * SEC DME DICE structure (from Z1 TIP ROM)
 */
#pragma pack(1)
typedef struct {
	UINT8 dme_pub_key[SEC_PUB_KEY_SIZE];
	UINT8 dme_signature[SEC_PUB_KEY_SIZE];
	UINT8 dme_hash[SHA512_HASH_LENGTH];
	UINT32 dme_tag;
	UINT32 dme_size;
	UINT8 dme_challenge[SEC_DME_CHALLENGE_LENGTH];
	UINT8 dme_salt[SEC_DME_CHALLENGE_LENGTH];
	UINT8 dme_pcr0[SEC_PCR0_LENGTH];
	UINT8 dice_pcr0[SEC_PCR0_LENGTH];
	UINT8 dme_pre_pcr0[SEC_PCR0_LENGTH];
	UINT8 dice_pre_pcr0[SEC_PCR0_LENGTH];
	UINT8 cdi[SHA512_HASH_LENGTH];
} SEC_DME_DICE_T_Z1;
#pragma pack()

void hardware_init (void);
void platform_reset (uint32_t reset_type);
void NVIC_BMC_reset (uint16_t num);
void tip_clear_reset_indication (void);
uint16_t tip_get_reset_indication (void);
void tip_update_reset_indication (bool updateIntcr2);
void NVIC_TrapHandlerCommon (uint16_t num);
void NVIC_IntHandlerCommon (uint16_t num);


#endif  /* TIP_BOOT_H_*/
