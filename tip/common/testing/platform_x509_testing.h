/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef PLATFORM_X509_TESTING_H_
#define PLATFORM_X509_TESTING_H_


//#define	X509_TESTING_USE_OPENSSL


#ifdef X509_TESTING_USE_OPENSSL
/* Configure the X.509 testing to use the OpenSSL. */
#include "crypto/x509_openssl.h"
#define	X509_TESTING_ENGINE_NAME	openssl
#endif


#endif /* PLATFORM_X509_TESTING_H_ */
