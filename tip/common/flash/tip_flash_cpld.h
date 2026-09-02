/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/
#ifndef TIP_FLASH_CPLD_H_
#define TIP_FLASH_CPLD_H_

#include "flash/tip_flash.h"


#ifdef CPLD
#undef FLASH_MAX_FIU
#define FLASH_MAX_FIU FIU_MODULE_0

/* CPLD section number. This is the 3rd parameter for tip_flash_cpld_read */
#define  CFG0       0x0
#define  UFM0       0x1
#define  Reserved   0x2
#define  FEA        0x3
#define  CFG1       0x4
#define  UFM1       0x5
#define  PUBKEY     0x6
#define  CSEC       0x7
#define  UFM2       0x8
#define  UFM3       0x9
#define  ASEKEY     0xA
#define  USEC       0xB

#define CFG0_SIZE 	12542
#define UFM0_SIZE   3582
#define CFG1_SIZE 	12542
#define UFM1_SIZE   3582
#define UFM2_SIZE	1150
#define UFM3_SIZE	191


int tip_flash_cpld_initialize_access (uint32_t fiu, uint32_t cs);
int tip_flash_cpld_busy (void);
int tip_flash_cpld_read (uint8_t *rd_data, int rd_count, unsigned int cfg);
int tip_flash_cpld_read_page (uint8_t *rd_data, int rd_count, unsigned int cfg, int page_no);
int tip_flash_cpld_read_id (uint8_t *rd_data);
int tip_flash_cpld_read_usercode (uint8_t *rd_data);
int tip_flash_cpld_read_status (uint8_t *rd_data);
void tip_flash_cpld_set_mux (unsigned int val);

#endif

#endif /* TIP_FLASH_CPLD_H_ */
