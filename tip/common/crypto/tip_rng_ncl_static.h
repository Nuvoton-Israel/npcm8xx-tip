// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_RNG_NCL_STATIC_H_
#define TIP_RNG_NCL_STATIC_H_

#include "tip_rng_ncl.h"

/* Internal functions declared to allow for static initialization. */
int tip_rng_ncl_generate_random_buffer (const struct rng_engine *engine, size_t rand_len,
	uint8_t *buf);


/**
 * Constant initializer for the RNG API.
 */
#define	TIP_RNG_NCL_API_INIT { \
		.generate_random_buffer = tip_rng_ncl_generate_random_buffer, \
	}


/**
 * Initialize a static NCL engine for generating random numbers using a hardware DRBG.
 *
 * There is no validation done on the arguments.
 *
 * @param state_ptr Variable context for RNG operations.
 */
#define	tip_rng_ncl_static_init(state_ptr, shared_state_ptr) { \
		.base = TIP_RNG_NCL_API_INIT, \
		.state = state_ptr, \
		.shared_state = shared_state_ptr, \
	}


#endif	/* TIP_RNG_NCL_STATIC_H_ */
