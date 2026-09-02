// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_AES_GCM_NCL_STATIC_H_
#define TIP_AES_GCM_NCL_STATIC_H_

#include "tip_aes_gcm_ncl.h"


/* Internal functions declared to allow for static initialization. */
int tip_aes_gcm_ncl_set_key (const struct aes_gcm_engine *engine, const uint8_t *key,
	size_t length);
int tip_aes_gcm_ncl_encrypt_data (const struct aes_gcm_engine *engine, const uint8_t *plaintext,
	size_t length, const uint8_t *iv, size_t iv_or_key_length, uint8_t *ciphertext,
	size_t out_length, uint8_t *tag, size_t tag_length);
int tip_aes_gcm_ncl_encrypt_with_add_data (const struct aes_gcm_engine *engine,
	const uint8_t *plaintext, size_t length, const uint8_t *iv, size_t iv_or_key_length,
	const uint8_t *additional_data, size_t additional_data_length, uint8_t *ciphertext,
	size_t out_length, uint8_t *tag, size_t tag_length);
int tip_aes_gcm_ncl_decrypt_data (const struct aes_gcm_engine *engine, const uint8_t *ciphertext,
	size_t length, const uint8_t *tag, const uint8_t *iv, size_t iv_length, uint8_t *plaintext,
	size_t out_length);
int tip_aes_gcm_ncl_decrypt_with_add_data (const struct aes_gcm_engine *engine,
	const uint8_t *ciphertext, size_t length, const uint8_t *tag, const uint8_t *iv,
	size_t iv_length, const uint8_t *additional_data, size_t additional_data_length,
	uint8_t *plaintext, size_t out_length);

/**
 * Constant initializer for the AES-GCM API.
 */
#define	TIP_AES_GCM_NCL_API_INIT { \
		.set_key = tip_aes_gcm_ncl_set_key, \
		.encrypt_data = tip_aes_gcm_ncl_encrypt_data, \
		.encrypt_with_add_data = tip_aes_gcm_ncl_encrypt_with_add_data, \
		.decrypt_data = tip_aes_gcm_ncl_decrypt_data, \
		.decrypt_with_add_data = tip_aes_gcm_ncl_decrypt_with_add_data, \
	}


/**
 * Initialize a static instance for running AES-GCM operations.
 *
 * There is no validation done on the arguments.
 *
 * @param state_ptr Variable context for AES operations.
 */
#define	tip_aes_gcm_ncl_static_init(state_ptr) { \
		.base = TIP_AES_GCM_NCL_API_INIT, \
		.state = state_ptr, \
	}


#endif	/* TIP_AES_GCM_NCL_STATIC_H_ */
