/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2022 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   hss_if.h                                                                                              */
/*            This file contains the HSS interface                                                         */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef __HSS_IF_H__
#define __HSS_IF_H__

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
typedef struct HSS_HANDLE_T HSS_HANDLE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* HSS sizes                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef HSS_KEY_LENGTH
#define HSS_KEY_LENGTH  60
#endif

#ifndef HSS_SIG_LENGTH
#define HSS_SIG_LENGTH  4944
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        HSS_Init                                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  handlePtr - pointer to HSS handle                                                      */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success or other DEFS_STATUS error code on error.                    */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the HSS module                                                */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS HSS_Init (HSS_HANDLE_T* handlePtr);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        HSS_Power                                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  on - TRUE to power on the module and FALSE to power off.                               */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success or other DEFS_STATUS error code on error                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine powers on/off the hardware modules needed                                 */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS HSS_Power (BOOLEAN on);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        HSS_VerifySig                                                                          */
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
/*                  This routine verifies HSS signature                                                    */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS HSS_VerifySig (const UINT8* publicKey, const UINT8* message, UINT32 messageLen,
                                  const UINT8* signature, UINT32 signatureLen);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        HSS_VerifySigStart                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  handlePtr    - pointer to HSS handle                                                   */
/*                  publicKey    - pointer to the public key                                               */
/*                  siganture    - signature to verify                                                     */
/*                  signatureLen - signature length in bytes                                               */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success or other DEFS_STATUS error code on error                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the process of verifying an HSS signature incrementally       */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS HSS_VerifySigStart (HSS_HANDLE_T* handlePtr, const UINT8* publicKey,
                                         const UINT8* signature, UINT32 signatureLen);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        HSS_VerifySigUpdate                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  handlePtr         - pointer to HSS handle                                              */
/*                  messageSegment    - message segment that was supposedly signed                         */
/*                  lenMessageSegment - message segment length in bytes                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success or other DEFS_STATUS error code on error                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine updates a message segment to validate                                     */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS HSS_VerifySigUpdate (HSS_HANDLE_T* handlePtr, const UINT8* messageSegment, UINT32 lenMessageSegment);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        HSS_VerifySigFinish                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  handlePtr    - pointer to HSS handle                                                   */
/*                  siganture    - signature to verify                                                     */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success or other DEFS_STATUS error code on error                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs the actual signature verification after all the pieces           */
/*                  of the message were added                                                              */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS HSS_VerifySigFinish (HSS_HANDLE_T* handlePtr, const UINT8* signature);

#endif //__HSS_IF_H__

