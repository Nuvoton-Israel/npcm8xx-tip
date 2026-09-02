/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2017 Cisco Systems, Inc.  All rights reserved.                                           */
/*  Copyright (c) 2022 by Nuvoton Technology Corporation                                                   */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   hss_drv.c                                                                                             */
/*            This file contains LMS implementation                                                        */
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

#ifndef HSS_MODULE_TYPE
/*---------------------------------------------------------------------------------------------------------*/
/* These includes are the open source code package which implements the LMS/LMS functionality              */
/* https://github.com/cisco/hash-sigs                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#include "../../hss/hw_sha/hash_sig/endian.c"
#include "../../hss/hw_sha/hash_sig/hash.c"
#include "../../hss/hw_sha/hash_sig/lm_common.c"
#include "../../hss/hw_sha/hash_sig/lm_ots_common.c"
#include "../../hss/hw_sha/hash_sig/lm_ots_verify.c"
#include "../../hss/hw_sha/hash_sig/lm_verify.c"
//#include "../../hss/hw_sha/hash_sig/sha256.c"
#endif

#include "lms_drv.h"

extern ROM_NCL_SHA_Power		   pROM_NCL_SHA_Power;

#if defined SHA_MODULE_TYPE
#include __MODULE_IF_HEADER_FROM_DRV(sha)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* LMS instance struct handler                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
   


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
DEFS_STATUS LMS_Power (BOOLEAN on)
{
    DEFS_STATUS_RET_CHECK(pROM_NCL_SHA_Power(NULL, on));

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS LMS_VerifySig (
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
  
    tempRes = lm_validate_signature(publicKey, message, messageLen, false,
                                    signature, signatureLen);

    if (DEFS_SEC_COND_CHECK(tempRes == DEFS_STATUS_OK))
    {
        retStatus = DEFS_STATUS_OK;
    }
    DEFS_FLOW_MONITOR_INCREMENT();
    DEFS_FLOW_MONITOR_COMPARE_RET(2);

    return retStatus;
}


