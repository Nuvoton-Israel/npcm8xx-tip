/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef BMC_DIRECT_COMPOSITE_EAT_STATUS_H_
#define BMC_DIRECT_COMPOSITE_EAT_STATUS_H_

#include "composite_eat/bmc_direct/bmc_direct_composite_eat_abi.h"
#include "composite_eat/tip_main_token_generator.h"


enum bmc_direct_composite_eat_status bmc_direct_composite_eat_map_main_token_status (
	enum composite_eat_tip_main_token_status status);


#endif /* BMC_DIRECT_COMPOSITE_EAT_STATUS_H_ */
