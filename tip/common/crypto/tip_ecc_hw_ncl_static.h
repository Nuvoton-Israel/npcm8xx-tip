// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.


#ifndef TIP_ECC_HW_NCL_STATIC_H_
#define TIP_ECC_HW_NCL_STATIC_H_

#include "tip_ecc_hw_ncl.h"


/* Internal functions declared to allow for static initialization. */
int tip_ecc_hw_ncl_engine_get_ecc_public_key (const struct ecc_hw *ecc_hw,
	const uint8_t *priv_key, size_t key_length, struct ecc_point_public_key *pub_key);
int tip_ecc_hw_ncl_engine_generate_ecc_key_pair (const struct ecc_hw *ecc_hw,
	size_t key_length, uint8_t *priv_key, struct ecc_point_public_key *pub_key);
int tip_ecc_hw_ncl_engine_ecdsa_verify (const struct ecc_hw *ecc_hw,
	const struct ecc_point_public_key *pub_key, const struct ecc_ecdsa_signature *signature,
	const uint8_t *digest, size_t digest_length);
int tip_ecc_hw_ncl_engine_ecdh_compute (const struct ecc_hw *ecc_hw,
	const uint8_t *priv_key, size_t key_length, const struct ecc_point_public_key *pub_key,
	uint8_t *secret, size_t length);
int tip_ecc_hw_ncl_engine_is_free (const struct ecc_hw *ecc_hw);
int tip_ecc_hw_ncl_engine_ecdsa_sign (const struct ecc_hw *ecc_hw, const uint8_t *priv_key,
	size_t key_length, const uint8_t *digest, size_t digest_length, const struct rng_engine *rng,
	struct ecc_ecdsa_signature *signature);

/**
 * Constant Initializer for ECC HW API
 */
#define TIP_ECC_HW_API_INIT { \
		.get_ecc_public_key = tip_ecc_hw_ncl_engine_get_ecc_public_key, \
		.generate_ecc_key_pair = tip_ecc_hw_ncl_engine_generate_ecc_key_pair, \
		.ecdsa_verify = tip_ecc_hw_ncl_engine_ecdsa_verify, \
		.ecdh_compute = tip_ecc_hw_ncl_engine_ecdh_compute, \
		.ecdsa_sign = tip_ecc_hw_ncl_engine_ecdsa_sign, \
}

/**
 * Initialize a static instance for running ECC operations.
 *
 * There is no validation done on the arguments.
 *
 * @param state_ptr Variable context for the ECC engine.
 */
#define	tip_ecc_hw_ncl_engine_static_init(state_ptr)	{ \
		.base = TIP_ECC_HW_API_INIT, \
		.state = state_ptr, \
	}

#endif /* TIP_ECC_HW_NCL_STATIC_H_ */
