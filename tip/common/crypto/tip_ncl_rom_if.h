/*  Nuvoton Technology Corporation confidential
 *  Copyright (c) 2021 by Nuvoton Technology Corporation
 *  All rights reserved
 *<<<---------------------------------------------------------------------------------
 * File Contents:
 *   tip_ncl_rom_if.h
 *            This file contains Nuvoton Cryptographic Library (NCL) interface
 * Project:
 *            SWC HAL
 */

#ifndef _NCL_IF_H
#define _NCL_IF_H

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#include <stdint.h>

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Please notice all parameters and structures should be 4B aligned.                                       */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*                                                  MISC                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/* API Version                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define NCL_VER_MAJOR 2
#define NCL_VER_MINOR 3
#define NCL_VER_INTERNAL 3

#define NCL_MODULE_NAME "Nuvoton Cryptographic Library v2.3 Arbel ROM"

/** Big integer structure.
 *
 */
typedef struct NCL_BN_T {
	/// This field contains the big-integer value.
	uint32_t *number;
	/// This filed contains the size of the big-integer in DWORD units.
	uint32_t size;
} NCL_BN_T;

/** Error status.
 *
 */
typedef enum {
	NCL_STATUS_OK_Z1 = 0,
	//#if defined NCL_LARGE_HAMMING_DISTANCE
	/// Status OK.
	NCL_STATUS_OK = 0xA5A5,
	/// Status fail.
	NCL_STATUS_FAIL = 0x5A5A,
	//#else
	//    NCL_STATUS_OK                       = 0x00,
	//    NCL_STATUS_FAIL                     = 0x01,
	//#endif
	/// Invalid parameter.
	NCL_STATUS_INVALID_PARAM = 0x02,
	/// Parameter not supported.
	NCL_STATUS_PARAM_NOT_SUPPORTED,
	/// System busy.
	NCL_STATUS_SYSTEM_BUSY,
	/// Authentication fail.
	NCL_STATUS_AUTHENTICATION_FAIL,
	/// No response.
	NCL_STATUS_NO_RESPONSE,
	/// Hardware error.
	NCL_STATUS_HARDWARE_ERROR,
	/// self test error.
	NCL_STATUS_SYSTEM_NOT_INITIALIZED,
} NCL_STATUS_T;

/** NCL FIPS mode.
 *
 */
typedef enum {
	/// FIPS mode.
	NCL_FIPS_MODE = 0xA5A5,
	/// Non-FIPS mode.
	NCL_NON_FIPS_MODE = 0x5A5A,
} NCL_FIPS_MODE_T;

/** Module description structure.
 *
 */
typedef struct NCL_MODULE_DESC_T {
	/// This field contains the module name (string).
	char moduleName[64];
	/// This field contains the SW version.
	uint32_t sw_version;
	/// This field contains the NCL approved mode indicator.
	NCL_STATUS_T approvedMode;
	/// This field contains the NCL FIPS mode indicator.
	NCL_FIPS_MODE_T fipsMode;
} NCL_MODULE_DESC_T;

/** Algorithm selftests.
 *
 */
typedef enum {
	/// SHA algorithm.
	NCL_ALGO_SHA = 0x00,
	/// DRBG algorithm.
	NCL_ALGO_DRBG,
	/// AES algorithm.
	NCL_ALGO_AES,
	/// RSA algorithm.
	NCL_ALGO_RSA,
	/// ECC algorithm.
	NCL_ALGO_ECC,
	/// All algorithm.
	NCL_ALGO_ALL,
} NCL_ALGO_T;

/** User handler.
 *
 */
typedef void (*NCL_USR_HANDLER) (NCL_STATUS_T status);

/** User callback.
 *
 */
typedef void (*NCL_USR_CALLBACK) (void);

/*---------------------------------------------------------------------------------------------------------*/
/*                                                   RSA                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef NCL_RSA
#define NCL_RSA_CONTEXT_BYTE_SIZE (0x258) // in A1: (0x9C)
#define NCL_RSA_MAX_KEY_SIZE RSA_KEY_SIZE_MAX

/** RSA padding types.
 *
 */
typedef enum {
	/// OAEP padding. according to PKCS spec #1 V 1.5, relevant for RSA encrypt/decrypt.
	NCL_RSA_PADDING_OAEP = 0,
	/// PKCS1 padding. according to PKCS spec #1 V 1.5, relevant for RSA sign/verify .
	NCL_RSA_PADDING_PKCS1,
	/// PKCS1 padding. according to PKCS spec#1 V 1.5, relevant for sign/verify.
	NCL_RSA_PADDING_PSS,
	/// Number of padding types, in use for error checking.
	NCL_RSA_PADDING_NUM
} NCL_RSA_PADDING_T;

/** RSA private key structure.
 *
 */
typedef struct NCL_RSA_PRV_KEY {
	/// Private exponent, in little endian format
	NCL_BN_T d;
	/// d mod (p - 1)
	NCL_BN_T dp;
	/// d mod (q - 1)
	NCL_BN_T dq;
	/// Inverse q
	NCL_BN_T inverseQ;
	/// Modulus (n), in little endian format
	NCL_BN_T modulus;
	/// p
	NCL_BN_T p;
	/// q
	NCL_BN_T q;
	/// CRT format
	uint8_t isCRTformat;
} NCL_RSA_PRV_KEY;

/** RSA public key structure.
 *
 */
typedef struct NCL_RSA_PUB_KEY {
	/// Public exponent (e), in little endian format. Size must be 2 dwords, pad with zeros if smaller
	NCL_BN_T exponent;
	/// Modulus (n), in little endian format
	NCL_BN_T modulus;
} NCL_RSA_PUB_KEY;

/** RSA key sizes.
 *
 */
typedef enum {
	/// RSA key size 2048b
	NCL_RSA_KEY_SIZE_2048 = 0,
	/// RSA key size 3072b
	NCL_RSA_KEY_SIZE_3072 = 1,
	/// RSA key size 4096b
	//NCL_RSA_KEY_SIZE_4096      = 2, //reserved
} NCL_RSA_KEY_SIZE_T;

#endif // NCL_RSA

/*---------------------------------------------------------------------------------------------------------*/
/*                                                   DRBG                                                  */
/*---------------------------------------------------------------------------------------------------------*/

#ifdef NCL_DRBG
/**
   This enum defines the security strengths supported by this DRBG mechanism.
   The internally generated entropy and nonce sizes are derived from these values.
   The supported actual sizes:
		Security strength (bits)    112 128 192 256 128_Test 256_Test

		Entropy size (Bytes)        32  48  64  96  111      128
		Nonce size (Bytes)          16  16  24  32  16       0
*/
typedef enum {
	NCL_DRBG_SECURITY_STRENGTH_112b = 0,
	NCL_DRBG_SECURITY_STRENGTH_128b,
	NCL_DRBG_SECURITY_STRENGTH_192b,
	NCL_DRBG_SECURITY_STRENGTH_256b,
	NCL_DRBG_SECURITY_STRENGTH_128b_TEST,
	NCL_DRBG_SECURITY_STRENGTH_256b_TEST,
	NCL_DRBG_MAX_SECURITY_STRENGTH
} NCL_DRBG_SECURITY_STRENGTH_T;

#define NCL_DRBG_CONTEXT_BYTE_SIZE (0xF0)
#define NCL_DRBG_DEFAULT_SECURITY_STRENGTH NCL_DRBG_SECURITY_STRENGTH_256b
#endif // NCL_DRBG

/*---------------------------------------------------------------------------------------------------------*/
/*                                                   AES                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef NCL_AES

#define NCL_AES_CONTEXT_BYTE_SIZE (0x0C)

/** AES key sizes.
 *
 */
typedef enum {
	/// AES key size 128b
	NCL_AES_KEY_SIZE_128 = 0,
	/// AES key size 192b
	NCL_AES_KEY_SIZE_192 = 1,
	/// AES key size 256b
	NCL_AES_KEY_SIZE_256 = 2,
} NCL_AES_KEY_SIZE_T;

/** AES modes.
 *
 */
typedef enum {
	/// AES mode ECB
	NCL_AES_MODE_ECB = 0,
	/// AES mode CBC
	NCL_AES_MODE_CBC = 1,
	/// AES mode CTR
	NCL_AES_MODE_CTR = 2,
	/// AES mode MAC
	NCL_AES_MODE_MAC = 3,
	/// AES mode CFB
	NCL_AES_MODE_CFB = 4,
	/// AES mode OFB
	NCL_AES_MODE_OFB = 5,
	/// AES mode CCM
	NCL_AES_MODE_CCM = 6,
	/// AES mode GCM
	NCL_AES_MODE_GCM = 7
} NCL_AES_MODE_T;

/** AES GCM mac optional sizes.
 *
 */
typedef enum {
	/// AES GCM mac size 4B
	NCL_AES_GCM_MAC_SIZE_4B,
	/// AES GCM mac size 8B
	NCL_AES_GCM_MAC_SIZE_8B,
	/// AES GCM mac size 12B
	NCL_AES_GCM_MAC_SIZE_12B,
	/// AES GCM mac size 13B
	NCL_AES_GCM_MAC_SIZE_13B,
	/// AES GCM mac size 14B
	NCL_AES_GCM_MAC_SIZE_14B,
	/// AES GCM mac size 15B
	NCL_AES_GCM_MAC_SIZE_15B,
	/// AES GCM mac size 16B
	NCL_AES_GCM_MAC_SIZE_16B
} NCL_AES_GCM_MAC_SIZE_T;

/** AES CCM mac optional sizes.
 *
 */
typedef enum {
	/// AES CCM mac size 4B
	NCL_AES_CCM_MAC_SIZE_4B,
	/// AES CCM mac size 6B
	NCL_AES_CCM_MAC_SIZE_6B,
	/// AES CCM mac size 8B
	NCL_AES_CCM_MAC_SIZE_8B,
	/// AES CCM mac size 10B
	NCL_AES_CCM_MAC_SIZE_10B,
	/// AES CCM mac size 12B
	NCL_AES_CCM_MAC_SIZE_12B,
	/// AES CCM mac size 14B
	NCL_AES_CCM_MAC_SIZE_14B,
	/// AES CCM mac size 16B
	NCL_AES_CCM_MAC_SIZE_16B
} NCL_AES_CCM_MAC_SIZE_T;

/** AES oprtation type.
 *
 */
typedef enum {
	/// AES operation encrypt
	NCL_AES_OP_ENCRYPT,
	/// AES operation decrypt
	NCL_AES_OP_DECRYPT
} NCL_AES_OP_T;

/** AES additional data structure.
 *
 */
typedef struct {
	/// Authenticated data pointer
	uint32_t *authData;
	/// Authenticated data size in bytes. Should be 4B aligned.
	uint32_t authSize;
	/// Input vector size in bytes
	uint32_t ivSize;
	/// mac size of type NCL_AES_GCM_MAC_SIZE_T/NCL_AES_CCM_MAC_SIZE_T
	uint32_t macSize;
} NCL_AES_ADD_DATA;

#endif // NCL_AES

#if defined NCL_SHA || defined NCL_RSA || defined NCL_ECC
/** SHA types.
 *
 */
typedef enum {
	/// SHA2 256b
	NCL_SHA_TYPE_2_256 = 0,
	/// SHA2 384b
	NCL_SHA_TYPE_2_384 = 1,
	/// SHA2 512b
	NCL_SHA_TYPE_2_512 = 2,
	NCL_SHA_TYPE_NUM
} NCL_SHA_TYPE_T;

/** SHA modules.
 *
 */
typedef enum
{
    /// SHA module 0
    NCL_SHA_MODULE_0           = 0,
    /// SHA module 1
    NCL_SHA_MODULE_1           = 1,
    NCL_SHA_MODULE_NUM
} NCL_SHA_MODULE_T;

#endif //  NCL_SHA || defined NCL_RSA || defined NCL_ECC


/*---------------------------------------------------------------------------------------------------------*/
/*                                                   SHA                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef NCL_SHA

#define NCL_SHA_CONTEXT_BYTE_SIZE (0xD4)

/** HMAC key structure.
 *
 */
typedef struct NCL_SHA_HMAC_KEY {
	/// Key pointer
	const uint8_t *key;
	/// Key length
	uint32_t keyLen;
} NCL_SHA_HMAC_KEY;

/** HKDF modes.
 *
 */
typedef enum {
	/// HKDF counter mode
	NCL_SHA_HKDF_MODE_COUNTER = 0,
	/// HKDF feedback mode
	NCL_SHA_HKDF_MODE_FEEDBACK,
	/// HKDF double pipeline mode
	NCL_SHA_HKDF_MODE_DOUBLE_PIPELINE,
} NCL_SHA_HKDF_MODE_T;

/** HKDF input parameters structure.
 *
 */
typedef struct {
	/// HKDF mode
	NCL_SHA_HKDF_MODE_T hkdfMode;
	/// SHA type
	NCL_SHA_TYPE_T shaType;
	/// Input key pointer
	uint8_t *keyInput;
	/// input key size
	uint32_t keyInputSize;
	/// Fixed data pointer
	uint8_t *fixedData;
	/// Fixed data size
	uint32_t fixedDataSize;
	/// IV pointer
	uint8_t *iv;
	/// IV size
	uint32_t ivSize;
} NCL_SHA_HKDF_INPUT_T;
#endif // NCL_SHA

/*---------------------------------------------------------------------------------------------------------*/
/*                                                   ECC                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef NCL_ECC

#define NCL_ECC_CONTEXT_BYTE_SIZE (0x13C)

/** ECC affine point.
 *
 */
typedef struct NCL_ECC_POINT_T {
	/// ECC point X coordinate
	NCL_BN_T X;
	/// ECC point Y coordinate
	NCL_BN_T Y;
} NCL_ECC_POINT_T;

/** ECC curve data.
 *
 */
typedef struct NCL_ECC_CURVE_DATA_T {
	///  A prime number
	NCL_BN_T p;
	///linear coefficient
	NCL_BN_T a;
	/// constant term
	NCL_BN_T b;
	///generator x coordinate (Gx)
	NCL_BN_T x;
	///generator y coordinate (Gy)
	NCL_BN_T y;
	/// the order of the curve
	NCL_BN_T n;
	/// cofactor
	uint8_t h;
} NCL_ECC_CURVE_DATA_T;

/** Predefined ECC curves selector enum.
 *
 */
typedef enum {
	NCL_ECC_CURVE_NIST_P_256 = 0,
	NCL_ECC_CURVE_NIST_P_384,
	NCL_ECC_CURVE_NIST_P_521,
} NCL_ECC_CURVE_T;

/** KDF function pointer.
 *
 */
typedef NCL_STATUS_T (*NCL_ECC_KdfFuncPtr) (void *, uint8_t *, uint32_t);
#endif // NCL_ECC

/*---------------------------------------------------------------------------------------------------------*/
/*                                                   DFT                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef NCL_DFT

#define DRBG_DFT

/** NCL DFT MISC state
 *
 */
typedef enum {
	/// No misc bypass
	NCL_DFT_MISC_STATUS_NOT_SET = 0,

	// Selftest status is NCL_STATUS_OK
	NCL_DFT_MISC_STATUS_OK = NCL_STATUS_OK,

	/// Selftest status is NCL_STATUS_FAIL
	NCL_DFT_MISC_STATUS_FAIL = NCL_STATUS_FAIL,
} NCL_DFT_MISC_STATUS_T;

/** NCL DFT ECC key state
 *
 */
typedef enum {
	/// No DFT key change
	NCL_DFT_ECC_KEY_PAIR_OK = 0,

	/// Alternate ECC private key
	NCL_DFT_ECC_PRV_KEY_ALTER,

	// Alternate ECC public key
	NCL_DFT_ECC_PUB_KEY_ALTER,
} NCL_DFT_ECC_T;

/** DFT KAT corruption
 *
 */
typedef enum {
	/// No KAT corruption
	NCL_DFT_KAT_NONE = 0,
	/// SHA KAT corruption
	NCL_DFT_KAT_SHA,
	/// HMAC KAT corruption
	NCL_DFT_KAT_SHA_HMAC,
	/// HKDF KAT corruption
	NCL_DFT_KAT_SHA_HKDF,
	/// DRBG instanciate KAT corruption
	NCL_DFT_KAT_DRBG_INSTANCIATE,
	/// DRBG reseed KAT corruption
	NCL_DFT_KAT_DRBG_RESEED,
	/// DRBG generation KAT corruption
	NCL_DFT_KAT_DRBG_GENERATE,
	/// AES CBC decryption KAT corruption
	NCL_DFT_KAT_AES_CBC_DEC,
	/// AES CCM encryption KAT corruption
	NCL_DFT_KAT_AES_CCM_ENC,
	/// ECC key exchange KAT corruption
	NCL_DFT_KAT_ECC_KEY_EXCHAGNE,
	/// ECC sign KAT corruption
	NCL_DFT_KAT_ECC_SIGN,
	/// ECC verify KAT corruption
	NCL_DFT_KAT_ECC_VERIFY,
	/// RSA sign KAT corruption
	NCL_DFT_KAT_RSA_SIGN,
	/// RSA verify KAT corruption
	NCL_DFT_KAT_RSA_VERIFY
} NCL_DFT_KAT_T;
#endif //NCL_DFT

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                   DFT                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifdef NCL_DFT
/**
  This routine initializes NCL DFT.\n
  It should be called before any NCL API is called.\n
  @return None.\n
*/
void NCL_DFT_Init (void);

/**
  This routine sets the NCL DFT selftest status which allows changing the MISC status\n
  It should be called before any NCL_MISC* API.\n
  This DFT can be used in order to bypass a specific algorithm self-test completion, or all\n
  of them (by calling this function with NCL_DFT_MISC_STATUS_OK), or to simulate any other\n
  self-test status for test purposes\n
  @param algo                 algorithm status to set  [in]              \n
  @param status               self-test status to set  [in]              \n
  @return None\n
 */
void NCL_DFT_MISC_SetStatus (NCL_ALGO_T algo, NCL_DFT_MISC_STATUS_T status);

/**
  This DFT can be used in order to test the failure of any KAT.\n
  This routine sets the NCL_DFT_KAT_corruption_l to change a KAT buffer of a certain selftest algorithm.\n
  The corruption will take effect in the next selftest after calling this API.\n
  Passing NCL_DFT_KAT_NONE will set it back to the regular mode (no KAT corruption).\n
  @param kat            a KAT that needed to be fail   [in] \n
  @return None\n
 */
void NCL_DFT_MISC_FailKAT (NCL_DFT_KAT_T kat);

/**
  This DFT returns the status of a service KAT.\n
  If the service KAT was not run yet, the DFT will return NCL_STATUS_SYSTEM_NOT_INITIALIZED.\n
  If it has ran and passed, the DFT will return NCL_STATUS_OK. If the NCL is in fail mode (either\n
  because this KAT failed or any other KAT failed), the DFT will return NCL_STATUS_FAIL.\n

  @param kat            A KAT status to return         [in] \n
  @return NCL_STATUS_SYSTEM_NOT_INITIALIZED when this KAT has not been performed yet, NCL_STATUS_OK for
		  successful completion, NCL_STATUS_FAIL for failure, NCL_STATUS_INVALID_PARAM for wrong paramater.
 */
NCL_STATUS_T NCL_DFT_MISC_GetKatStatus (NCL_DFT_KAT_T kat);

/**
  This routine sets the NCL DFT ECC key which allows changing the ECC generated key pair. \n
  It should be called before NCL_ECC_GenKey API.\n
  This DFT can be used in order to test the failure of ECC key generation
  Changing the private/public key before ECDSA sign/verify test is performed on the key pair.
  @param keyChange               self-test status to set  [in]              \n
  @return None\n
 */
void NCL_DFT_ECC_SetKeyChange (NCL_DFT_ECC_T keyChange);

/**
  This routine sets the NCL DFT DRBG manual reseed state which returns DRBG_RESEED_REQUIERD . \n
  status from DRBG_Generate instead of internally reseeding.\n
  This DFT can be used in order to verify reseed_required flag functionality.
  @param manualReseed          Manual reseed status to set  [in]              \n
  @return None\n
 */
void NCL_DFT_DRBG_SetManualReseed (BOOLEAN manualReseed);

/**
  This routine copies the internally generated entropy buffer and size to an external buffer. \n
  This DFT should be used after a call to NCL_DRBG_Instantiate or NCL_DRBG_Reseed. \n
  Otherwise entropy data might be out of date. \n
  The function returns a pointer to the RNG buffer. \n
  @param buffer          Pointer to output buffer       [out]   \n
  @param entSize         Configured entropy size        [out]   \n
  @param nonce           Pointer to nonce output buffer [out]   \n
  @param nonceSize       Configured nonce size          [out]   \n
  @return a pointer to the latest RNG data that was generated   \n
 */
void* NCL_DFT_DRBG_GetHwEntropy (uint8_t* entBuffer,   uint32_t* entSize,
								 uint8_t* nonceBuffer, uint32_t* nonceSize);

/**
  This DFT routine generates entropy of 'size' bytes using the RNG HW, and, if specified,
  runs the DRBG healthtest on them. \n
  @param powerOff        Power off RNG after use                [in]
  @param entropy         Pointer to output buffer               [out] \n
  @param size            number of bytes to generate            [in]  \n
  @param verify          Run healthtest on the generated bytes  [in]  \n

  @return NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error  \n
 */
NCL_STATUS_T NCL_DFT_DRBG_GenerateAndTestEntropy (BOOLEAN powerOff, uint8_t *entropy, uint32_t size,
												  BOOLEAN verify);
#endif //NCL_DFT

/*! \addtogroup NCL MISC
   *  NCL MISC APIs.\n
   *  Supports FIPS required self tests routines.\n
   *  @{
   */

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                  MISC                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/**
  This routine returns NCL self-test status of the specified module (or all of them).\n
  A NCL module has three operational modes:\n
   a.  Not initialized: the initialization function didn’t complete (successfully) yet.
   b.  FIPS approved mode: the POST ran and passed.
   c.  Failure mode: the self-test failed at some point of time (either during POST or when called on demand);\n
	   in order to return to FIPS approved mode, the self-test must re-run and complete successfully.
  @param algo            Algorithm selftest status to return            [in]     \n
  @return NCL_STATUS_SYSTEM_BUSY if self-test is not done yet, NCL_STATUS_OK for \n
  successful completion and NCL_STATUS_SYSTEM_NOT_INITIALIZED for error.
*/
NCL_STATUS_T NCL_MISC_GetStatus (NCL_ALGO_T algo);

/**
  This routine runs NCL self-test of a specified algorithm or of all the algorithms.        \n
  This routine can run any of the following self-tests (or all of them, see NCL_ALGO_T):    \n
  1. AES:                                                                                   \n
	a. AES encrypt (compare to known answer) and                                            \n
	   Decrypt(compare to input data) with following modes:                                 \n
	  1. CMAC/CCM/GCM (one mode only) encrypt mode with key size 128                        \n
	  2. OFB/ECB/CBC/CTR(one mode only) decrypt mode with key size 128                      \n
  2. RSA:                                                                                   \n
	a. Encrypt (compare to known answer) and decrypt                                        \n
	   (compare to input data) with OAEP padding key size                                   \n
	   3072/2048 and SHA size 256/384/512 (test for each approved key size and hash)        \n
	b. Verify with PKCS padding key size 3072/2048 and                                      \n
	   SHA size 256/384/512 (test for each approved key size and hash)                      \n
	   (no need to test PSS padding as well)                                                \n
  3. SHA:                                                                                   \n
	a. SHA2 256 (no need for SHA 384/512)                                                   \n
	b. HMAC with SHA2-512                                                                   \n
	c. HKDF counter mode using HMAC256                                                      \n
  4. ECC:                                                                                   \n
	a. ECDH known answer test with key size 256 (with any approved curve)                   \n
	b. ECDSA sign & verify with key size 256 (predefined key)                               \n
  5. DRBG:                                                                                  \n
	a. DRBG initiate with known entropy, DRBG generate(self-test mode)                      \n
	   and compare to known buffer.                                                         \n
	b. DRBG reseed, DRBG Generate(self test mode) and compare to known buffer               \n
	c. Verify output buffers from a,b tests are not equal.                                  \n
	d. Run uninitiate                                                                       \n
	* All above is for SHA size 512(need to run above for each SHA size supported)          \n
	* Notice: The known-answer tests shall be performed for each implemented                \n
	  security_strength                                                                     \n
  Note: All the module that are going to be used by this function should be activated and   \n
		initialized prior to this operation by the caller.                                  \n
	For SHA  - Power-on SHA                                                                 \n
	For DRBG - Power-on SHA and DRBG                                                        \n
	For ECC  - Power-on SHA and ECC                                                         \n
	For RSA  - Power-on SHA and RSA                                                         \n
	For AES  - Power-on AES                                                                 \n
	When running all selftests, all the modules should be powered on                        \n
  @param algo            Algorithm selftest to run                      [in]                \n
  @param userHandler     Handler to be called when self test is done.   [in]                \n
		 If userHandler equals NULL. user should poll on status using NCL_FIPS_GetStatus routine. [in]
  @return NCL_STATUS_OK for successful completion and NCL_STATUS_FAIL for failure.
 */
NCL_STATUS_T NCL_MISC_SelfTest (NCL_ALGO_T algo, NCL_USR_HANDLER userHandler);

/**
  This routine handles tamper detection by the application firmware .\n
  in case the firmware detects a tamper attack it should call this routine in order to to clear secrets
  and block further services till the chip returns to a safe working mode.\n
  this routine performs the following:\n
  1.  Clears secrets: the NCL keeps only one secret across functions –
	  the last used AES key; this is done due to performance considerations
	  (allows to perform consecutive operations on the same key without the need to reload the key every time);
	  It clears this key by calling NCL_AES_ClearParams ().\n
  2. Blocks further services till re-initialization (and completion of a successful ST) of the services.
  @return None
*/
void NCL_MISC_TamperDetected (void);

/**
  This routine returns a module secription structure that contains the module name, its version\n
  and the approved mode indicator.\n
  @param description        A pointer to the module description structure   [out]
  @return NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error           \n
*/
NCL_STATUS_T NCL_MISC_GetModuleDescription (NCL_MODULE_DESC_T *description);

/*! @} */

#ifdef NCL_DRBG
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                   DRBG                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*! \addtogroup DRBG
   *  DRBG APIs                                                  \n
   * Supports deterministic random bit generator                 \n
   *  @{
   */

/**
  This routine returns the DRBG context size.
 * @return  DRBG context size
 */
uint32_t NCL_DRBG_GetContextSize (void);

/**
  This routine initializes the DRBG context, the context area should be allocated
  by the user prior calling to this function.
 * @param context              pointer to DRBG context [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_DRBG_InitContext (void *context);

/**
  This routine power on/off DRBG module
 * @param context              pointer to DRBG context                 [in]
 * @param on                   True for power on, False otherwise      [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_DRBG_Power (void *context, uint8_t on);

/**
  This routine finalize the DRBG context, it should be called after DRBG operation is
  finished
 * @param context              pointer to DRBG context [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_DRBG_FinalizeContext (void *context);

/**
  Clear all DRBG SSPs (Sensitive Security Parameters) in HW and in driver level.\n
  This function will be called once SSP zeroisation is required.\n
  The routine clears all internal varibales, zeroizes all inputs and activates
  the HW module with zeroized inputs.
 * @param context              Pointer to DRBG context   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_DRBG_Clear (void *context);

/**
  This routine initialize DRBG
  This function should be called once at initiate time and before any other
  operation of this module's APIs.\n
  This function will run selftest automatically if wasn't run yet.
 * @param context              pointer to DRBG context   [in]
 * @param intEnable            Interrupt enable mode     [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_DRBG_Init (void *context, BOOLEAN intEnable);

/**
	This routine configures DRBG

 * @param context              pointer to DRBG context                                [in]
 * @param reseedInterval       updated Number of generations allowed until reseeding. [in]
 * @param predResistance       1/0 to enable/disable prediction resistance.           [in]

 * @return  NCL_STATUS_OK on success and other NCL_STATUS_* on error
 */
NCL_STATUS_T NCL_DRBG_Config (
	void*       context,
	uint32_t    reseedInterval,
	uint8_t     predResistance
);

/**
	This routine creates a first instantiation of the DRBG mechanism parameters.\n
	The routine pulls an initial seed from the HW RNG module and resets the reseed counter.\n
	DRBG and SHA modules should be activate prior to the this operation. \n
	reseedInterval is set to default value.\n
	predResistance is set to FALSE by default since the RNG provides sufficient entropy.

 * @param context              pointer to DRBG context                                       [in]
 * @param securityStrength     Security strength level                                       [in]
*/
#ifdef NCL_DFT
/**
 * @param entropy              Pointer to external entropy. \n
							If equals NULL - will generate entropy internally.            [in]
							Entropy size is always derived from the security strength.    [in]
 * @param nonce                A nonce used for seed creation alongside HW entropy input.\n
							If equals NULL - will generate a nonce internally. \n
							Nonce size is always derived from the security strength.      [in]
*/
#endif
/**
 * @param persString           Optional. Personalization string input to add security level.\n
							Set to Null in order to get full seed from HW.                [in]
 * @param persStringLen        Personalization string length in bytes. Set to 0 if not used. [in]

 * @return  NCL_STATUS_OK on success and other NCL_STATUS_* on error
 */
NCL_STATUS_T NCL_DRBG_Instantiate (
	void*                                 context,
	NCL_DRBG_SECURITY_STRENGTH_T          securityStrength,
#ifdef NCL_DFT
	const uint8_t*                        entropy,
	const uint8_t*                        nonce,
#endif
	const uint8_t*                        persString,
	uint32_t                              persStringLen
);

/**
  This routine performes DRBG un instantiate

 * @param context              pointer to DRBG context [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_DRBG_UnInstantiate (void *context);

/**
	This routine reseeds the internal state of the given instantce.\n
	Additional input can increase security level but is optional.
	DRBG and SHA modules should be activate prior to the this operation. \n

 * @param context              pointer to DRBG context.\n
							Use NULL in order to reseed the state of the shared NCL DRBG context [in]
*/
#ifdef NCL_DFT
/**
 * @param entropy              Pointer to external entropy.\n
							If equals NULL - will generate entropy internally.                   [in]
*/
#endif
/**
 * @param addData              Additional data for reseeding (optional). set to NULL if unused.     [in]
 * @param addDataLen           Additional data length in bytes.                                     [in]

 * @return  NCL_STATUS_OK on success and other NCL_STATUS_* on error
		This function may only be called after NCL_DRBG_Instantiate() was called for the same
		context. Otherwise returns DEFS_STATUS_FAIL.
 */
NCL_STATUS_T NCL_DRBG_Reseed (
	void*                        context,
#ifdef NCL_DFT
	const uint8_t*               entropy,
#endif
	const uint8_t*               addData,
	uint32_t                     addDataLen
);

/**
	This routine generates a random number from the current internal state.\n
	With prediction resistance enabled, reseed will occur before every generation.
	DRBG and SHA modules should be activate prior to the this operation. \n

 * @param context              pointer to DRBG context.\n
							Use NULL in order to generate using the shared NCL DRBG context [in]
 * @param addData              Additional data for random bits generation.                     [in]
 * @param addDataLen           Additional data length in bytes.                                [in]
 * @param outBuff              Output buffer for random bytes                                  [out]
 * @param outBuffLen           Requested size of random data, in bytes                         [in]

 * @return  NCL_STATUS_OK on success and other NCL_STATUS_* on error
		This function may only be called after NCL_DRBG_Instantiate() was called for the same
		context. Otherwise returns DEFS_STATUS_FAIL.
 */
NCL_STATUS_T NCL_DRBG_Generate (
	void*           context,
	const uint8_t*  addData,
	uint32_t        addDataLen,
	uint8_t*        outBuff,
	uint32_t        outBuffLen
);
/*! @} */
#endif // NCL_DRBG

#ifdef NCL_RSA
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                   RSA                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*! \addtogroup RSA
   *  RSA APIs                                                  \n
   * Supports sign/verify operations with PKCS1.5/PSS padding.  \n
   * Supports encrypt/decrypt operations with OAEP padding.     \n
   * Supported key sizes are 2048b,3072b.                       \n
   *  @{
   */

/**
  This routine returns the RSA context size.
 * @return  RSA context size
 */
uint32_t NCL_RSA_GetContextSize (void);

/**
  This routine initializes the RSA context, the context area should be allocated
  by the user prior calling to this function.
 * @param context              pointer to RSA context   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_InitContext (void *context);

/**
  This routine finalize the RSA context, it should be called after RSA operation is
  finished
 * @param context              pointer to RSA context   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_FinalizeContext (void *context);

/**
  Clear all RSA SSPs (Sensitive Security Parameters) in HW and in driver level.\n
  This function will be called once SSP zeroisation is required.\n
  The routine clears all internal varibales, zeroizes all inputs and activates
  the HW module with zeroized inputs.
 * @param context              Pointer to ECC context   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_Clear (void *context);

/**
  This function performs RSA hardware initialization or setup that might be
  needed.\n
  This function should be called once at initiate time and before any other
  operation of this module's APIs.\n
  This function will run selftest automatically if wasn't run yet.
 * @param context              pointer to RSA context    [in]
 * @param handler              User handler function that will be called at the end of RSA calculation
							Can be null pointer       [in]
 * @return  NCL_STATUS_OK on success and other NCL_STATUS_* on error
 */
NCL_STATUS_T NCL_RSA_Init (void *context, NCL_USR_HANDLER handler);

/**
  This routine sets the RSA keys.\n
  This function do not copy keys to internal storage,
  only saves pubKey/prvKey pointers in context.
 * @param context              pointer to RSA context                  [in]
 * @param pubKey               Public key, can be NULL if not used     [in]
 * @param prvKey               Private key, can be NULL if not used    [in]
 * @param keySize              RSA key size                            [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_SetKey (
	void*                   context,
	const NCL_RSA_PUB_KEY*  pubKey,
	const NCL_RSA_PRV_KEY*  prvKey,
	NCL_RSA_KEY_SIZE_T      keySize
);

/**
  This routine clears the RSA keys.
 * @param context              pointer to RSA context    [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_ClearKey (void *context);

/**
  This API starts an RSA Decryption operation - the output buffer should be
  the result of the RSA Decryption with Optimal Asymmetric Encryption Padding.\n
  There are no alignment restrictions for either input or output buffers.\n
  RSA and SHA modules should be activate prior to the RSA decrypt operation.  \n
  If this function returns with NCL_STATUS_OK, NCL_RSA_GetStatus should be called until it returns
  non busy status to indicate the completion of this operation.
 * @param context              pointer to RSA context                               [in]
 * @param data                 Input data for decryption in LE byte order format    [in]
 * @param padding              Padding type                                         [in]
 * @param shaType              Hashing type and size                                [in]
 * @param associatedData       Associated data in BE byte order format              [in]
 * @param associatedDataLen    Length of associated data                            [in]
 * @param dataOut              Decrypted data in LE byte order format               [out]
 * @param dataOutLen           pointer to decrypted data size in bytes              [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_Decrypt (
	void*               context,
	const uint8_t*      data,
	NCL_RSA_PADDING_T   padding,
	NCL_SHA_TYPE_T      shaType,
	const uint8_t*      associatedData,
	uint32_t            associatedDataLen,
	uint8_t*            dataOut,
	uint32_t*           dataOutLen
);

/**
  This API starts an RSA Encryption operation - the output buffer should be
  the result of the RSA Encryption with Optimal Asymmetric Encryption Padding.
  There are no alignment restrictions for either input or output buffer.
  RSA, DRBG and SHA modules should be activate prior to the RSA encrypt operation.
  If this function returns with NCL_STATUS_OK, NCL_RSA_GetStatus should be called until it returns
  non busy status to indicate the completion of this operation.
 * @param context              pointer to RSA context                             [in]
 * @param data                 Input data for encryption in LE byte order format  [in]
 * @param dataLen              Length of input data                               [in]
 * @param padding              Padding type                                       [in]
 * @param shaType              Hashing type and size                              [in]
 * @param associatedData       Associated data in BE byte order format            [in]
 * @param associatedDataLen    Length of associated data                          [in]
 * @param dataOut              Encrypted data in LE byte order format             [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_Encrypt (
	void*               context,
	const uint8_t*      data,
	uint32_t            dataLen,
	NCL_RSA_PADDING_T   padding,
	NCL_SHA_TYPE_T      shaType,
	const uint8_t*      associatedData,
	uint32_t            associatedDataLen,
	uint8_t*            dataOut
);

/**
  This API starts an RSA sign operation - the output buffer should be
  the result of the RSA calculation that can be used in digital signature verification.\n
  There are no alignment restrictions for either input or output buffer\n
  RSA, DRBG and SHA modules should be activate prior to the RSA sign operation.\n
  If this function returns with NCL_STATUS_OK, NCL_RSA_GetStatus should be called until it returns
  non busy status to indicate the completion of this operation.
 * @param context              pointer to RSA context                               [in]
 * @param data                 Input data for signing in BE byte order format       [in]
 * @param dataLen              Length of input data                                 [in]
 * @param padding              Padding type                                         [in]
 * @param shaType              Hashing type and size                                [in]
 * @param sig                  Signature in LE byte order format                    [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_SignData (
	void*               context,
	const uint8_t*      data,
	uint32_t            dataLen,
	NCL_RSA_PADDING_T   padding,
	NCL_SHA_TYPE_T      shaType,
	uint8_t*            sig
);

/**
  This API starts an RSA sign operation - the output buffer should be
  the result of the RSA calculation that can be used in digital signature verification.\n
  There are no alignment restrictions for either input or output buffer.\n
  RSA, DRBG (when padding is PSS) and SHA modules should be activate prior to the RSA sign operation.\n
  If this function returns with NCL_STATUS_OK, NCL_RSA_GetStatus should be called until it returns
  non busy status to indicate the completion of this operation.
 * @param context              pointer to RSA context                           [in]
 * @param hash                 Input hash for signing in BE byte order format   [in]
 * @param padding              Padding type                                     [in]
 * @param shaType              Hashing type and size                            [in]
 * @param sig                  Signature in LE byte order format                [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_SignHash (
	void*               context,
	const uint8_t*      hash,
	NCL_RSA_PADDING_T   padding,
	NCL_SHA_TYPE_T      shaType,
	uint8_t*            sig
);

/**
  This API starts an RSA verify operation.\n
  There are no alignment restrictions for the input buffer.\n
  RSA and SHA modules should be activate prior to the RSA verify operation.\n
  If this function returns with NCL_STATUS_OK, NCL_RSA_GetStatus should be called until it returns
  non busy status to indicate the completion of this operation.
 * @param context              Pointer to RSA context                               [in]
 * @param sigOk                Verification result                                  [out]
 * @param data                 Input data for verification in BE byte order format  [in]
 * @param dataLen              Length of input data                                 [in]
 * @param padding              Padding type                                         [in]
 * @param shaType              Hashing type and size                                [in]
 * @param sig                  Signature in LE byte order format                    [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_VerifyData (
	void*               context,
	NCL_STATUS_T*       sigOk,
	const uint8_t*      data,
	uint32_t            dataLen,
	NCL_RSA_PADDING_T   padding,
	NCL_SHA_TYPE_T      shaType,
	const uint8_t*      sig
);

/**
  This API starts an RSA verify operation.\n
  There are no alignment restrictions for the input buffer.\n
  RSA and SHA modules should be activate prior to the RSA verify operation.\n
  If this function returns with NCL_STATUS_OK, NCL_RSA_GetStatus should be called until it returns
  non busy status to indicate the completion of this operation.
 * @param context              Pointer to RSA context                   [in]
 * @param sigOk                Verification result                      [out]
 * @param hash                 Input data hash in BE byte order format  [in]
 * @param padding              Padding type                             [in]
 * @param shaType              Hashing type and size                    [in]
 * @param sig                  Signature in LE byte order format        [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_VerifyHash (
	void*               context,
	NCL_STATUS_T*       sigOk,
	const uint8_t*      hash,
	NCL_RSA_PADDING_T   padding,
	NCL_SHA_TYPE_T      shaType,
	const uint8_t*      sig
);

/**
  This routine power on/off the RSA module.\n
  ECC and RSA shares same hardware, therefor powering on/off RSA hardware will also
  power on/off ECC hardware.
 * @param context              pointer to RSA context                 [in]
 * @param on                   True for power on, False otherwise     [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_Power (void *context, uint8_t on);

/**
  This routine return RSA completion status.
 * @param context              pointer to RSA context                 [in]
 * @return  NCL_STATUS_SYSTEM_BUSY if operation is not done yet, NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_RSA_GetStatus (void *context);
/*! @} */
#endif //NCL_RSA

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                   AES                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#ifdef NCL_AES
/*! \addtogroup AES
   *  AES APIs.                                                \n
   * Supported modes for AES encrypt/decrypt operations are:   \n
   * ECB, CBC, CTR, MAC, CFB, OFB, CCM, GCM.                   \n
   * Supported key sizes are 128b, 192b, 256b.                 \n
   *  @{
   */

/**
  This routine returns AES context size.
 * @return  AES context size
 */
uint32_t NCL_AES_GetContextSize (void);

/**
  This routine initializes AES context.
 * @param context              Pointer to AES context   [in]
 * @return  none
 */
NCL_STATUS_T NCL_AES_InitContext (void *context);

/**
  This routine finalize AES context.
 * @param context              Pointer to AES context   [in]
 * @return  none
 */
NCL_STATUS_T NCL_AES_FinalizeContext (void *context);

/**
  Perform any AES hardware initialization or setup that needed.\n
  This function should be called once at initiate time and before any other
  operation of this module's APIs.\n
  This function will run selftest automatically if wasn't run yet.
 * @param context              Pointer to AES context   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_Init (void *context);

/**
  Clear all AES SSPs (Sensitive Security Parameters) in HW and in driver level.\n
  This function will be called once SSP zeroisation is required.\n
  The routine clears all internal varibales, zeroizes all inputs and activates
  the HW module with zeroized inputs.
 * @param context              Pointer to AES context   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_Clear (void *context);

#ifdef AES_CAPABILITY_SCA_COUNTER_MEASURE
/**
  Selects source of random seed that is used by the AES as DPA countermeasure.\n
  Selecting bUseRandGen will use an internal DRBG based on the given 32b seed,\n
  otherwise, the AES engine will use the lower 4b of the received seed as a fixed seed.\n
 * @param context      - Pointer to AES context                               [in]
 * @param seed         - Random number to be loded to the AES engine.         [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_SetSeed (void *context, uint32_t seed, BOOLEAN bUseRandGen);
#endif

/**
  This API should be called at least once after NCL_AES_Init and before using
  any AES operation.\n
  If caller needs to use different key, or different AES mode, it needs
  to call this API again to re-set AES parameters correctly.\n
  It is the callers responsibility to ensure that the AES hardware
  is idle before this function is called.
 * @param context              Pointer to AES context  [in]
 * @param key                  AES key                 [in]
 * @param keySize              AES key size            [in]
 * @param mode                 AES operation mode      [in]
 * @param ivCtr                Initialization vector or counter depend on AES mode  [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_SetParams (
	void*               context,
	const uint32_t*     key,
	NCL_AES_KEY_SIZE_T  keySize,
	NCL_AES_MODE_T      mode,
	const uint32_t*     ivCtr
);

/**
  This API selects which AES key in the engine will be used for the further AES
  operations. Should be called before key/parameter settings and any AES calculation.
 * @param context            Pointer to AES context    [in]
 * @param keyIndex           Key index to select       [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
*/
NCL_STATUS_T NCL_AES_SelectKey (void *context, uint8_t keyIndex);

#ifdef AES_CAPABILITY_KEY_LOCK
/**
  This API This routine locks the selected key previously loaded to the engine.
  Once a key is locked it can be used by the engine but it cannot be read or changed.
 * @param context            Pointer to AES context    [in]
 * @param keyIndex           Key index to lock         [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
*/
NCL_STATUS_T NCL_AES_LockKey (void *context, uint8_t keyIndex);
#endif

/**
  This API should be called when modifying IV only.
  This should be used for the modes: CBC, CTR, MAC, CFB, OFB.
  It is the callers responsibility to ensure that the AES hardware
  is idle before this function is called.
 * @param context              Pointer to AES context                               [in]
 * @param ivCtr                Initialization vector or counter depend on AES mode  [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
*/
NCL_STATUS_T NCL_AES_SetIv (
	void*           context,
	const uint32_t* ivCtr
);

/**
  This API should be called when modifying AES key only.
  Key size and mode will be configured as well.
  It is the callers responsibility to ensure that the AES hardware
  is idle before this function is called.
 * @param context            Pointer to AES context    [in]
 * @param key                Pointer to AES key        [in]
 * @param keySize            Key size enum             [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
*/
NCL_STATUS_T NCL_AES_SetKey (
	void*               context,
	const uint32_t*     key,
	NCL_AES_KEY_SIZE_T  keySize
);

/**
  This API should be called in GCM mode after NCL_AES_Init and before using any
  AES operation (encrypt/decrypt) in GCM mode.\n
  It is the callers responsibility to ensure that the AES hardware
  is idle before this function is called.
  SHA and DRBG (when performing encryption) modules should be activate prior to this operation.
 * @param context      Pointer to AES context                              [in]
 * @param iv           Initialization vector or counter depend on AES op.
					For ENCRYPT op, IV is generated internally and
					assigned to 'iv'. For DECRYPT op, IV is received
					from user (should be the same IV that was returned
					from the encrypt operation.
					IV size must be between 96 to 128 bits.
					Address should be 4-byte aligned                [in/out]
 * @param key          AES key                                             [in]
 * @param keySize      AES key size                                        [in]
 * @param addData      Additional data needed for GCM mode. Data byte size
					should be 4B aligned                                [in]
 * @param op           AES operation - encrypt, decrypt                    [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_GCM_SetParams (
	void*                   context,
	uint32_t*               iv,
	const uint32_t*         key,
	NCL_AES_KEY_SIZE_T      keySize,
	const NCL_AES_ADD_DATA* addData,
	NCL_AES_OP_T            op
);

/**
  This API should be called in CCM mode after NCL_AES_Init and before using any
  AES operation (encrypt/decrypt) in CCM mode.\n
  It is the callers responsibility to ensure that the AES hardware
  is idle before this function is called.
 * @param context      Pointer to AES context                              [in]
 * @param iv           Initialization vector or counter depend on AES mode
					Address should be 4 Bytes aligned                   [in]
 * @param key          AES key                                             [in]
 * @param keySize      AES key size                                        [in]
 * @param addData      Additional data needed for CCM mode. Data byte size
					should be 4B aligned                                [in]
 * @param dataSize     Plaintext size in bytes.                            [in]
 * @param op           AES operation - encrypt, decrypt                    [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_CCM_SetParams (
	void*                   context,
	const uint32_t*         iv,
	const uint32_t*         key,
	NCL_AES_KEY_SIZE_T      keySize,
	const NCL_AES_ADD_DATA* addData,
	uint32_t                dataSize,
	NCL_AES_OP_T            op
);

/**
  This routine clears AES parameters.
 * @param context      Pointer to AES context                          [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_ClearParams (void *context);

/**
  This API encrypts input data, and returns the encrypted output data.\n
  This API should return error if NCL_AES_SetParams is not called
  to set the AES mode and key.
 * @param context      Pointer to AES context                                                  [in]
 * @param plainData    Input data for encryption                                               [in]
 * @param cipherData   Encrypted data                                                          [out]
 * @param dataLen      Data length (Should be 16-byte aligned)                                 [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_Encrypt (
	void*           context,
	const uint32_t* plainData,
	uint32_t*       cipherData,
	uint32_t        dataLen
);

/**
  This API decrypts input data, and returns the decrypted output data.\n
  This API returns error if NCL_AES_SetParams is not called
  to set the AES mode and key.
 * @param context      Pointer to AES context                                                  [in]
 * @param cipherData   Input data for decryption                                               [in]
 * @param plainData    Decrypted data                                                          [out]
 * @param dataLen      Data length (Should be 16-byte aligned)                                 [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_Decrypt (
	void*           context,
	const uint32_t* cipherData,
	uint32_t*       plainData,
	uint32_t        dataLen
);

/**
  This API continues the last AES operation performed.
  this APi should only be called after NCL_AES_SetParams and either NCL_AES_Decrypt/Encrypt were
  called first.
  The operation type (enc/dec), as well as the AES mode, remain unchanged. The Key and
  IV should also remain unchanged compared to the last operation.
  Supported AES modes: ECB, CBC, CFB, OFB
 * @param context      Pointer to AES context                                                  [in]
 * @param in           Input pointer                                                           [in]
 * @param out          Output pointer                                                          [out]
 * @param dataLen      Data length (Should be 16-byte aligned)                                 [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_Update (
	void*           context,
	const uint32_t* in,
	uint32_t*       out,
	uint32_t        dataLen
);

/**
  This API encrypts input data in GCM mode, and returns the encrypted output data and authentication tag.
  This API should return error if NCL_AES_GCM_SetParams is not called
  to set the AES mode and key.
 * @param context      Pointer to AES context                                          [in]
 * @param plainData    Input data for encryption (Address should be 4-byte aligned)    [in]
 * @param cipherData   Encrypted data (Address should be 4-byte aligned)               [out]
 * @param dataLen      Data length                                                     [in]
 * @param tag          Authentication tag (Address should be 4-byte aligned)           [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_GCM_Encrypt (
	void*           context,
	const uint32_t* plainData,
	uint32_t*       cipherData,
	uint32_t        dataLen,
	uint32_t*       tag
);

/**
  This API decrypts input data in GCM mode, and returns the decrypted output data.
  This API also verifies the authentication tag correctness, zeroizes output buffer if not.
  This API should return error if NCL_AES_GCM_SetParams is not called
  to set the AES mode and key.
 * @param context      Pointer to AES context                                          [in]
 * @param cipherData   Input data for decryption (Address should be 4-byte aligned)    [in]
 * @param plainData    Decrypted data (Address should be 4-byte aligned)               [out]
 * @param dataLen      Data length.                                                    [in]
 * @param tag          Authentication tag (Address should be 4-byte aligned)           [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_GCM_Decrypt (
	void*           context,
	const uint32_t* cipherData,
	uint32_t*       plainData,
	uint32_t        dataLen,
	const uint32_t* tag
);

/**
  This API encrypts input data in CCM mode, and returns the encrypted output data and authentication tag.
  The authentication tag is concatenated to the ciphertext, so a buffer of at least (plaintext_size + tag_size)
  bytes must be allocated.
  This API should return error if NCL_AES_CCM_SetParams is not called
  to set the AES mode and key.
 * @param context      Pointer to AES context                                          [in]
 * @param plainData    Input data for encryption (Address should be 4-byte aligned)    [in]
 * @param cipherData   Encrypted data (Address should be 4-byte aligned)               [out]
 * @param dataLen      Data length                                                     [in]
 * @param tagSize      Authentication tag size (enum)                                  [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_CCM_Encrypt (
	void*                  context,
	const uint32_t*        plainData,
	uint32_t*              cipherData,
	uint32_t               dataLen,
	NCL_AES_CCM_MAC_SIZE_T tagSize
);

/**
  This API decrypts input data in CCM mode, and returns the decrypted output data.
  The input data should contain the the authentication tag concatenated to the ciphertext.
  Therefore the input buffer allocated size should be at least (plaintext_size + tag_size) bytes.
  This API also verifies the authentication tag correctness. If tag authentication fails,
  this API zeroizes the cipherdata buffer.
  This API should return error if NCL_AES_CCM_SetParams is not called
  to set the AES mode and key.
 * @param context      Pointer to AES context                                          [in]
 * @param cipherData   Input data for Decryption (Address should be 4-byte aligned)    [in]
 * @param plainData    Decrypted data (Address should be 4-byte aligned)               [out]
 * @param dataLen      Data length                                                     [in]
 * @param tagSize      Authentication tag size (enum)                                  [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_CCM_Decrypt (
	void*                  context,
	const uint32_t*        cipherData,
	uint32_t*              plainData,
	uint32_t               dataLen,
	NCL_AES_CCM_MAC_SIZE_T tagSize
);

/**
  This API generates a MAC using AES CMAC mode.
  This API should be called after AES_SetParams, with IV parameter set to NULL.
 * @param context      Pointer to AES context                                          [in]
 * @param message      Input message (Address should be 4-byte aligned)                [in]
 * @param mac          Calculated MAC (Address should be 4-byte aligned)               [out]
 * @param messageLen   Message length                                                  [in]
 * @param macLen       Authentication tag size (in Bytes, up to 16B)                   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_CMAC_Generate (
	void*                  context,
	const uint32_t*        message,
	uint32_t*              mac,
	uint32_t               messageLen,
	uint32_t               macLen
);

/**
  This API verifies a MAC using AES CMAC mode.
  This API should be called after AES_SetParams, with IV parameter set to NULL.
 * @param context      Pointer to AES context                                          [in]
 * @param isVerified   Verification result                                             [out]
 * @param message      Input message (Address should be 4-byte aligned)                [in]
 * @param mac          Expected MAC (Address should be 4-byte aligned)                 [in]
 * @param messageLen   Message length                                                  [in]
 * @param macLen       Authentication tag size (in Bytes, up to 16B)                   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_CMAC_Verify (
	void*                  context,
	NCL_STATUS_T*          isVerified,
	const uint32_t*        message,
	const uint32_t*        mac,
	uint32_t               messageLen,
	uint32_t               macLen
);

/**
  This routine power on/off AES module.
 * @param context              Pointer to AES context                 [in]
 * @param on                   True for power on, False otherwise     [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_AES_Power (void *context, uint8_t on);
/*! @} */
#endif //NCL_AES

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                   SHA                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#ifdef NCL_SHA
/*! \addtogroup SHA
   *  SHA APIs.                                                 \n
   * Supports SHA sizes 256b, 384b and 512b.                    \n
   * Enables hashing with multiple data updates.                \n
   * Supports HMAC calculation for all supported hash sizes.    \n
   *  @{
   */

/**
  This routine returns SHA context size.
 * @return  SHA context size
 */
uint32_t NCL_SHA_GetContextSize (void);

/**
  This routine initializes SHA context.
 * @param context              pointer to SHA context   [in]
 * @return  none
 */
NCL_STATUS_T NCL_SHA_InitContext (void *context);

/**
  This routine finalize SHA context.
 * @param context              pointer to SHA context   [in]
 * @return  none
 */
NCL_STATUS_T NCL_SHA_FinalizeContext (void *context);

/**
  Clear all SHA SSPs (Sensitive Security Parameters) in HW and in driver level.\n
  This function will be called once SSP zeroisation is required.\n
  The routine clears all internal varibales, zeroizes all inputs and activates
  the HW module with zeroized inputs.
 * @param context              pointer to SHA context    [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_Clear (void *context);
/**
  This API initiates the SHA hardware and setups needed parameters.\n
  This function should be called once at initiate time and before any other
  operation of this module's APIs.\n
  This function will run selftest automatically if wasn't run yet.
 * @param context              pointer to SHA context   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_Init (void *context);

/**
  This API prepares the context buffer for a SHA calculation -  by loading
  the initial SHA-256/384/512 parameters.\n
  This is a blocking function - when the function returns the hardware and memory buffer
  shall be ready to accept new data buffers for SHA calculation.
 * @param context              pointer to SHA context   [in]
 * @param shaType              SHA type and size        [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_Start (void *context, NCL_SHA_TYPE_T shaType);

/**
  This function updates the SHA calculation with additional data.\n
  This is a blocking function - when the function returns the
  hardware and memory buffer shall be ready to accept new data.\n
  Buffers for SHA calculation and changes to the data in data buffer should
  no longer effect the SHA calculation.\n
  The number of bytes in buffer may not be an even multiple of the
  SHA-256/384/512 message block size (512/768/1024 bits).\n The vendor API
  routine must properly handle this.\n
  The input buffer data has no alignment restrictions, and will be
  non-null. The length parameter dataLen will be greater than zero.
 * @param context              pointer to SHA context    [in]
 * @param data                 Data for hashing          [in]
 * @param dataLen              Data size                 [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_Update (void *context, const uint8_t *data, uint32_t dataLen);

/**
  This APIs returns the SHA result.\n
  The hash result is size is the same as that of the hash function chosen.\n
  The first word of the result (that is W0 - the low order word) is stored in
  the first 4 bytes sha[0..3].\n The word is stored in little endian
  byte order. W1 is stored next, and so on.\n
  The hashDigest output buffer has no alignment restrictions and will be non NULL.
 * @param context              pointer to SHA context           [in]
 * @param hashDigest           Output buffer for hashing digest [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_Finish (void *context, uint8_t *hashDigest);

/**
  This API is a blocking function, that performs complete SHA calculation in one step.
 * @param context              pointer to SHA context            [in]
 * @param shaType              SHA type and size                 [in]
 * @param data                 Data for hashing                  [in]
 * @param dataLen              Data size                         [in]
 * @param hashDigest           Output buffer for hashing digest  [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_Calc (
	void*           context,
	NCL_SHA_TYPE_T  shaType,
	const uint8_t*  data,
	uint32_t        dataLen,
	uint8_t*        hashDigest
);

/**
  This routine power on/off SHA module.
 * @param context              pointer to SHA context                 [in]
 * @param on                   True for power on, False otherwise     [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_Power (void *context, uint8_t on);

/**
  This API resets the SHA hardware, terminating any in-progress operations.\n
  This function is blocked while the reset is taking place and only\n
  returns when the reset is finished and the hardware is ready for use.
  The hardware shall be in the same state as when NCL_SHA_Init returns.
 * @param context              pointer to SHA context           [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_Reset (void *context);

/**
  This API prepares the context buffer for a HMAC calculation -  by loading
  the initial SHA-256/384/512 parameters.\n
  The function also performs the initialization of the hash with the HMAC key XOR iPad and
  returns the 'padKey' (should be used later as an 'padKey' input to NCL_SHA_HMAC_Finish).\n
  This is a blocking function - when the function returns the hardware and memory buffer
  shall be ready to accept new data buffers for HMAC calculation.
 * @param context              pointer to SHA context      [in]
 * @param shaType              SHA type and size           [in]
 * @param key                  HMAC key                    [in]
 * @param padKey               The HMAC key in iPad format [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_HMAC_Start (
	void*                   context,
	NCL_SHA_TYPE_T          shaType,
	const NCL_SHA_HMAC_KEY* key,
	uint8_t*                padKey
);

/**
  This function updates the HMAC calculation with additional data.\n
  This is a blocking function - when the function returns the
  hardware and memory buffer shall be ready to accept new data.\n
  The number of bytes in buffer may not be an even multiple of the
  SHA-256/384/512 message block size (512/768/1024 bits).\n The vendor API
  routine must properly handle this.\n
  The input buffer data has no alignment restrictions, and will be
  non-null. The length parameter dataLen will be greater than zero.
 * @param context              pointer to SHA context      [in]
 * @param data                 Data buffer                 [in]
 * @param dataLen              Data length                 [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_HMAC_Update (
	void*                   context,
	const uint8_t*          data,
	uint32_t                dataLen
);

/**
  This APIs returns the HMAC result.\n
  The hmac result is size is the same as that of the hash function chosen.\n
  The function finishes digesting all accumulated messages. Then, it updates the HMAC key
  XOR oPad, hashes the oPadKey and the completed digest and returns HMAC result\n
 * @param context              pointer to SHA context      [in]
 * @param padKey               The HMAC key in oPad format [in]
 * @param hmac                 HMAC result                 [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_HMAC_Finish (
	void*                   context,
	uint8_t*                padKey,
	uint8_t*                hmac
);

/**
  This API returns the HMAC result.\n
  The HMAC result size is the same as that of the hash function choosen via
  shaType parameter.\n
  The first word of the result (that is W0 - the low order word) is stored in
  the first 4 bytes hmac[0..3]. The word is stored in little endian
  byte order. W1 is stored next, and so on.\n
  This is a blocking function - when the function returns the
  hardware and memory buffer shall be ready to accept new data
  buffers for HMAC calculation and changes to the data in 'data' buffer should
  no longer effect the HMAC calculation.\n
  The number of bytes in buffer may not be an even multiple of the
  SHA-256 message block size (512 bits).\n The caller API routine must
  properly handle this.
  The input buffer data has no alignment restrictions, and will be
  non-null. The length parameter dataLen will be greater than zero.\n
  Same applies to input parameter key.\n
  The hmac output buffer has no alignment restrictions and will be non-NULL.
 * @param context              pointer to SHA context    [in]
 * @param shaType              SHA type and size         [in]
 * @param key                  HMAC key                  [in]
 * @param data                 Data buffer               [in]
 * @param dataLen              Data length               [in]
 * @param hmac                 HMAC result               [out]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_HMAC_Calc (
	void*                   context,
	NCL_SHA_TYPE_T          shaType,
	const NCL_SHA_HMAC_KEY* key,
	const uint8_t*          data,
	uint32_t                dataLen,
	uint8_t*                hmac
);

/**
  This API performs a (HMAC)-based Key Derivation Function (HKDF). \n
  The API is an implementaion of a KDF as defined in the SP800-108 NIST document
  while the HMAC is used as the Pseudorandom Function (PRF). \n
  All buffers, including the results, and their size should be valid (there are no
  alignment restrictions).\n
  The result should be non-null, and the result size to be greater than zero. \n
 * @param context              Pointer to SHA context                       [in]
 * @param hkdfInput            Pointer to the HKDF input parameter stucture [in]
 * @param result               Pointer to the result (Output Key Material)  [out]
 * @param resultSize           Size (in bytes) of the result key            [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_SHA_HKDF_Calc (
	void*                               context,
	_CONST_TYPE_ NCL_SHA_HKDF_INPUT_T*  hkdfInput,
	uint8_t*                            result,
	uint32_t                            resultSize
);
/*! @} */
#endif //NCL_SHA

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                   ECC                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#ifdef NCL_ECC
/*! \addtogroup ECC
   *  ECC APIs                                          \n
   * Supports curve P-256/P-384/P-521.                  \n
   * Supports ECDSA sign/verify operations.             \n
   * Supports ECDH key exchange operation.              \n
   * Supports key generation.                           \n
   * Supported key sizes are: 256b, 384b, 521b.         \n
   *  @{
   */

/**
  This routine returns ECC context size
 * @return  ECC context size
 */
uint32_t NCL_ECC_GetContextSize (void);

/**
  This routine initializes ECC context.
 * @param context              pointer to ECC context   [in]
 * @return  none
 */
NCL_STATUS_T NCL_ECC_InitContext (void *context);

/**
  This routine finalize ECC context.
 * @param context              pointer to ECC context   [in]
 * @return  None
 */
NCL_STATUS_T NCL_ECC_FinalizeContext (void *context);

/**
  Clear all ECC SSPs (Sensitive Security Parameters) in HW and in driver level.\n
  This function will be called once SSP zeroisation is required.\n
  The routine clears all internal varibales, zeroizes all inputs and activates
  the HW module with zeroized inputs.
 * @param context              Pointer to ECC context   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_Clear (void *context);

/**
  This routine initializes ECC operation.\n
  This function should be called once at initiate time and before any other
  operation of this module's APIs.\n
  This function will run selftest automatically if wasn't run yet.
 * @param context              pointer to ECC context        [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_Init (void *context);

/**
  This routine power on/off ECC module.\n
  ECC and RSA shares same hardware, therefor powering on/off ECC hardware will also
  power on/off RSA hardware.
 * @param context              pointer to ECC context               [in]
 * @param on                   True for power on, False otherwise   [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_Power (void *context, uint8_t on);

/**
  This routine perform ECC pre calculations.\n
  Set ECC parameters to be used for key generation, key exchange, sign and verify.\n
  This API should be called at least once before using any ECC operation
  after HW initialization.\n
  If caller needs to use different curve, it needs to call this API again
  to re-set ECC parameters correctly.\n
  It is the callers responsibility to ensure that the ECC hardware
  is idle before this function is called.\n
  curveData input parameters is expected in little-endian byte order.
 * @param context              pointer to ECC context [in]
 * @param curveData            curve data             [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_SetCurve (void *context, const NCL_ECC_CURVE_DATA_T *curveData);

/**
  This routine sets the given seed to be used during ECC calculation blinding\n
  and DPA protection.\n
  The seed value should be random (high-qulity rng is not required, as it is used only as a countermeasure).\n
  To generate random seed internally using DRBG, call this API with randSeed = 0xFFFFFFFF.\n
  This random number will be stirred after every use.
 * @param context              pointer to ECC context                          [in]
 * @param randSeed             32-bit seed value / 0 to generate internally    [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_SetSeed (void *context, uint32_t randSeed);

/**
  This routine signs input message.\n
  This function uses Elliptic Curve Digital Signature Algorithm (ECDSA)
  to sign the input buffer.\n
  prvKey, msg, curveData input parameters are expected in little-endian byte order\n
  r,s output parameters are given in little-endian byte order.
  ECC, DRBG and SHA modules should be activate prior to this operation.\n
 * @param context              pointer to ECC context                      [in]
 * @param r                    signature r value                           [out]
 * @param s                    signature s value                           [out]
 * @param shaType              hashing type for hashing input message      [in]
 * @param prvKey               private key                                 [in]
 * @param msg                  message to sign                             [in]
 * @param msgLen               Length of message in bytes                  [in]
 * @param curveData            curve data                                  [in]
 * @param kdfFunc              key derivation function pointer             [in]
 * @param kdfData              key derivation input data pointer           [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_ECDSASignData (
	void*                       context,
	NCL_BN_T*                   r,
	NCL_BN_T*                   s,
	NCL_SHA_TYPE_T              shaType,
	const NCL_BN_T*             prvKey,
	const uint8_t*              msg,
	uint32_t                    msgLen,
	const NCL_ECC_CURVE_DATA_T* curveData,
	NCL_ECC_KdfFuncPtr          kdfFunc,
	void*                       kdfData
);

/**
  This routine signs input message.\n
  This function uses Elliptic Curve Digital Signature Algorithm (ECDSA)
  to sign the input buffer.\n
  prvKey, curveData input parameters are expected in little-endian byte order.\n
  hash input parameter is expected in big-endian byte order.\n
  r,s output parameters are given in little-endian byte order.\n
  ECC, DRBG and SHA modules should be activate prior to this operation.\n
 * @param context              pointer to ECC context                      [in]
 * @param r                    signature r value                           [out]
 * @param s                    signature s value                           [out]
 * @param shaType              hashing type for hashing input message      [in]
 * @param prvKey               private key                                 [in]
 * @param hash                 input hash for signing                      [in]
 * @param curveData            curve data                                  [in]
 * @param kdfFunc              key derivation function pointer             [in]
 * @param kdfData              key derivation input data pointer           [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_ECDSASignHash (
	void*                       context,
	NCL_BN_T*                   r,
	NCL_BN_T*                   s,
	NCL_SHA_TYPE_T              shaType,
	const NCL_BN_T*             prvKey,
	const uint8_t*              hash,
	const NCL_ECC_CURVE_DATA_T* curveData,
	NCL_ECC_KdfFuncPtr          kdfFunc,
	void*                       kdfData
);

/**
  This routine verifies input message.\n
  This function uses Elliptic Curve Digital Signature Algorithm (ECDSA).
  to verify the input buffer is signed correctly.\n
  r, s, msg, pubKey, curveData input parameters are expected in little-endian byte order.\n
  SHA module should be activate prior to this operation.\n
 * @param context              pointer to ECC context                      [in]
 * @param sigOk                result                                      [out]
 * @param r                    signature r value                           [in]
 * @param s                    signature s value                           [in]
 * @param shaType              hashing type for hashing input message      [in]
 * @param msg                  message to verify                           [in]
 * @param msgLen               Length of message in bytes                  [in]
 * @param pubKey               public key pointer                          [in]
 * @param curveData            curve data                                  [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_ECDSAVerifyData (
	void*                       context,
	NCL_STATUS_T*               sigOk,
	const NCL_BN_T*             r,
	const NCL_BN_T*             s,
	NCL_SHA_TYPE_T              shaType,
	const uint8_t*              msg,
	uint32_t                    msgLen,
	const NCL_ECC_POINT_T*      pubKey,
	const NCL_ECC_CURVE_DATA_T* curveData
);

/**
  This routine verifies input message.\n
  This function uses Elliptic Curve Digital Signature Algorithm (ECDSA)
  to verify the input buffer is signed correctly.\n
  r, s, pubKey, curveData input parameters are expected in little-endian byte order.\n
  hash input parameter is expected in big-endian byte order.\n
  hash input buffer is expected in big-endian byte order.
 * @param context              pointer to ECC context                      [in]
 * @param sigOk                result                                      [out]
 * @param r                    signature r value                           [in]
 * @param s                    signature s value                           [in]
 * @param shaType              hashing type for hashing input message      [in]
 * @param hash                 message to verify                           [in]
 * @param pubKey               public key pointer                          [in]
 * @param curveData            curve data                                  [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_ECDSAVerifyHash (
	void*                       context,
	NCL_STATUS_T*               sigOk,
	const NCL_BN_T*             r,
	const NCL_BN_T*             s,
	NCL_SHA_TYPE_T              shaType,
	const uint8_t*              hash,
	const NCL_ECC_POINT_T*      pubKey,
	const NCL_ECC_CURVE_DATA_T* curveData
);

/**
  This routine generates ECC key pair.\n
  This function uses Elliptic-curve Diffie-Hellman (ECDH) key agreement protocol
  to generate elliptic-curve public-private key pair, that allows two parties
  to establish a shared secret over an insecure channel.\n
  prvKey,pubKey output parameters are given in little-endian byte order.\n
  ECC, DRBG and SHA modules should be activate prior to this operation.\n
 * @param context              pointer to ECC context                      [in]
 * @param prvKey               private key                                 [out]
 * @param pubKey               public key                                  [out]
 * @param curveData            curve data                                  [in]
 * @param kdfFunc              key derivation function pointer             [in]
 * @param kdfData              key derivation input data pointer           [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_GenKey (
	void*                       context,
	const NCL_BN_T*             prvKey,
	const NCL_ECC_POINT_T*      pubKey,
	const NCL_ECC_CURVE_DATA_T* curveData,
	NCL_ECC_KdfFuncPtr          kdfFunc,
	void*                       kdfData
);

/**
  This routine calculates shared secret.\n
  This function uses Elliptic-curve Diffie Hellman (ECDH) key agreement protocol
  to exchange keys between two parties and establish a shared secret over an
  insecure channel.\n
  It is the caller's responsibility to power on ECC HW block
  prior to starting the ECC key exchange operation.\n
  APrvKey, BPubKey, curveData input parameters are expected in little-endian byte order.\n
  sharedSecret output parameter is given in little-endian byte order.
 * @param context              pointer to ECC context            [in]
 * @param sharedSecret         shared secret                     [out]
 * @param APrvKey              Alice's private key               [in]
 * @param BPubKey              Bob's public key                  [in]
 * @param curveData            curve data                        [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_ECDHKeyExchange (
	void*                       context,
	const NCL_ECC_POINT_T*      sharedSecret,
	const NCL_BN_T*             APrvKey,
	const NCL_ECC_POINT_T*      BPubKey,
	const NCL_ECC_CURVE_DATA_T* curveData
);

/**
  This routine returns wether given point is on the given curve
 * @param context              pointer to ECC context            [in]
 * @param isVerified           return value (TRUE=1,FALSE=0)     [out]
 * @param pointIn              Point to verify                   [in]
 * @param curveData            curve data                        [in]
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_PointVerify (
	void*                       context,
	uint8_t*                    isVerified,
	const NCL_ECC_POINT_T*      pointIn,
	const NCL_ECC_CURVE_DATA_T* curveData
);

/**
  This routine returns pointer to a predefined NIST approved curve
 * @param context              pointer to ECC context            [in]
 * @param curve                Requested predefined curve        [in]
 * @return  Pointer to existing curve, NULL if curve type is not supported
 */
NCL_ECC_CURVE_DATA_T* NCL_ECC_GetCurveData (
	void*           context,
	NCL_ECC_CURVE_T curve
);
/*! @} */
#endif // NCL_ECC

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               SELF TESTS                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#ifdef NCL_SELF_TEST

#if defined NCL_RSA && defined NCL_DRBG && defined NCL_SHA
/**
  This routine perform RSA encrypt self test
 * @return  NCL_STATUS_OK if no errors, NCL_STATUS_FAIL otherwise.
 */
NCL_STATUS_T NCL_RSA_ExtendedEncryptSelfTest_l (void);

/**
  This routine perform RSA verify self test
 * @return  NCL_STATUS_OK if no errors, NCL_STATUS_FAIL otherwise.
 */
NCL_STATUS_T NCL_RSA_ExtendedVerifySelfTest_l (void);

/**
  This routine perform RSA Sign self test
 * @return  NCL_STATUS_OK if no errors, NCL_STATUS_ERROR otherwise.
 */
NCL_STATUS_T NCL_RSA_ExtendedSignSelfTest_l (void);

/**
  This routine perform RSA Sign/verify with PKCS padding,
  and enc/dec with all supported key sizes and hash sizes self test.
 * @return  NCL_STATUS_OK if no errors, NCL_STATUS_ERROR otherwise.
 */
NCL_STATUS_T NCL_RSA_ExtendedSelfTest_l (void);

#endif //defined NCL_RSA && defined NCL_DRBG && defined NCL_SHA

#ifdef NCL_AES
/**
  This routine performs AES self test
 * @return  NCL_STATUS_OK if no errors, NCL_STATUS_FAIL otherwise.
 */
NCL_STATUS_T NCL_AES_ExtendedSelfTest_l (void);
#endif // NCL_AES

#ifdef NCL_DRBG
/**
  This routine performs DRBG test
  - Currently in order to test if the DRBG result is a random we just look at itand
  not more than that.
  - As a result it is not an automatic test but needed to be run with debuger
 * @return  NCL_STATUS_OK if no errors, NCL_STATUS_FAIL otherwise.
 */
NCL_STATUS_T NCL_DRBG_ExtendedSelfTest_l (void);
#endif // NCL_DRBG

#ifdef NCL_SHA
/**
  This routine perform SHA self test
 * @return  NCL_STATUS_OK if no errors, NCL_STATUS_FAIL otherwise.
 */
NCL_STATUS_T NCL_SHA_ExtendedSelfTest_l (void);

/**
  This routine perform HMAC self test
 * @return  NCL_STATUS_OK if no errors, NCL_STATUS_FAIL otherwise.
 */
NCL_STATUS_T NCL_SHA_ExtendedHmacSelfTest_l (void);
#endif // NCL_SHA

#if defined NCL_ECC && defined NCL_DRBG && defined NCL_SHA
/**
  This routine perform ECC self test
 * @return  NCL_STATUS_OK for successful completion and other NCL_STATUS_* for error
 */
NCL_STATUS_T NCL_ECC_ExtendedSelfTest_l (void);

#endif // NCL_ECC && NCL_DRBG && NCL_SHA

#endif // NCL_SELF_TEST

/*---------------------------------------------------------------------------------------------------------*/
/* ECC Key size DWORD                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum {
	ECC_KEY_SIZE_DWORD_144 = 5,
	ECC_KEY_SIZE_DWORD_256 = 8,
	ECC_KEY_SIZE_DWORD_384 = 12,
	ECC_KEY_SIZE_DWORD_521 = 17,
} ECC_KEY_SIZE_DWORD_T;

#endif //_NCL_IF_H
