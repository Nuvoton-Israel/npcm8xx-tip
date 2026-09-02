/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef RIOT_CORE_TIP_H_
#define RIOT_CORE_TIP_H_

#include <stdbool.h>
#include "riot/riot_core_common.h"
#include "crypto/hash.h"
#include "crypto/ecc.h"
#include "asn1/x509.h"
#include "asn1/base64.h"


/**
 * TIP DICE layer 0 derived from riot_core_common for customization.
 */
struct riot_core_tip {
	struct riot_core_common base;
};

int riot_core_tip_init (struct riot_core_tip *riot, struct riot_core_common_state *state,
	struct hash_engine *hash, struct ecc_engine *ecc, struct x509_engine *x509,
	struct base64_engine *base64, size_t key_length,
	const struct x509_extension_builder *const *device_id_ext, size_t device_id_ext_count,
	const struct x509_extension_builder *const *alias_ext, size_t alias_ext_count);
void riot_core_tip_release (struct riot_core_tip *riot);


#endif /*  RIOT_CORE_TIP_H_ */
