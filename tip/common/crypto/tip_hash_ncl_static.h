// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.


#ifndef TIP_ECC_HW_NCL_STATIC_H_
#define TIP_ECC_HW_NCL_STATIC_H_

#include "tip_hash_ncl.h"


/* Internal functions declared to allow for static initialization. */
int tip_hash_ncl_calculate_sha1 (const struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length);
int tip_hash_ncl_start_sha1 (const struct hash_engine *engine);
int tip_hash_ncl_calculate_sha256 (const struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length);
int tip_hash_ncl_start_sha256 (const struct hash_engine *engine);
int tip_hash_ncl_calculate_sha384 (const struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length);
int tip_hash_ncl_start_sha384 (const struct hash_engine *engine);
int tip_hash_ncl_calculate_sha512 (const struct hash_engine *engine, const uint8_t *data,
	size_t length, uint8_t *hash, size_t hash_length);
int tip_hash_ncl_start_sha512 (const struct hash_engine *engine);
int tip_hash_ncl_update (const struct hash_engine *engine, const uint8_t *data,
	size_t length);
enum hash_type tip_hash_ncl_get_active_algorithm (const struct hash_engine *engine);
int tip_hash_ncl_finish (const struct hash_engine *engine, uint8_t *hash, size_t hash_length);
void tip_hash_ncl_cancel (const struct hash_engine *engine);

/**
 * Constant initializer for SHA1 APIs.
 */
#ifdef HASH_ENABLE_SHA1
#define	TIP_HASH_NCL_SHA1   \
	.calculate_sha1 = tip_hash_ncl_calculate_sha1, \
	.start_sha1 = tip_hash_ncl_start_sha1,
#else
#define	TIP_HASH_NCL_SHA1
#endif

/**
 * Constant initializer for SHA-256 APIs.
 */
#define	TIP_HASH_NCL_SHA256   \
	.calculate_sha256 = tip_hash_ncl_calculate_sha256, \
	.start_sha256 = tip_hash_ncl_start_sha256,

/**
 * Constant initializer for SHA-384 APIs.
 */
#ifdef HASH_ENABLE_SHA384
#define	TIP_HASH_NCL_SHA384 \
	.calculate_sha384 = tip_hash_ncl_calculate_sha384, \
	.start_sha384 = tip_hash_ncl_start_sha384,
#else
#define	TIP_HASH_NCL_SHA384
#endif

/**
 * Constant initializer for SHA-512 APIs.
 */
#ifdef HASH_ENABLE_SHA512
#define	TIP_HASH_NCL_SHA512 \
	.calculate_sha512 = tip_hash_ncl_calculate_sha512, \
	.start_sha512 = tip_hash_ncl_start_sha512,
#else
#define	TIP_HASH_NCL_SHA512
#endif

/**
 * Constant initializer for the hash API.
 */
#define	TIP_HASH_NCL_API_INIT  { \
		TIP_HASH_NCL_SHA1 \
		TIP_HASH_NCL_SHA256 \
		TIP_HASH_NCL_SHA384 \
		TIP_HASH_NCL_SHA512 \
		.get_active_algorithm = tip_hash_ncl_get_active_algorithm, \
		.update = tip_hash_ncl_update, \
		.finish = tip_hash_ncl_finish, \
		.cancel = tip_hash_ncl_cancel, \
	}

/**
 * Initialize a static hash engine.
 *
 * There is no validation done on the arguments.
 *
 * @param state_ptr Variable context for the hash engine.
 */
#define	tip_hash_ncl_static_init(state_ptr)	{ \
		.base = HASH_MBEDTLS_API_INIT, \
		.state = state_ptr, \
	}



#endif /* TIP_ECC_HW_NCL_STATIC_H_ */
