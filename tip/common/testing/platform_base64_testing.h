/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef PLATFORM_BASE64_TESTING_H_
#define PLATFORM_BASE64_TESTING_H_


//#define	BASE64_TESTING_USE_OPENSSL


#ifdef BASE64_TESTING_USE_OPENSSL
/* Configure the base64 testing to use the OpenSSL. */
#include "crypto/base64_openssl.h"
#define	BASE64_TESTING_ENGINE_NAME	openssl
#endif


#endif /* PLATFORM_BASE64_TESTING_H_ */
