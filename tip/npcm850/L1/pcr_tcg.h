/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef PCR_TCG_H_
#define PCR_TCG_H_

/**
 * PCR TCG event types
 */
enum {
	PCR_TCG_EVENT_TYPE_PLATFORM_BOOT_IMG = 0,
	PCR_TCG_EVENT_TYPE_PLATFORM_APP_IMG,
	PCR_TCG_EVENT_TYPE_PLATFORM_CERBERUS_FW_VERSION,
	PCR_TCG_EVENT_TYPE_PLATFORM_KMT_IMG,
	PCR_TCG_EVENT_TYPE_PLATFORM_SKMT_IMG,
	PCR_TCG_EVENT_TYPE_PLATFORM_BB_IMG,
	PCR_TCG_EVENT_TYPE_PLATFORM_BL31_IMG,
	PCR_TCG_EVENT_TYPE_PLATFORM_OPTEE_IMG,
	PCR_TCG_EVENT_TYPE_PLATFORM_UBOOT_IMG,
#ifdef CERBERUS_ENABLE_COMPONENT_ATTESTATION
	PCR_TCG_EVENT_TYPE_PCD_ID,
	PCR_TCG_EVENT_TYPE_PCD_DATA,
	PCR_TCG_EVENT_TYPE_PCD_PLATFORM_ID,
	PCR_TCG_EVENT_TYPE_CFM_ID,
	PCR_TCG_EVENT_TYPE_CFM_DATA,
	PCR_TCG_EVENT_TYPE_CFM_INITIALIZATION_STATUS,
	PCR_TCG_EVENT_TYPE_CFM_PLATFORM_ID,
#endif
};


#endif /* PCR_TCG_H_ */
