/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "crypto/aes_gcm.h"
#include "platform_api.h"
#include "platform_io.h"
#include "tip_aes_gcm_ncl.h"
#include "tip_ncl_rom_if.h"
#include "tip_security.h"
#include "tip_rom_ncl_shared_state.h"


extern ROM_NCL_AES_ClearParams	   pROM_NCL_AES_ClearParams;
extern ROM_NCL_AES_Power		   pROM_NCL_AES_Power;
extern ROM_NCL_AES_SetKey		   pROM_NCL_AES_SetKey;
extern ROM_NCL_AES_GCM_Encrypt	   pROM_NCL_AES_GCM_Encrypt;
extern ROM_NCL_AES_GCM_Decrypt	   pROM_NCL_AES_GCM_Decrypt;
extern ROM_NCL_AES_SelectKey	   pROM_NCL_AES_SelectKey;
extern ROM_NCL_AES_GCM_SetParams   pROM_NCL_AES_GCM_SetParams;
extern ROM_NCL_AES_InitContext	   pROM_NCL_AES_InitContext;
extern ROM_NCL_AES_Init			   pROM_NCL_AES_Init;
extern ROM_NCL_AES_FinalizeContext pROM_NCL_AES_FinalizeContext;

#define AES_GCM_TAG_LEN 16

int tip_aes_gcm_ncl_set_key (const struct aes_gcm_engine *engine, const uint8_t *key, size_t length)
{
	const struct tip_aes_gcm_ncl_engine *tip_ncl = (const struct tip_aes_gcm_ncl_engine*) engine;
	NCL_STATUS_T ncl_status;
	NCL_AES_KEY_SIZE_T key_size;
	NCL_AES_ADD_DATA aes_added_info_gcm;

	if ((tip_ncl == NULL) || (key == NULL)) {
		return AES_GCM_ENGINE_INVALID_ARGUMENT;
	}

	switch (length) {
		case (128 / 8):
			key_size = NCL_AES_KEY_SIZE_128;
			break;
		case (192 / 8):
			key_size = NCL_AES_KEY_SIZE_192;
			break;
		case (256 / 8):
			key_size = NCL_AES_KEY_SIZE_256;
			break;
		default:
			return AES_GCM_ENGINE_INVALID_KEY_LENGTH;
	}

	tip_ncl->state->key_size = key_size;

	aes_added_info_gcm.authData = NULL;
	aes_added_info_gcm.authSize = 0;
	aes_added_info_gcm.ivSize = 12;
	aes_added_info_gcm.macSize = NCL_AES_GCM_MAC_SIZE_12B;

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	ncl_status = pROM_NCL_AES_GCM_SetParams (tip_ncl->state->context, NULL, (const uint32_t*) key,
		key_size, (const NCL_AES_ADD_DATA*) &aes_added_info_gcm, NCL_AES_OP_DECRYPT);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

	return tip_ncl_status (ncl_status, AES_GCM_ENGINE_NO_KEY);
}

static NCL_STATUS_T tip_aes_gcm_set_param (const struct tip_aes_gcm_ncl_engine *tip_ncl,
	const uint8_t *iv, size_t iv_length, size_t tag_length, const uint8_t *auth_data,
	size_t auth_data_length, NCL_AES_OP_T op)
{
	NCL_AES_ADD_DATA AESaddedinfoGCM;
	uint32_t tag_length_enum;
	NCL_STATUS_T status;

	switch (tag_length) {
		case 4:
			tag_length_enum = NCL_AES_GCM_MAC_SIZE_4B;
			break;
		case 8:
			tag_length_enum = NCL_AES_GCM_MAC_SIZE_8B;
			break;
		case 12:
			tag_length_enum = NCL_AES_GCM_MAC_SIZE_12B;
			break;
		case 13:
			tag_length_enum = NCL_AES_GCM_MAC_SIZE_13B;
			break;
		case 14:
			tag_length_enum = NCL_AES_GCM_MAC_SIZE_14B;
			break;
		case 15:
			tag_length_enum = NCL_AES_GCM_MAC_SIZE_15B;
			break;
		case 16:
			tag_length_enum = NCL_AES_GCM_MAC_SIZE_16B;
			break;
		default:
			return NCL_STATUS_INVALID_PARAM;
	}

	AESaddedinfoGCM.authData = (uint32_t*) auth_data;
	AESaddedinfoGCM.authSize = (uint32_t) auth_data_length;
	AESaddedinfoGCM.ivSize = (UINT32) iv_length;
	AESaddedinfoGCM.macSize = tag_length_enum;

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	status = pROM_NCL_AES_SelectKey (tip_ncl->state->context, 0);
	if (status != NCL_STATUS_OK) {
		tip_ncl->shared_state->unlock (tip_ncl->shared_state);
		return status;
	}

	status = pROM_NCL_AES_GCM_SetParams (tip_ncl->state->context, (uint32_t *) iv, NULL,
		tip_ncl->state->key_size, (const NCL_AES_ADD_DATA*) &AESaddedinfoGCM, op);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);

	return status;
}

int tip_aes_gcm_ncl_encrypt_data_common (const struct aes_gcm_engine *engine,
	const uint8_t *plaintext, size_t length, const uint8_t *iv, size_t iv_length,
	const uint8_t *addtional_data, size_t addtional_data_length, uint8_t *ciphertext,
	size_t out_length, uint8_t *tag, size_t tag_length)
{
	const struct tip_aes_gcm_ncl_engine *tip_ncl = (const struct tip_aes_gcm_ncl_engine*) engine;
	NCL_STATUS_T ncl_status;

	if ((tip_ncl == NULL) || (plaintext == NULL) || (length == 0) || (iv == NULL) ||
		(iv_length == 0) || (ciphertext == NULL) || (tag == NULL)) {
		return AES_GCM_ENGINE_INVALID_ARGUMENT;
	}

	if ((out_length < length) || (tag_length < 16)) {
		return AES_GCM_ENGINE_OUT_BUFFER_TOO_SMALL;
	}

	if (tip_ncl->state->key_size == 0) {
		return AES_GCM_ENGINE_NO_KEY;
	}

	ncl_status = tip_aes_gcm_set_param (tip_ncl, iv, iv_length, tag_length, addtional_data,
		addtional_data_length, (NCL_AES_OP_T) NCL_AES_OP_ENCRYPT);
	if (ncl_status == NCL_STATUS_OK) {
		tip_ncl->shared_state->lock (tip_ncl->shared_state);
		ncl_status = pROM_NCL_AES_GCM_Encrypt (tip_ncl->state->context, (const uint32_t*) plaintext,
			(uint32_t*) ciphertext, (uint32_t) length, (uint32_t*) tag);
		tip_ncl->shared_state->unlock (tip_ncl->shared_state);
		if (ncl_status != NCL_STATUS_OK) {
			platform_printf ("pROM_NCL_AES_GCM_Encrypt failed on %x" NEWLINE, ncl_status);
		}
	}

	return tip_ncl_status (ncl_status, AES_GCM_ENGINE_ENCRYPT_FAILED);
}

int tip_aes_gcm_ncl_encrypt_with_add_data (const struct aes_gcm_engine *engine,
	const uint8_t *plaintext, size_t length, const uint8_t *iv, size_t iv_length,
	const uint8_t *additional_data, size_t additional_data_length, uint8_t *ciphertext,
	size_t out_length, uint8_t *tag, size_t tag_length)
{
	return tip_aes_gcm_ncl_encrypt_data_common (engine, plaintext, length, iv, iv_length,
		additional_data, additional_data_length , ciphertext, out_length, tag, tag_length);
}

int tip_aes_gcm_ncl_encrypt_data (const struct aes_gcm_engine *engine, const uint8_t *plaintext,
	size_t length, const uint8_t *iv, size_t iv_length, uint8_t *ciphertext,
	size_t out_length, uint8_t *tag, size_t tag_length)
{
	return tip_aes_gcm_ncl_encrypt_data_common (engine, plaintext, length, iv, iv_length, NULL,
		0, ciphertext, out_length, tag, tag_length);
}

static int tip_aes_ncl_decrypt_data_common (const struct aes_gcm_engine *engine,
	const uint8_t *ciphertext, size_t length, const uint8_t *tag, const uint8_t *iv,
	size_t iv_length, const uint8_t *addtional_data, size_t addtional_data_length,
	uint8_t *plaintext, size_t out_length)
{
	const struct tip_aes_gcm_ncl_engine *tip_ncl = (const struct tip_aes_gcm_ncl_engine *) engine;
	NCL_STATUS_T ncl_status;

	if ((tip_ncl == NULL) || (ciphertext == NULL) || (length == 0) || (tag == NULL) ||
		(iv == NULL) || (iv_length == 0) || (plaintext == NULL))
		return AES_GCM_ENGINE_INVALID_ARGUMENT;

	if (out_length < length)
		return AES_GCM_ENGINE_OUT_BUFFER_TOO_SMALL;

	ncl_status = tip_aes_gcm_set_param (tip_ncl, iv, iv_length, AES_GCM_TAG_LEN, addtional_data,
		addtional_data_length, (NCL_AES_OP_T) NCL_AES_OP_DECRYPT);
	if (ncl_status != NCL_STATUS_OK) {
		return AES_GCM_ENGINE_NO_KEY;
	}

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	ncl_status = pROM_NCL_AES_GCM_Decrypt (tip_ncl->state->context, (const uint32_t*) ciphertext,
		(uint32_t*) plaintext, (uint32_t) length, (const uint32_t*) tag);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);
	if (ncl_status == NCL_STATUS_AUTHENTICATION_FAIL) {
		return tip_ncl_status (ncl_status, AES_GCM_ENGINE_GCM_AUTH_FAILED);
	}

	return tip_ncl_status (ncl_status, AES_GCM_ENGINE_DECRYPT_FAILED);
}

int tip_aes_gcm_ncl_decrypt_data (const struct aes_gcm_engine *engine, const uint8_t *ciphertext,
	size_t length, const uint8_t *tag, const uint8_t *iv, size_t iv_length, uint8_t *plaintext,
	size_t out_length)
{
	return tip_aes_ncl_decrypt_data_common (engine, ciphertext, length, tag, iv, iv_length,
		NULL, 0, plaintext, out_length);
}

int tip_aes_gcm_ncl_decrypt_with_add_data (const struct aes_gcm_engine *engine,
	const uint8_t *ciphertext, size_t length, const uint8_t *tag, const uint8_t *iv,
	size_t iv_length, const uint8_t *additional_data, size_t additional_data_length,
	uint8_t *plaintext, size_t out_length)
{
	return tip_aes_ncl_decrypt_data_common (engine, ciphertext, length, tag, iv, iv_length,
		additional_data, additional_data_length, plaintext, out_length);
}

/**
 * Initialize only the variable state of an NCL AES-GCM engine.  The rest of the instance is
 * assumed to already have been initialized.
 *
 * This would generally be used with a statically initialized instance.
 *
 * @param engine The AES-GCM engine that contains the state to initialize.
 *
 * @return 0 if the state was successfully initialized or an error code.
 */
int tip_aes_gcm_ncl_init_state (const struct tip_aes_gcm_ncl_engine *engine)
{
	if ((engine == NULL) || (engine->state == NULL)) {
		return AES_GCM_ENGINE_INVALID_ARGUMENT;
	}

	memset (engine->state, 0, sizeof (*engine->state));

	engine->shared_state->lock (engine->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_AES_Power (engine->state->context, TRUE),
		engine->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_AES_InitContext (engine->state->context),
		engine->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_AES_Init (engine->state->context),
		engine->shared_state);
	engine->shared_state->unlock (engine->shared_state);

	return 0;
}

/**
 * Initialize an instance for run AES-GCM operations using NCL.
 *
 * @param engine The AES engine to initialize.
 * @param state Variable context for AES-GCM operations.  This must be uninitialized.
 *
 * @return 0 if the AES engine was successfully initialized or an error code.
 */
int tip_aes_gcm_ncl_init (struct tip_aes_gcm_ncl_engine *engine,
	struct tip_aes_gcm_ncl_state *state, struct tip_rom_ncl_shared_state *shared_state)
{
	if (engine == NULL)
		return AES_GCM_ENGINE_INVALID_ARGUMENT;

	if (shared_state == NULL)
		return AES_GCM_ENGINE_INVALID_ARGUMENT;

	memset (engine, 0, sizeof (struct tip_aes_gcm_ncl_engine));

	engine->base.set_key = tip_aes_gcm_ncl_set_key;
	engine->base.encrypt_data = tip_aes_gcm_ncl_encrypt_data;
	engine->base.encrypt_with_add_data = tip_aes_gcm_ncl_encrypt_with_add_data;
	engine->base.decrypt_data = tip_aes_gcm_ncl_decrypt_data;
	engine->base.decrypt_with_add_data = tip_aes_gcm_ncl_decrypt_with_add_data;

	engine->state = state;
	engine->shared_state = shared_state;

	return tip_aes_gcm_ncl_init_state (engine);
}

/**
 * Release an tipncl AES GCM engine.
 *
 * @param engine The AES GCM engine to release.
 */
void tip_aes_gcm_ncl_release (const struct tip_aes_gcm_ncl_engine *engine)
{
	if (engine) {
		engine->shared_state->lock (engine->shared_state);
		pROM_NCL_AES_ClearParams (engine->state->context);
		pROM_NCL_AES_FinalizeContext (engine->state->context);
		engine->shared_state->unlock (engine->shared_state);
	}
}
