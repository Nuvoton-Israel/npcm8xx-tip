/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include "composite_eat/bmc_direct/bmc_direct_composite_eat_status.h"


enum bmc_direct_composite_eat_status bmc_direct_composite_eat_map_main_token_status (
	enum composite_eat_tip_main_token_status status)
{
	switch (status) {
		case COMPOSITE_EAT_TIP_MAIN_TOKEN_OK:
			return BMC_DIRECT_COMPOSITE_EAT_OK;
		case COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_VERSION:
			return BMC_DIRECT_COMPOSITE_EAT_BAD_VERSION;
		case COMPOSITE_EAT_TIP_MAIN_TOKEN_BAD_REQUEST:
			return BMC_DIRECT_COMPOSITE_EAT_BAD_REQUEST;
		case COMPOSITE_EAT_TIP_MAIN_TOKEN_TOO_MANY_RECORDS:
			return BMC_DIRECT_COMPOSITE_EAT_TOO_MANY_RECORDS;
		case COMPOSITE_EAT_TIP_MAIN_TOKEN_BUFFER_TOO_SMALL:
		case COMPOSITE_EAT_TIP_MAIN_TOKEN_RESPONSE_TOO_LARGE:
			return BMC_DIRECT_COMPOSITE_EAT_RESPONSE_TOO_SMALL;
		case COMPOSITE_EAT_TIP_MAIN_TOKEN_SIGN_ERROR:
			return BMC_DIRECT_COMPOSITE_EAT_SIGN_FAILED;
		case COMPOSITE_EAT_TIP_MAIN_TOKEN_BUSY:
			return BMC_DIRECT_COMPOSITE_EAT_BUSY;
		case COMPOSITE_EAT_TIP_MAIN_TOKEN_CANCELED:
			return BMC_DIRECT_COMPOSITE_EAT_INTERNAL;
		default:
			return BMC_DIRECT_COMPOSITE_EAT_INTERNAL;
	}
}
