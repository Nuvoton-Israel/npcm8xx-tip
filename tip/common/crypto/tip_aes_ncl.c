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

#include "crypto/aes.h"
#include "tip_aes_ncl.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "platform_api.h"
#include "platform_io.h"
#include "tip_security.h"


extern ROM_NCL_AES_ClearParams	   pROM_NCL_AES_ClearParams;
extern ROM_NCL_AES_Power		   pROM_NCL_AES_Power;
extern ROM_NCL_AES_SetKey		   pROM_NCL_AES_SetKey;
extern ROM_NCL_AES_GCM_Encrypt	   pROM_NCL_AES_GCM_Encrypt;
extern ROM_NCL_AES_GCM_Decrypt	   pROM_NCL_AES_GCM_Decrypt;
extern ROM_NCL_AES_Encrypt	   	   pROM_NCL_AES_Encrypt;
extern ROM_NCL_AES_Decrypt	  	   pROM_NCL_AES_Decrypt;
extern ROM_NCL_AES_SelectKey	   pROM_NCL_AES_SelectKey;
extern ROM_NCL_AES_GCM_SetParams   pROM_NCL_AES_GCM_SetParams;
extern ROM_NCL_AES_InitContext	   pROM_NCL_AES_InitContext;
extern ROM_NCL_AES_Init			   pROM_NCL_AES_Init;
extern ROM_NCL_AES_FinalizeContext pROM_NCL_AES_FinalizeContext;
extern ROM_NCL_AES_SetParams       pROM_NCL_AES_SetParams;

#define AES_GCM_TAG_LEN 16

static int tip_aes_ncl_set_key (struct aes_engine *engine, const uint8_t *key, size_t length)
{
	struct tip_aes_ncl_engine *tip_ncl = (struct tip_aes_ncl_engine *) engine;
	NCL_STATUS_T ncl_status = NCL_STATUS_OK;
	if ((tip_ncl == NULL) || (key == NULL)) {
		return AES_ENGINE_INVALID_ARGUMENT;
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
			return AES_ENGINE_INVALID_KEY_LENGTH;
	}

	tip_ncl->key_size = key_size;

	if (tip_ncl->aes_mode != NCL_AES_MODE_GCM && tip_ncl->aes_mode != NCL_AES_MODE_CCM) {
		ncl_status = pROM_NCL_AES_SetKey (tip_ncl->context, (const uint32_t *) key, key_size);
	} 
	else {
		/*dummy info , the real one will enter later upon encrypt\decrypt*/
		NCL_AES_ADD_DATA aes_added_info_gcm;
		aes_added_info_gcm.authData = NULL;
		aes_added_info_gcm.authSize = 0;
		aes_added_info_gcm.ivSize = 12;
		aes_added_info_gcm.macSize = NCL_AES_GCM_MAC_SIZE_12B;
		ncl_status = pROM_NCL_AES_GCM_SetParams (tip_ncl->context, NULL, (const uint32_t *) key,
			key_size, (const NCL_AES_ADD_DATA *) &aes_added_info_gcm, NCL_AES_OP_DECRYPT);
	}
	return tip_ncl_status (ncl_status, AES_ENGINE_NO_KEY);
}


static NCL_STATUS_T set_params (struct tip_aes_ncl_engine *tip_ncl, const uint8_t *iv, size_t iv_length,
	size_t tag_length, NCL_AES_OP_T op)
{
	if (tip_ncl->aes_mode == NCL_AES_MODE_GCM) {
		NCL_AES_ADD_DATA AESaddedinfoGCM;
		uint32_t tag_length_enum;
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

		AESaddedinfoGCM.authData = NULL;
		AESaddedinfoGCM.authSize = 0;
		AESaddedinfoGCM.ivSize = (UINT32) iv_length;
		AESaddedinfoGCM.macSize = tag_length_enum;

		NCL_STATUS_RET_CHECK (pROM_NCL_AES_SelectKey (tip_ncl->context, tip_ncl->key_ind));

		return pROM_NCL_AES_GCM_SetParams (tip_ncl->context, (uint32_t *) iv, NULL, tip_ncl->key_size,
			(const NCL_AES_ADD_DATA *) &AESaddedinfoGCM, op);
	} 
	else {
		return pROM_NCL_AES_SetParams (tip_ncl->context, NULL, iv_length, tip_ncl->aes_mode,
			(uint32_t *) iv);
	}
}

static int tip_aes_ncl_encrypt_data (struct aes_engine *engine, const uint8_t *plaintext,
	size_t length, const uint8_t *iv, size_t iv_length, uint8_t *ciphertext, size_t out_length,
	uint8_t *tag, size_t tag_length)

{
	struct tip_aes_ncl_engine *tip_ncl = (struct tip_aes_ncl_engine *) engine;
	NCL_STATUS_T ncl_status;

	if ((tip_ncl == NULL) || (plaintext == NULL) || (length == 0) || (iv == NULL) ||
		(iv_length == 0) || (ciphertext == NULL)) {
		return AES_ENGINE_INVALID_ARGUMENT;
	}

	if (out_length < length) {
		return AES_ENGINE_OUT_BUFFER_TOO_SMALL;
	}

	if (tip_ncl->aes_mode == NCL_AES_MODE_GCM) {
		if (tag == NULL) {
			return AES_ENGINE_INVALID_ARGUMENT;
		}

		if (tag_length < 16) {
			return AES_ENGINE_OUT_BUFFER_TOO_SMALL;
		}
		ncl_status = set_params (tip_ncl, iv, iv_length, tag_length,
			(NCL_AES_OP_T) NCL_AES_OP_ENCRYPT);
		if (ncl_status != NCL_STATUS_OK) {
			return AES_ENGINE_NO_KEY;
		}
		ncl_status = pROM_NCL_AES_GCM_Encrypt (tip_ncl->context, (const uint32_t *) plaintext,
			(uint32_t *) ciphertext, (uint32_t) length, (uint32_t *) tag);

	} else {
		ncl_status = set_params (tip_ncl, iv, iv_length, tag_length,
			(NCL_AES_OP_T) NCL_AES_OP_ENCRYPT);
		if (ncl_status != NCL_STATUS_OK) {
			return AES_ENGINE_NO_KEY;
		}
		ncl_status = pROM_NCL_AES_Encrypt (tip_ncl->context, (uint32_t *) plaintext,
			(uint32_t *) ciphertext, (uint32_t) length);
	}
	return tip_ncl_status (ncl_status, AES_ENGINE_ENCRYPT_FAILED);
}

static int tip_aes_ncl_decrypt_data (struct aes_engine *engine, const uint8_t *ciphertext,
	size_t length, const uint8_t *tag, const uint8_t *iv, size_t iv_length, uint8_t *plaintext,
	size_t out_length)
{
	struct tip_aes_ncl_engine *tip_ncl = (struct tip_aes_ncl_engine *) engine;
	NCL_STATUS_T ncl_status;
	if ((tip_ncl == NULL) || (ciphertext == NULL) || (length == 0) || (iv == NULL) ||
		(iv_length == 0) || (plaintext == NULL)) {
		return AES_ENGINE_INVALID_ARGUMENT;
	}
	if (out_length < length) {
		return AES_ENGINE_OUT_BUFFER_TOO_SMALL;
	}
	if (tip_ncl->aes_mode == NCL_AES_MODE_GCM) {
		if (tag == NULL) {
			return AES_ENGINE_INVALID_ARGUMENT;
		}
		ncl_status = set_params (tip_ncl, iv, iv_length, AES_GCM_TAG_LEN,
			(NCL_AES_OP_T) NCL_AES_OP_DECRYPT);

		if (ncl_status != NCL_STATUS_OK) {
			return AES_ENGINE_NO_KEY;
		}
		ncl_status = pROM_NCL_AES_GCM_Decrypt (tip_ncl->context, (const uint32_t *) ciphertext,
			(uint32_t *) plaintext, (uint32_t) length, (const uint32_t *) tag);
	
		if (ncl_status == NCL_STATUS_AUTHENTICATION_FAIL) {
			return tip_ncl_status (ncl_status, AES_ENGINE_GCM_AUTH_FAILED);
		}
	} else {
		ncl_status = set_params (tip_ncl, iv, iv_length, AES_GCM_TAG_LEN,
			(NCL_AES_OP_T) NCL_AES_OP_DECRYPT);
		if (ncl_status != NCL_STATUS_OK) {
			return AES_ENGINE_NO_KEY;
		}
		ncl_status = pROM_NCL_AES_Decrypt (tip_ncl->context, (const uint32_t *) ciphertext,
			(uint32_t *) plaintext, (uint32_t) length);
	}


	return tip_ncl_status (ncl_status, AES_ENGINE_DECRYPT_FAILED);
}
/**
 * Sets the engine's member key_ind as the parameter key_num
 * @param engine The AES engine to release.
 * @param key_num to set.
 */
static int tip_aes_select_key (struct tip_aes_ncl_engine *engine, uint8_t key_num)
{
	if (engine == NULL)
		return AES_ENGINE_INVALID_ARGUMENT;

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
		return AES_ENGINE_INVALID_ARGUMENT;

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
		return AES_ENGINE_INVALID_ARGUMENT;

	memset (engine, 0, sizeof (struct tip_aes_ncl_engine));
	NCL_STATUS_RET_CHECK (pROM_NCL_AES_Power (engine->context, TRUE));

	NCL_STATUS_RET_CHECK (pROM_NCL_AES_InitContext (engine->context));

	NCL_STATUS_RET_CHECK (pROM_NCL_AES_Init (engine->context));

	engine->base.set_key = tip_aes_ncl_set_key;
	engine->base.encrypt_data = tip_aes_ncl_encrypt_data;
	engine->base.decrypt_data = tip_aes_ncl_decrypt_data;
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
