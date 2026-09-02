/*
 *  Nuvoton Technology Corporation confidential
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *<<<----------------------------------------------------------------------
 * File Contents:
 *   tip_aes_ncl.c
 *            This file contains API of security routines for ROM code AES engine
 *  Project:
 *            Arbel
 */

#include "crypto/aes_gcm.h"
#include "tip_aes_ncl.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "platform_api.h"
#include "platform_io.h"
#include "tip_security.h"
#include "tip_aes_ncl.h"


extern ROM_NCL_AES_ClearParams	   pROM_NCL_AES_ClearParams;
extern ROM_NCL_AES_Power		   pROM_NCL_AES_Power;
extern ROM_NCL_AES_SetKey		   pROM_NCL_AES_SetKey;
extern ROM_NCL_AES_Encrypt	   	   pROM_NCL_AES_Encrypt;
extern ROM_NCL_AES_Decrypt	  	   pROM_NCL_AES_Decrypt;
extern ROM_NCL_AES_SelectKey	   pROM_NCL_AES_SelectKey;
extern ROM_NCL_AES_InitContext	   pROM_NCL_AES_InitContext;
extern ROM_NCL_AES_Init			   pROM_NCL_AES_Init;
extern ROM_NCL_AES_FinalizeContext pROM_NCL_AES_FinalizeContext;
extern ROM_NCL_AES_SetParams       pROM_NCL_AES_SetParams;

#define AES_GCM_TAG_LEN 16

static int tip_aes_ncl_set_key (const struct aes_gcm_engine *engine, const uint8_t *key,
	size_t length)
{
	struct tip_aes_ncl_engine *tip_ncl = (struct tip_aes_ncl_engine*) engine;
	NCL_STATUS_T ncl_status = NCL_STATUS_OK;
	if ((tip_ncl == NULL) || (key == NULL)) {
		return TIP_AES_ENGINE_INVALID_ARGUMENT;
	}
	NCL_AES_KEY_SIZE_T key_size;

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
			return TIP_AES_ENGINE_INVALID_KEY_LENGTH;
	}

	tip_ncl->key_size = key_size;

	if (tip_ncl->aes_mode != NCL_AES_MODE_GCM && tip_ncl->aes_mode != NCL_AES_MODE_CCM) {
		ncl_status = pROM_NCL_AES_SetKey (tip_ncl->context, (const uint32_t*) key, key_size);
	}
	return tip_ncl_status (ncl_status, TIP_AES_ENGINE_NO_KEY);
}

static int tip_aes_ncl_encrypt_data_common (const struct aes_gcm_engine *engine,
	const uint8_t *plaintext, size_t length, const uint8_t *iv, size_t key_length,
	const uint8_t *additional_data, size_t additional_data_length, uint8_t *ciphertext,
	size_t out_length, uint8_t *tag, size_t tag_length)
{
	struct tip_aes_ncl_engine *tip_ncl = (struct tip_aes_ncl_engine*) engine;
	NCL_STATUS_T ncl_status;

	if ((tip_ncl == NULL) || (plaintext == NULL) || (length == 0) || (iv == NULL) ||
		(ciphertext == NULL)) {
		return TIP_AES_ENGINE_INVALID_ARGUMENT;
	}

	if (out_length < length) {
		return TIP_AES_ENGINE_OUT_BUFFER_TOO_SMALL;
	}
	ncl_status =  pROM_NCL_AES_SetParams (tip_ncl, NULL, key_length, tip_ncl->aes_mode,
			(uint32_t*) iv);

	if (ncl_status != NCL_STATUS_OK) {
		return AES_GCM_ENGINE_NO_KEY;
	}
	ncl_status = pROM_NCL_AES_Encrypt (tip_ncl->context, (uint32_t*) plaintext,
		(uint32_t*) ciphertext, (uint32_t) length);

	return tip_ncl_status (ncl_status, AES_GCM_ENGINE_ENCRYPT_FAILED);
}

static int tip_aes_ncl_encrypt_with_add_data (const struct aes_gcm_engine *engine,
	const uint8_t *plaintext, size_t length, const uint8_t *iv, size_t key_length,
	const uint8_t *additional_data, size_t additional_data_length, uint8_t *ciphertext,
	size_t out_length, uint8_t *tag, size_t tag_length)
{
	return tip_aes_ncl_encrypt_data_common (engine, plaintext, length, iv, key_length,
		additional_data, additional_data_length , ciphertext, out_length, tag, tag_length);
}


static int tip_aes_ncl_encrypt_data (const struct aes_gcm_engine *engine, const uint8_t *plaintext,
	size_t length, const uint8_t *iv, size_t key_length, uint8_t *ciphertext,
	size_t out_length, uint8_t *tag, size_t tag_length)

{
	return tip_aes_ncl_encrypt_data_common (engine, plaintext, length, iv, key_length, NULL,
		0, ciphertext, out_length, tag, tag_length);
}

static int tip_aes_ncl_decrypt_data_common (const struct aes_gcm_engine *engine,
	const uint8_t *ciphertext, size_t length, const uint8_t *tag, const uint8_t *iv,
	size_t key_length, const uint8_t *addtional_data, size_t addtional_data_length,
	uint8_t *plaintext, size_t out_length)
{
	struct tip_aes_ncl_engine *tip_ncl = (struct tip_aes_ncl_engine*) engine;
	NCL_STATUS_T ncl_status;
	if ((tip_ncl == NULL) || (ciphertext == NULL) || (length == 0) || (iv == NULL) ||
		(plaintext == NULL)) {
		return TIP_AES_ENGINE_INVALID_ARGUMENT;
	}
	if (out_length < length) {
		return TIP_AES_ENGINE_OUT_BUFFER_TOO_SMALL;
	}

	/* For Non-GCM operations, additional_data is NULL. */
	ncl_status =  pROM_NCL_AES_SetParams (tip_ncl, NULL, key_length, tip_ncl->aes_mode,
			(uint32_t*) iv);

	if (ncl_status != NCL_STATUS_OK) {
		return TIP_AES_ENGINE_NO_KEY;
	}
	ncl_status = pROM_NCL_AES_Decrypt (tip_ncl->context, (const uint32_t*) ciphertext,
		(uint32_t*) plaintext, (uint32_t) length);


	return tip_ncl_status (ncl_status, TIP_AES_ENGINE_DECRYPT_FAILED);
}

static int tip_aes_ncl_decrypt_data (const struct aes_gcm_engine *engine,
	const uint8_t *ciphertext, size_t length, const uint8_t *tag, const uint8_t *iv,
	size_t key_length,  uint8_t *plaintext, size_t out_length)
{
	return tip_aes_ncl_decrypt_data_common (engine, ciphertext, length, tag, iv, key_length, NULL,
		0, plaintext, out_length);
}

static int tip_aes_ncl_decrypt_with_add_data (const struct aes_gcm_engine *engine,
	const uint8_t *ciphertext, size_t length, const uint8_t *tag, const uint8_t *iv,
	size_t key_length, const uint8_t *additional_data, size_t additional_data_length,
	uint8_t *plaintext, size_t out_length)
{
	return tip_aes_ncl_decrypt_data_common (engine, ciphertext, length, tag, iv, key_length,
		additional_data, additional_data_length, plaintext, out_length);
}
/**
 * Sets the engine's member key_ind as the parameter key_num
 * @param engine The AES engine to release.
 * @param key_num to set.
 */
static int tip_aes_select_key (struct tip_aes_ncl_engine *engine, uint8_t key_num)
{
	if (engine == NULL)
		return TIP_AES_ENGINE_INVALID_ARGUMENT;

	engine->key_ind = key_num;

	NCL_STATUS_RET_CHECK (pROM_NCL_AES_SelectKey (engine->context, engine->key_ind));
	return 0;
}


/**
 * Sets the engine's member aes_mode as the parameter mode
 * @param engine The AES engine to release.
 * @param mode to set.
 */
static int tip_aes_set_mode (struct tip_aes_ncl_engine *engine, NCL_AES_MODE_T mode)
{
	if (engine == NULL)
		return TIP_AES_ENGINE_INVALID_ARGUMENT;

	engine->aes_mode = mode;
	return 0;
}

/* Initialize an instance for run AES operations using tipncl.
 *
 * @param engine The AES engine to initialize.
 *
 * @return 0 if the AES engine was successfully initialized or an error code.
 */
int tip_aes_ncl_init (struct tip_aes_ncl_engine *engine)
{
	if (engine == NULL)
		return TIP_AES_ENGINE_INVALID_ARGUMENT;

	memset (engine, 0, sizeof (struct tip_aes_ncl_engine));
	NCL_STATUS_RET_CHECK (pROM_NCL_AES_Power (engine->context, TRUE));

	NCL_STATUS_RET_CHECK (pROM_NCL_AES_InitContext (engine->context));

	NCL_STATUS_RET_CHECK (pROM_NCL_AES_Init (engine->context));

	engine->base.set_key = tip_aes_ncl_set_key;
	engine->base.encrypt_data = tip_aes_ncl_encrypt_data;
	engine->base.encrypt_with_add_data =  tip_aes_ncl_encrypt_with_add_data;
	engine->base.decrypt_data = tip_aes_ncl_decrypt_data;
	engine->base.decrypt_with_add_data = tip_aes_ncl_decrypt_with_add_data;
	engine->aes_mode = NCL_AES_MODE_GCM;
	engine->key_ind = 0;
	engine->select_key = tip_aes_select_key;
	engine->set_mode = tip_aes_set_mode;
	return 0;
}

/**
 * Release an tipncl AES engine.
 *
 * @param engine The AES engine to release.
 */
void tip_aes_ncl_release (struct tip_aes_ncl_engine *engine)
{
	if (engine) {
		pROM_NCL_AES_ClearParams (engine->context);
		pROM_NCL_AES_FinalizeContext (engine->context);
	}
}
