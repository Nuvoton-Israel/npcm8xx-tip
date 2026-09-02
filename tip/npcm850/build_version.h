/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *				build_version.h
 *				This file contains version number and string
 * Project:
 *				Arbel
 *------------------------------------------------------------------------*/

#ifndef BUILD_VERSION_H_
#define BUILD_VERSION_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "tip_firmware_component.h"


/* L1 Version number components. */
#define	FW_VERSION_MAJOR			0
#define	FW_VERSION_MINOR			7
#define	FW_VERSION_BUILD			6

/* String macros to convert version number. */
#define	FW_STRING(x)				#x
#define	FW_TO_STRING(x)				FW_STRING (x)

/**
 * The version string for the firmware.
 */
#define	CERBERUS_FW_VERSION_STRING	FW_TO_STRING (FW_VERSION_MAJOR) "." FW_TO_STRING (FW_VERSION_MINOR) "." FW_TO_STRING (FW_VERSION_BUILD)


#endif /* BUILD_VERSION_H_ */