/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef PLATFORM_AES_TESTING_H_
#define PLATFORM_AES_TESTING_H_


//#define	AES_TESTING_USE_OPENSSL


#ifdef AES_TESTING_USE_OPENSSL
/* Configure the AES testing to use the OpenSSL. */
#include "crypto/aes_openssl.h"
#define	AES_TESTING_ENGINE_NAME	openssl
#endif


#endif /* PLATFORM_AES_TESTING_H_ */
