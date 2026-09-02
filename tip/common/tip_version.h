/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *    			tip_version.h
 *            This file contains version handling  manager
 * Project:
 *            Arbel
 *------------------------------------------------------------------------*/

#ifndef TIP_VERSION_H_
#define TIP_VERSION_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "tip_firmware_component.h"


/**
 *  SVN (security version number) data handler.
 *  This struct holds the data for the loaded data.
 */
#pragma pack(push, 1)
struct tip_version_handler {
	uint16_t kmt_version;
	uint16_t kmt_version_minimal_in_otp;
	uint16_t kmt_version_next;
	uint16_t tip_fw_l0_version;
	uint16_t tip_fw_l0_version_minimal_in_otp;
	uint16_t tip_fw_l0_version_next;
	uint16_t skmt_version;
	uint16_t skmt_version_minimal_in_otp;
	uint16_t skmt_version_next;
	uint16_t tip_fw_l1_version;
	uint16_t tip_fw_l1_version_minimal_in_otp;
	uint16_t tip_fw_l1_version_next;
	uint16_t bmc_version;
	uint16_t bmc_version_minimal_in_otp;
	uint16_t bmc_version_next;
};
#pragma pack(pop)

uint16_t tip_get_otp_version (IMG_TYPE_E img);
int tip_version_init (struct tip_version_handler *ver);
int tip_version_set_in_OTP (struct tip_version_handler *ver);
int tip_version_check (struct tip_version_handler *ver, uint16_t version_num,
	uint16_t minimal_version_num, IMG_TYPE_E img);
int tip_version_set (struct tip_version_handler *ver, uint16_t version_num,
	uint16_t minimal_version_num, IMG_TYPE_E img);


#endif /* TIP_VERSION_H_ */
