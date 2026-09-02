/*
 *  Nuvoton Technology Corporation confidential
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *<<<----------------------------------------------------------------------
 * File Contents:
 *   tip_rsa_ncl.c
 *            This file contains API of security routines for ROM code AES engine
 *  Project:
 *            Arbel
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "tip_rsa_ncl.h"
#include "mbedtls/pk_internal.h"
#include "mbedtls/rsa.h"
#include "logging/debug_log.h"
#include "crypto/crypto_logging.h"
#include "tip_utils.h"

extern ROM_NCL_RSA_Init			   pROM_NCL_RSA_Init;
extern ROM_NCL_RSA_InitContext	   pROM_NCL_RSA_InitContext;
extern ROM_NCL_RSA_VerifyHash	   pROM_NCL_RSA_VerifyHash;
extern ROM_NCL_RSA_Decrypt		   pROM_NCL_RSA_Decrypt;
extern ROM_NCL_RSA_SetKey		   pROM_NCL_RSA_SetKey;
extern ROM_NCL_RSA_GetStatus	   pROM_NCL_RSA_GetStatus;
extern ROM_NCL_RSA_FinalizeContext pROM_NCL_RSA_FinalizeContext;
extern ROM_NCL_SHA_InitContext	   pROM_NCL_SHA_InitContext;
extern ROM_NCL_SHA_FinalizeContext pROM_NCL_SHA_FinalizeContext;
extern ROM_NCL_SHA_Power		   pROM_NCL_SHA_Power;
extern ROM_NCL_RSA_Encrypt		   pROM_NCL_RSA_Encrypt;
extern ROM_NCL_RSA_Power		   pROM_NCL_RSA_Power;

#define RSA_PRIV_DER_MAX_SIZE         \
	47 + (3 * MBEDTLS_MPI_MAX_SIZE) + \
		(5 * ((MBEDTLS_MPI_MAX_SIZE / 2) + (MBEDTLS_MPI_MAX_SIZE % 2)))
#define RSA_PUB_DER_MAX_SIZE 38 + (2 * MBEDTLS_MPI_MAX_SIZE)

#define NCL_BUSY_WAITTIMEOUT(busy_cond, timeout) \
	{                                            \
		uint32_t __time = timeout;               \
                                                 \
		do {                                     \
			if (__time-- == 0) {                 \
				return NCL_STATUS_NO_RESPONSE;   \
			}                                    \
		} while (busy_cond);                     \
	}

#define rsa_get_rsa_key(x) mbedtls_pk_rsa (*((mbedtls_pk_context *) x->context))

#if (defined RSA_ENABLE_PRIVATE_KEY || defined RSA_ENABLE_DER_PUBLIC_KEY)
/**
 * Allocate and initialize a context for an RSA key.
 *
 * @return The initialized key context or null.
 */
static mbedtls_pk_context *rsa_alloc_key_context ()
{
	mbedtls_pk_context *key = platform_malloc (sizeof (mbedtls_pk_context));

	if (key != NULL) {
		mbedtls_pk_init (key);
	}

	return key;
}

/**
 * Zeroize an RSA key context and free the memory.
 *
 * @param context The context to free.
 */
static void rsa_free_key_context (void *context)
{
	mbedtls_pk_free ((mbedtls_pk_context *) context);
	platform_free (context);
}
#endif

#ifdef RSA_ENABLE_PRIVATE_KEY
static int rsa_init_private_key (struct rsa_engine *engine, struct rsa_private_key *priv_key,
	const uint8_t *der, size_t key_length)
{
	mbedtls_pk_context *key_ctx;
	int status;

	if ((engine == NULL) || (priv_key == NULL) || (der == NULL) || (key_length == 0)) {
		return RSA_ENGINE_INVALID_ARGUMENT;
	}

	memset (priv_key, 0, sizeof (struct rsa_private_key));

	key_ctx = rsa_alloc_key_context ();
	if (key_ctx == NULL) {
		return RSA_ENGINE_NO_MEMORY;
	}

	status = mbedtls_pk_parse_key (key_ctx, der, key_length, NULL, 0);
	if (status != 0) {
		debug_log_create_entry (DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_CRYPTO,
			CRYPTO_LOG_MSG_MBEDTLS_PK_PARSE_EC, status, 0);
		goto error;
	}

	if (mbedtls_pk_get_type (key_ctx) != MBEDTLS_PK_RSA) {
		status = RSA_ENGINE_NOT_RSA_KEY;
		goto error;
	}

	mbedtls_rsa_set_padding (mbedtls_pk_rsa (*key_ctx), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1);
	priv_key->context = key_ctx;

	return 0;

error:
	rsa_free_key_context (key_ctx);
	return status;
}

static void rsa_release_key (struct rsa_engine *engine, struct rsa_private_key *key)
{
	if (engine && key) {
		rsa_free_key_context (key->context);
	}
}

static int tip_rsa_ncl_decrypt (struct rsa_engine *engine, const struct rsa_private_key *key,
	const uint8_t *encrypted, size_t in_length, const uint8_t *label, size_t label_length,
	enum hash_type pad_hash, uint8_t *decrypted, size_t out_length)
{
	uint8_t LE_encrypted[256] __attribute__ ((aligned (16)));
	uint32_t decryptMsgSize = 1000;
	NCL_RSA_PRV_KEY prvKey;
	struct tip_rsa_ncl_engine *tip_ncl = (struct tip_rsa_ncl_engine *) engine;
	NCL_STATUS_T ncl_status;

	if ((tip_ncl == NULL) || (key == NULL) || (encrypted == NULL) || (in_length == 0) ||
		(decrypted == NULL)) {
		return RSA_ENGINE_INVALID_ARGUMENT;
	}

	if (pad_hash > HASH_TYPE_SHA256) {
		return RSA_ENGINE_UNSUPPORTED_HASH_TYPE;
	}

#ifndef MBEDTLS_SHA1_C
	if (pad_hash == HASH_TYPE_SHA1) {
		return RSA_ENGINE_UNSUPPORTED_HASH_TYPE;
	}
#endif
#ifndef MBEDTLS_SHA256_C
	if (pad_hash == HASH_TYPE_SHA256) {
		return RSA_ENGINE_UNSUPPORTED_HASH_TYPE;
	}
#endif

	/*swap endianess of encrypted given buffer */
	memcpy (LE_encrypted, encrypted, 256);
	tip_mem_swap_endianness (LE_encrypted, NULL, 256);

	if (pad_hash == HASH_TYPE_SHA256) {
		mbedtls_rsa_set_padding (rsa_get_rsa_key (key), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
	}

	mbedtls_rsa_context *obj = rsa_get_rsa_key (key);

	uint8_t *modulus = (uint8_t *) (obj->N.p);
	uint8_t *exponent = (uint8_t *) (obj->D.p);

	prvKey.d.number = (uint32_t *) exponent;
	prvKey.d.size = sizeof (exponent) / sizeof (uint32_t);

	prvKey.modulus.number = (uint32_t *) modulus;
	prvKey.modulus.size = sizeof (modulus) / sizeof (uint32_t);

	NCL_STATUS_RET_CHECK (pROM_NCL_RSA_InitContext (tip_ncl->context));

	NCL_STATUS_RET_CHECK (pROM_NCL_RSA_Init (tip_ncl->context, NULL));

	NCL_STATUS_RET_CHECK (
		pROM_NCL_RSA_SetKey (tip_ncl->context, NULL, &prvKey, NCL_RSA_KEY_SIZE_2048));

	NCL_STATUS_RET_CHECK (
		pROM_NCL_RSA_Decrypt (tip_ncl->context, LE_encrypted, NCL_RSA_PADDING_OAEP,
			NCL_SHA_TYPE_2_256, label, label_length, decrypted, (uint32_t *) &decryptMsgSize));
	NCL_BUSY_WAIT_TIMEOUT ((pROM_NCL_RSA_GetStatus (tip_ncl->context) == NCL_STATUS_SYSTEM_BUSY),
		NCL_RESPONSE_COUNTER);
	DEFS_STATUS_COND_CHECK (
		(ncl_status = pROM_NCL_RSA_GetStatus (tip_ncl->context)) == NCL_STATUS_OK, ncl_status);

	/*swap endianess of decrypted output buffer */
	tip_mem_swap_endianness (decrypted, NULL, decryptMsgSize);

	return decryptMsgSize;
}
#endif

static int tip_rsa_ncl_sig_verify (struct rsa_engine *engine, const struct rsa_public_key *key,
	const uint8_t *signature, size_t sig_length, const uint8_t *match, size_t match_length)
{
	struct tip_rsa_ncl_engine *tip_ncl = (struct tip_rsa_ncl_engine *) engine;
	NCL_STATUS_T ncl_status;
	NCL_STATUS_T sigOk;
	NCL_RSA_PUB_KEY keyVerify;
	uint32_t sizeOfSignature = 256;
	/*signature and modulus must be little endian*/
	uint8_t LE_signature[256];
	uint8_t LE_modulus[256];
	/* exponent must be 2 dwords*/
	uint32_t pubExp[2];

	if ((engine == NULL) || (key == NULL) || (signature == NULL) || (match == NULL) ||
		(sig_length == 0) || (match_length == 0)) {
		return RSA_ENGINE_INVALID_ARGUMENT;
	}

	memcpy (LE_signature, signature, sizeOfSignature);
	tip_mem_swap_endianness (LE_signature, NULL, sizeOfSignature);
	memcpy (LE_modulus, (uint8_t *) key->modulus, sizeOfSignature);
	tip_mem_swap_endianness (LE_modulus, NULL, sizeOfSignature);

	pubExp[0] = key->exponent;
	pubExp[1] = 0x00;

	keyVerify.exponent.number = pubExp;
	keyVerify.exponent.size = sizeof (pubExp) / sizeof (uint32_t);
	keyVerify.modulus.number = (uint32_t *) LE_modulus;
	keyVerify.modulus.size = sizeof (key->modulus) / sizeof (uint32_t);

	NCL_STATUS_RET_CHECK (pROM_NCL_RSA_InitContext (tip_ncl->context));
	NCL_STATUS_RET_CHECK (pROM_NCL_RSA_Init (tip_ncl->context, NULL));
	NCL_STATUS_RET_CHECK (
		pROM_NCL_RSA_SetKey (tip_ncl->context, &keyVerify, NULL, NCL_RSA_KEY_SIZE_2048));

	NCL_STATUS_RET_CHECK (pROM_NCL_RSA_VerifyHash (tip_ncl->context, &sigOk, match,
		NCL_RSA_PADDING_PKCS1, NCL_SHA_TYPE_2_256, LE_signature));
	NCL_BUSY_WAITTIMEOUT ((pROM_NCL_RSA_GetStatus (tip_ncl->context) == NCL_STATUS_SYSTEM_BUSY),
		0x00FFFFFF);
	ncl_status = pROM_NCL_RSA_GetStatus (tip_ncl->context);
	if (ncl_status != NCL_STATUS_OK)
		return RSA_ENGINE_BAD_SIGNATURE;
	else
		return tip_ncl_status (ncl_status, RSA_ENGINE_VERIFY_FAILED);

	return 0;
}


/**
 * Initialize an mbedTLS RSA engine.
 *
 * @param engine The RSA engine to initialize.
 *
 * @return 0 if the RSA engine was successfully initialize or an error code.
 */
int tip_rsa_ncl_init (struct tip_rsa_ncl_engine *engine)
{
	int status = 0;
	if (engine == NULL) {
		return RSA_ENGINE_INVALID_ARGUMENT;
	}

	memset (engine, 0, sizeof (struct tip_rsa_ncl_engine));
#ifdef RSA_ENABLE_PRIVATE_KEY
	engine->base.generate_key = NULL;
	engine->base.init_private_key = rsa_init_private_key;
	engine->base.release_key = rsa_release_key;
	engine->base.get_private_key_der = NULL;
	engine->base.decrypt = tip_rsa_ncl_decrypt;
#endif
#ifdef RSA_ENABLE_DER_PUBLIC_KEY
	engine->base.init_public_key = NULL;
	engine->base.get_public_key_der = NULL;
#endif
	engine->base.sig_verify = tip_rsa_ncl_sig_verify;

	NCL_STATUS_RET_CHECK (pROM_NCL_RSA_InitContext (engine->context));
	NCL_STATUS_RET_CHECK (pROM_NCL_RSA_Power (engine->context, TRUE));

	status = pROM_NCL_RSA_Init (engine->context, NULL);

	return tip_ncl_status (status, RSA_ENGINE_HW_NOT_INIT);
}

/**
 * Release the resources used by an mbedTLS RSA engine.
 *
 * @param engine The RSA engine to release.
 */
void tip_rsa_ncl_release (struct tip_rsa_ncl_engine *engine)
{
	if (engine != NULL) {
		pROM_NCL_RSA_FinalizeContext (engine->context);
	}
}
