/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2022 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   lms_if.h                                                                                              */
/*            This file contains the LMS interface                                                         */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef __LMS_IF_H__
#define __LMS_IF_H__

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()

#if defined SHA_MODULE_TYPE
#include __MODULE_IF_HEADER_FROM_IF(sha)
#endif

#include <stddef.h>


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                          TYPES AND DEFINITIONS                                          */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
typedef struct LMS_HANDLE_T LMS_HANDLE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* LMS sizes                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef LMS_KEY_LENGTH
#define LMS_KEY_LENGTH  60
#endif

#ifndef LMS_SIG_LENGTH
#define LMS_SIG_LENGTH  4944
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        LMS_Init                                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  handlePtr - pointer to LMS handle                                                      */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success or other DEFS_STATUS error code on error.                    */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the LMS module                                                */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        LMS_Power                                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  on - TRUE to power on the module and FALSE to power off.                               */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success or other DEFS_STATUS error code on error                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine powers on/off the hardware modules needed                                 */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS LMS_Power (BOOLEAN on);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        LMS_VerifySig                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  publicKey    - pointer to the public key                                               */
/*                  message      - message that was supposedly signed                                      */
/*                  messageLen   - message length in bytes                                                 */
/*                  siganture    - siganture to verify                                                     */
/*                  signatureLen - signature length in bytes                                               */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success or other DEFS_STATUS error code on error                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine verifies LMS signature                                                    */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS LMS_VerifySig (const UINT8* publicKey, const UINT8* message, UINT32 messageLen,
                                  const UINT8* signature, UINT32 signatureLen);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        LMS_VerifySigStart                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  handlePtr    - pointer to LMS handle                                                   */
/*                  publicKey    - pointer to the public key                                               */
/*                  siganture    - signature to verify                                                     */
/*                  signatureLen - signature length in bytes                                               */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success or other DEFS_STATUS error code on error                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the process of verifying an LMS signature incrementally       */
/*---------------------------------------------------------------------------------------------------------*/

#endif //__LMS_IF_H__

