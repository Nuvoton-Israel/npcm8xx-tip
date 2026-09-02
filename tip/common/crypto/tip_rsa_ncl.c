/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/
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
#include "tip_rom_ncl_shared_state.h"

extern ROM_NCL_RSA_Init			   pROM_NCL_RSA_Init;
extern ROM_NCL_RSA_InitContext	   pROM_NCL_RSA_InitContext;
extern ROM_NCL_RSA_VerifyHash	   pROM_NCL_RSA_VerifyHash;
extern ROM_NCL_RSA_Decrypt		   pROM_NCL_RSA_Decrypt;
extern ROM_NCL_RSA_SetKey		   pROM_NCL_RSA_SetKey;
extern ROM_NCL_RSA_GetStatus	   pROM_NCL_RSA_GetStatus;
extern ROM_NCL_RSA_FinalizeContext pROM_NCL_RSA_FinalizeContext;
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
	uint32_t decrypt_msg_size = 1000;
	NCL_RSA_PRV_KEY prv_key;
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

	prv_key.d.number = (uint32_t *) exponent;
	prv_key.d.size = sizeof (exponent) / sizeof (uint32_t);

	prv_key.modulus.number = (uint32_t *) modulus;
	prv_key.modulus.size = sizeof (modulus) / sizeof (uint32_t);

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_RSA_InitContext (tip_ncl->context),
		tip_ncl->shared_state);

	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_RSA_Init (tip_ncl->context, NULL),
		tip_ncl->shared_state);

	NCL_STATUS_RET_CHECK_MUTEX (
		pROM_NCL_RSA_SetKey (tip_ncl->context, NULL, &prv_key, NCL_RSA_KEY_SIZE_2048),
		tip_ncl->shared_state);

	NCL_STATUS_RET_CHECK_MUTEX (
		pROM_NCL_RSA_Decrypt (tip_ncl->context, LE_encrypted, NCL_RSA_PADDING_OAEP,
			NCL_SHA_TYPE_2_256, label, label_length, decrypted, (uint32_t *) &decrypt_msg_size),
		tip_ncl->shared_state);
	{
		uint32_t __time = NCL_RESPONSE_COUNTER;
		do {
			if (__time-- == 0) {
				tip_ncl->shared_state->unlock (tip_ncl->shared_state);
				return NCL_STATUS_NO_RESPONSE;
			}
		} while (pROM_NCL_RSA_GetStatus (tip_ncl->context) == NCL_STATUS_SYSTEM_BUSY);
	}
	ncl_status = pROM_NCL_RSA_GetStatus (tip_ncl->context);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);
	DEFS_STATUS_COND_CHECK (ncl_status == NCL_STATUS_OK, ncl_status);

	/*swap endianess of decrypted output buffer */
	tip_mem_swap_endianness (decrypted, NULL, decrypt_msg_size);

	return decrypt_msg_size;
}
#endif

static int tip_rsa_ncl_sig_verify (struct rsa_engine *engine, const struct rsa_public_key *key,
	const uint8_t *signature, size_t sig_length, enum hash_type sig_hash, const uint8_t *match, size_t match_length)
{
	struct tip_rsa_ncl_engine *tip_ncl = (struct tip_rsa_ncl_engine *) engine;
	NCL_STATUS_T ncl_status;
	NCL_STATUS_T sig_ok;
	NCL_RSA_PUB_KEY key_verify;
	uint32_t size_of_signature = RSA_MAX_KEY_LENGTH;
	/*signature and modulus must be little endian*/
	uint8_t LE_signature[RSA_MAX_KEY_LENGTH];
	uint8_t LE_modulus[RSA_MAX_KEY_LENGTH];
	/* exponent must be 2 dwords*/
	uint32_t pub_exp[2];
	NCL_SHA_TYPE_T hash_alg = NCL_SHA_TYPE_2_256;
	NCL_RSA_KEY_SIZE_T key_size = NCL_RSA_KEY_SIZE_2048;
	
	if ((engine == NULL) || (key == NULL) || (signature == NULL) || (match == NULL) ||
			(sig_length == 0) || (match_length == 0)) {
			return RSA_ENGINE_INVALID_ARGUMENT;
	}
	switch (sig_hash) {
		case HASH_TYPE_SHA256:
			hash_alg = NCL_SHA_TYPE_2_256;
			break;	
		case HASH_TYPE_SHA384:
			hash_alg = NCL_SHA_TYPE_2_384;
			break;

		case HASH_TYPE_SHA512:
			hash_alg = NCL_SHA_TYPE_2_512;
			break;	
		
		default:
			return	RSA_ENGINE_UNSUPPORTED_SIG_TYPE;	

	}
	
	if (sig_length != key->mod_length) {
		return RSA_ENGINE_BAD_SIGNATURE;
	}
	
	switch (key->mod_length) {
		case RSA_KEY_LENGTH_2K:
			key_size =  NCL_RSA_KEY_SIZE_2048;
			size_of_signature = RSA_KEY_LENGTH_2K;
			break;
			
		case RSA_KEY_LENGTH_3K:
			key_size =  NCL_RSA_KEY_SIZE_3072;
			size_of_signature = RSA_KEY_LENGTH_3K;
			break;
			
		/*reserved*/
		/*case RSA_KEY_LENGTH_4K:
		key_size =  NCL_RSA_KEY_SIZE_4096;
		size_of_signature = RSA_KEY_LENGTH_4K;*/
			break;
		
		default:
			return RSA_ENGINE_UNSUPPORTED_KEY_LENGTH;
	}

	memcpy (LE_signature, signature, size_of_signature);
	tip_mem_swap_endianness (LE_signature, NULL, size_of_signature);
	memcpy (LE_modulus, (uint8_t *) key->modulus, size_of_signature);
	tip_mem_swap_endianness (LE_modulus, NULL, size_of_signature);

	pub_exp[0] = key->exponent;
	pub_exp[1] = 0x00;

	key_verify.exponent.number = pub_exp;
	key_verify.exponent.size = sizeof (pub_exp) / sizeof (uint32_t);
	key_verify.modulus.number = (uint32_t *) LE_modulus;
	key_verify.modulus.size = sizeof (key->modulus) / sizeof (uint32_t);

	tip_ncl->shared_state->lock (tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_RSA_InitContext (tip_ncl->context),
		tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_RSA_Init (tip_ncl->context, NULL),
		tip_ncl->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (
		pROM_NCL_RSA_SetKey (tip_ncl->context, &key_verify, NULL, key_size ),
		tip_ncl->shared_state);

	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_RSA_VerifyHash (tip_ncl->context, &sig_ok, match,
		NCL_RSA_PADDING_PKCS1, hash_alg, LE_signature), tip_ncl->shared_state);
	{
		uint32_t __time = 0x00FFFFFF;
		do {
			if (__time-- == 0) {
				tip_ncl->shared_state->unlock (tip_ncl->shared_state);
				return NCL_STATUS_NO_RESPONSE;
			}
		} while (pROM_NCL_RSA_GetStatus (tip_ncl->context) == NCL_STATUS_SYSTEM_BUSY);
	}
	ncl_status = pROM_NCL_RSA_GetStatus (tip_ncl->context);
	tip_ncl->shared_state->unlock (tip_ncl->shared_state);
	if (ncl_status != NCL_STATUS_OK)
	{
		return RSA_ENGINE_BAD_SIGNATURE;
	}
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
int tip_rsa_ncl_init (struct tip_rsa_ncl_engine *engine, struct tip_rom_ncl_shared_state *shared_state)
{
	int status = 0;
	if (engine == NULL) {
		return RSA_ENGINE_INVALID_ARGUMENT;
	}
	if (shared_state == NULL) {
		return RSA_ENGINE_INVALID_ARGUMENT;
	}

	memset (engine, 0, sizeof (struct tip_rsa_ncl_engine));
	engine->shared_state = shared_state;
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

	engine->shared_state->lock (engine->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_RSA_InitContext (engine->context),
		engine->shared_state);
	NCL_STATUS_RET_CHECK_MUTEX (pROM_NCL_RSA_Power (engine->context, TRUE),
		engine->shared_state);

	status = pROM_NCL_RSA_Init (engine->context, NULL);
	engine->shared_state->unlock (engine->shared_state);

	return tip_ncl_status (status, RSA_ENGINE_HW_NOT_INIT);
}

/**
 * Release the resources used by an mbedTLS RSA engine.
 *
 * @param engine The RSA engine to release.
 */
void tip_rsa_ncl_release (struct tip_rsa_ncl_engine *engine)
{
	if (engine != NULL && engine->shared_state) {
		engine->shared_state->lock (engine->shared_state);
		pROM_NCL_RSA_FinalizeContext (engine->context);
		engine->shared_state->unlock (engine->shared_state);
	}
}
