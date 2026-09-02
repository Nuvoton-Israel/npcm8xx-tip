/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef PLATFORM_RNG_TESTING_H_
#define PLATFORM_RNG_TESTING_H_


//#define	RNG_TESTING_USE_OPENSSL


#ifdef RNG_TESTING_USE_OPENSSL
/* Configure the RNG testing to use the OpenSSL. */
#include "crypto/rng_openssl.h"
#define	RNG_TESTING_ENGINE_NAME	openssl
#endif


#endif /* PLATFORM_RNG_TESTING_H_ */
