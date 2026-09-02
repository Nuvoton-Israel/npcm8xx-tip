/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef PLATFORM_ECC_TESTING_H_
#define PLATFORM_ECC_TESTING_H_


//#define	ECC_TESTING_USE_OPENSSL


#ifdef ECC_TESTING_USE_OPENSSL
/* Configure the ECC testing to use the OpenSSL. */
#include "crypto/ecc_openssl.h"
#define	ECC_TESTING_ENGINE_NAME	openssl
#endif


#endif /* PLATFORM_ECC_TESTING_H_ */
