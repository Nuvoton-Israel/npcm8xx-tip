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


/* L1 Version number components. */
#define	FW_VERSION_MAJOR			0
#define	FW_VERSION_MINOR			5
#define	FW_VERSION_BUILD			4

/* String macros to convert version number. */
#define	FW_STRING(x)				#x
#define	FW_TO_STRING(x)				FW_STRING (x)

/**
 * The version string for the firmware.
 */
#define	CERBERUS_FW_VERSION_STRING	FW_TO_STRING (FW_VERSION_MAJOR) "." FW_TO_STRING (FW_VERSION_MINOR) "." FW_TO_STRING (FW_VERSION_BUILD)

/**
 *  SVN (security version number) data handler.
 *  This struct holds the data for the loaded data.
 */
struct tip_version_handler {
	uint16_t kmt_version;
	uint16_t kmt_version_minimal;
	uint16_t tip_fw_l0_version;
	uint16_t tip_fw_l0_version_minimal;
	uint16_t skmt_version;
	uint16_t skmt_version_minimal;
	uint16_t tip_fw_l1_version;
	uint16_t tip_fw_l1_version_minimal;
	uint16_t bmc_version;
	uint16_t bmc_version_minimal;
};

uint16_t tip_get_otp_version (IMG_TYPE_E img);
int tip_version_init (struct tip_version_handler *ver);
int tip_version_set_and_check (struct tip_version_handler *ver, uint16_t version_num,
	IMG_TYPE_E img);


#endif /* TIP_VERSION_H_ */
