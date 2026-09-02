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

#define WD_WDIV_L0                  4
#define WD_PRESET_L0                10
#define WD_WDIV_L1                  4
#define WD_PRESET_L1                10

/* Time in seconds to pet the WD */
#define WD_PERIOD_SEC               10

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
	RESET_TIP = 0x4000,
	RESET_SW3 = 0x0008,
	RESET_SW2 = 0x0010,
	RESET_SW1 = 0x0020,
	RESET_WD0 = 0x0040,
	RESET_CORST = 0x0080,
	RESET_PORST = 0x0100,
	RESET_TIP_WOL = 0x0800,
	RESET_TIP_FORCE_RESTART = 0x0200,
	RESET_TIP_WD = 0x8000,
	RESET_TIP_SW_SEC_ERR = 0x1000,
}  RESET_TYPE_T;

/*
 * Chip life cycle states
 */
#define LIFE_CYCLE_UNTESTED   0x0     /* After die manufacturing - New untested chip */
#define LIFE_CYCLE_PRODUCTION 0x33    /* Production Device (written by customer)     */
#define LIFE_CYCLE_DEBUG      0x55    /* Debug (written by customer or Nuvoton)      */
#define LIFE_CYCLE_RMA_PQA    0x77    /* RMA or PQA (written by Nuvoton)             */
#define LIFE_CYCLE_EOL        0xFF    /* EOL (written by customer or Nuvoton)        */

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
	uint32_t  LMS_KMO;                                /* 128       4            */
	uint32_t  img_length;                             /* 132       4            */
	uint32_t  key_mask_select;                        /* 136       4 - A2 only  */
	uint32_t  key_index;                              /* 140       4 - A1 only  */
	uint32_t  key_invalid;                            /* 144       4            */
	uint8_t   tft_enc_ctl;                            /* 148       1            */
	uint8_t   reserved_4[1];                          /* 149       1            */
	uint16_t  otp_revocation_version;                 /* 150       2            */
	uint16_t  otp_version;                            /* 152       2            */
	uint16_t  minor_version;                          /* 154       2            */
	uint32_t  active_img_table;                       /* 156       4            */
	uint32_t  copy1_img_table;                        /* 160       4            */
	uint32_t  copy2_img_table;                        /* 164       4            */
	uint32_t  fw_table_pointer;                       /* 168       4            */
	uint8_t   aes_cbc_iv[16];                         /* 172       16           */
	uint32_t  timestamp;                              /* 188       4            */
	uint8_t   reserved_5[8];                          /* 192       8            */
	uint32_t  SystemControlFlags;                    /*  200       4            */
	uint32_t  log_start_address;                     /*  204       4            */
	uint32_t  log_size;                              /*  208       4            */
	uint8_t   reserved_6[44];                        /*  212       44           */

}  TIP_HEADER_STRUCT_T;
#pragma pack(pop)

/**
 * General purpose image header structure
 */
#pragma pack(push, 1)
typedef union HEADER_GENERAL_tag {
	struct {
		                                         /* Offset       Size(bytes) Description                                                        */
		uint8_t   startTag[8];                   /* 0            8           AA55_0850h, 424F_4F54h (‘BOOT’)                                    */
		uint32_t  reserved;                      /* 8            4                                                                              */
		uint32_t  img_crc;                       /* 0xC  (12)    4                                                                              */
		uint8_t   signature[96];                 /* 0x10 (16)    96          Boot Block ECC signature decrypted with Customer’s Private Key     */
		/* Start signed area: */
		uint8_t   reservedSigned[20];            /* 0x70 (112)   0x14(20)    Reserved, signed.                                                  */
		uint32_t  SystemControlFlags;            /* 0x84 (132)    4                                                                             */
		uint32_t  key_mask_select;               /* 0x88 (136)   4           - A2 only                                                          */
		uint32_t  KeyIndex;                      /* 0x8C (140)   4           - A1 only                                                          */
		uint8_t   reservedSigned1[4];            /* 0x90 (144)   4           Reserved, signed.                                                  */
		uint8_t   tft_enc_ctl;                   /* 0x94 (148)   1                                                                              */
		uint8_t   enableLMS;                     /* 0x95 (149)               whether LMS is anbled or not .    signed                           */
		uint16_t  next_version; 				 /* 0x96																					   */
		uint16_t  version;						 /* 0x98																					   */
		uint8_t   reservedSigned2[0x12];		 /* 0x9A		 ( challenge, IV..	ROM only)												   */
		uint8_t   aes_cbc_iv[16];				 /* 0xAC		 ( challenge, IV..	ROM only)												   */
		uint32_t  timestamp;					 /* 0xBC																					   */
		uint32_t  key_mask_select_lms;           /* 0xC0 (192)   4           - A2 only                                                          */
		uint32_t  KeyIndex_lms;                  /* 0xC4 (196)               - A1 only                                                          */
		uint8_t   reservedSigned3[0xEC];         /* 0xC8 (200)               Reserved, signed.                                                  */
		uint32_t  regs_offset;                   /* 0x1B4 (436)   4         offset to the external register table                               */
		uint8_t   reservedSigned4[0x40];         /* 0x1B8 (440)   0x40        Reserved, signed.                                                 */
		uint32_t  destAddr;                      /* 0x1F8(504)                                                                                  */
		uint32_t  codeSize;                      /* 0x1FC(508)                                                                                  */
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
										 /* Offset   Size(bytes)     */
	TIP_HEADER_STRUCT_T img_header_copy; /* 0        256           <= repurpose this area for 2 KMT
											keys (only two keys used).   */
	uint8_t img_hash[64];				 /* 256      64				*/
	uint32_t img_crc;					 /* 320      4					*/
	uint32_t img_table_address;			 /* 324      4		  */
	uint32_t img_boot_sts;				 /* 328      4			   */
	uint32_t img_sec_sts;				 /* 332      4				*/
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
} BOOT_STS_TABLE_T;
#pragma pack(pop)

/**
 * TIP Boot log table structure
 */

#pragma pack(push, 1)
typedef struct
{
	                                          /* Offset   Size(bytes) */
	uint32_t  boot_fcfg[8];                   /*  0        32         */
	uint32_t  reset_inidication;              /* 32        4          */
	uint32_t  prev_reset_inidication;         /* 36        4          */
	uint8_t   reset_sts;                      /* 40        1          */
	uint8_t   prev_reset_sts;                 /* 41        1          */
	uint8_t   reserved_shared_ram[78];
}  RESET_STS_TABLE_T;
#pragma pack()

#pragma pack(push, 1)
typedef struct {
	IMG_STS_TABLE_T img_logs[BOOT_IMG_NUM];
	BOOT_STS_TABLE_T boot_log;
 	RESET_STS_TABLE_T   reset_log;
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

#pragma pack(push,1)
typedef union {
	SEC_DME_DICE_T_A1 *a1;
	SEC_DME_DICE_T_A2 *a2;
} SEC_DME_DICE_T_PTR;
#pragma pack()

/**
 * TIP DME structure format (Type 7) to be carried into DME extension.
 */
#pragma pack(1)
struct tip_dme_struct_data_a2 {
	uint8_t dme_nonce[SEC_DME_NONCE_LENGTH];
	uint8_t dme_challenge[SEC_DME_CHALLENGE_LENGTH];
	uint8_t dice_pub_key[SEC_PUB_KEY_SIZE];
	uint8_t dme_pcr0[SEC_PCR0_LENGTH];
};
#pragma pack()


/**
 * buffer that holds the data to print the log into the memory
 */
struct log_buffer_t {
	uint32_t log_start_address;
	uint32_t log_size;
	uint32_t log_next_address;
	bool  is_ddr_and_read;
};


/**
 * TIP L1 header handler for flag SystemControlFlags (control in IGPS, TipFwAndHeader_L1.xml)
 */
struct tip_L1_system_control {
	bool tip_recovery_force;           /* bit 0 in SystemControlFlags */
	bool tip_start_wd;                 /* bit 1 in SystemControlFlags */
	bool tip_support_encryption;       /* bit 2 in SystemControlFlags */
	bool tip_gpio_wol;                 /* bit 3 in SystemControlFlags */
	bool tip_disable_print_to_uart;    /* bit 5 in SystemControlFlags */
	bool tip_print_to_memory;          /* bit 6 in SystemControlFlags */
};

/**
 * TIP L0 header handler for flag SystemControlFlags (control in IGPS, TipFwAndHeader_L1.xml)
 */
struct tip_L0_system_control {
	bool tip_disable_print_to_uart;             /* bit 0 in SystemControlFlags */
	bool tip_print_to_memory;          /* bit 1 in SystemControlFlags */
};


void tip_L1_system_control_init (struct tip_L1_system_control *tip_L1_sys_ctrl_l, uint32_t val);
void hardware_init (void);
void NVIC_BMC_reset (uint16_t num);
void NVIC_TrapHandlerCommon (uint16_t num);
void NVIC_IntHandlerCommon (uint16_t num);
void tip_twd_common_init (bool enable, unsigned int preset, unsigned int wdiv);
bool tip_check_mem_log_valid_location (uint32_t start_addr, uint32_t size);

#ifdef ENABLE_RECOVERY_PROTECTION
int protect_recovery_flash (void);
#endif
#endif  /* TIP_BOOT_H_*/
