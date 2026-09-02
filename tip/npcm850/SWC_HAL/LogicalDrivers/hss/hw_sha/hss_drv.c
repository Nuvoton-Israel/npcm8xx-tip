/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2017 Cisco Systems, Inc.  All rights reserved.                                           */
/*  Copyright (c) 2022 by Nuvoton Technology Corporation                                                   */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   hss_drv.c                                                                                             */
/*            This file contains HSS implementation                                                        */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_DRV()

#include <string.h>
#include <stdio.h>
#include "hss_drv.h"
#include "./hash_sig/hash.h"
#include "./hash_sig/hss_verify_inc.h"
#include "./hash_sig/hss_verify.h"

/*---------------------------------------------------------------------------------------------------------*/
/* These includes are the open source code package which implements the HSS/LMS functionality              */
/* https://github.com/cisco/hash-sigs                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#include "./hash_sig/endian.c"
#include "./hash_sig/hash.c"
#include "./hash_sig/hss_thread_single.c"
#include "./hash_sig/hss_verify.c"
#include "./hash_sig/hss_verify_inc.c"
#include "./hash_sig/hss_zeroize.c"
#include "./hash_sig/lm_common.c"
#include "./hash_sig/lm_ots_common.c"
#include "./hash_sig/lm_ots_verify.c"
#include "./hash_sig/lm_verify.c"
#include "./hash_sig/sha256.c"

#if defined SHA_MODULE_TYPE
#include __MODULE_IF_HEADER_FROM_DRV(sha)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* HSS instance struct handler                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
struct HSS_HANDLE_T
{
    BOOLEAN                 active;
    struct hss_validate_inc hss_context;
};


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
DEFS_STATUS HSS_Init (HSS_HANDLE_T* handlePtr)
{
    DEFS_STATUS_COND_CHECK((handlePtr != NULL), DEFS_STATUS_INVALID_PARAMETER);

    handlePtr->active = FALSE;

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS HSS_Power (BOOLEAN on)
{
    DEFS_STATUS_RET_CHECK(SHA_Power(on));

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS HSS_VerifySig (
    const UINT8*    publicKey,
    const UINT8*    message,
    UINT32          messageLen,
    const UINT8*    signature,
    UINT32          signatureLen
)
{
    volatile DEFS_STATUS retStatus = DEFS_STATUS_AUTHENTICATION_FAIL;
    volatile DEFS_STATUS tempRes   = DEFS_STATUS_FAIL;
    DEFS_FLOW_MONITOR_DECLARE(0);
    /*-----------------------------------------------------------------------------------------------------*/
    /* Arguments Check                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK((publicKey != NULL), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((message != NULL),   DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((messageLen != 0),   DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((signature != NULL), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((signatureLen != 0), DEFS_STATUS_INVALID_PARAMETER);

    DEFS_FLOW_MONITOR_INCREMENT();

	UINT64 time64_1 = CLK_GetUpTimeMicros ();
	 tempRes = hss_validate_signature(publicKey, message, messageLen, signature, signatureLen, NULL);
	
	UINT64 time64_2 = CLK_GetUpTimeMicros ();
	serial_printf (KYEL "Uptime 1 %lu uSecs\n"  KNRM, (uint32_t) time64_1);
	serial_printf (KYEL "Uptime 2 %lu uSecs\n"  KNRM, (uint32_t) time64_2);
	serial_printf (KYEL "HSS time is   %lu uSecs\n"  KNRM, (uint32_t) (time64_2-time64_1));

	
   
    DEFS_FLOW_MONITOR_INCREMENT();

    if (DEFS_SEC_COND_CHECK((tempRes == DEFS_STATUS_OK)))
    {
        retStatus = DEFS_STATUS_OK;
    }
    DEFS_FLOW_MONITOR_COMPARE_RET(2);
    return retStatus;
}

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
DEFS_STATUS HSS_VerifySigStart(
    HSS_HANDLE_T*   handlePtr,
    const UINT8*    publicKey,
    const UINT8*    signature,
    UINT32          signatureLen
)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Arguments Check                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK((handlePtr != NULL), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((publicKey != NULL), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((signature != NULL), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((signatureLen != 0), DEFS_STATUS_INVALID_PARAMETER);

    handlePtr->active = TRUE;

    DEFS_STATUS_COND_CHECK(hss_validate_signature_init(&(handlePtr->hss_context), publicKey, signature, signatureLen, NULL),
                           DEFS_STATUS_AUTHENTICATION_FAIL);

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS HSS_VerifySigUpdate (
    HSS_HANDLE_T*   handlePtr,
    const UINT8*    messageSegment,
    UINT32          lenMessageSegment
)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Arguments Check                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK((handlePtr != NULL),      DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((messageSegment != NULL), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((lenMessageSegment != 0), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK(handlePtr->active == TRUE, DEFS_STATUS_SYSTEM_NOT_INITIALIZED);

    DEFS_STATUS_COND_CHECK(hss_validate_signature_update(&(handlePtr->hss_context), messageSegment, lenMessageSegment),
                           DEFS_STATUS_AUTHENTICATION_FAIL);

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS HSS_VerifySigFinish (
    HSS_HANDLE_T*   handlePtr,
    const UINT8*    signature
)
{
    volatile DEFS_STATUS retStatus = DEFS_STATUS_AUTHENTICATION_FAIL;
    volatile DEFS_STATUS tempRes   = DEFS_STATUS_FAIL;
    DEFS_FLOW_MONITOR_DECLARE(0);
    /*-----------------------------------------------------------------------------------------------------*/
    /* Arguments Check                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK((handlePtr != NULL), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((signature != NULL), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK(handlePtr->active == TRUE, DEFS_STATUS_SYSTEM_NOT_INITIALIZED);

    DEFS_FLOW_MONITOR_INCREMENT();
    tempRes = hss_validate_signature_finalize(&(handlePtr->hss_context), signature, NULL);
    DEFS_FLOW_MONITOR_INCREMENT();

    if (DEFS_SEC_COND_CHECK(tempRes == DEFS_STATUS_OK))
    {
    handlePtr->active = FALSE;

        retStatus = DEFS_STATUS_OK;
    }
    DEFS_FLOW_MONITOR_COMPARE_RET(2);
    return retStatus;
}

