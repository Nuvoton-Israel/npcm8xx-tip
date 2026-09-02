/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2019 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   npcm850_tip_if.h                                                                                      */
/*            This file contains chip dependent interface                                                  */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef __NPCM850_TIP_IF_H__
#define __NPCM850_TIP_IF_H__

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/* Module Dependencies                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/

#if defined (NVIC_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(nvic)
#endif

#if defined (AES_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(aes)
#endif

#if defined (GPIO_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(gpio)
#endif

#if defined (FIU_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(fiu)
#endif

#if defined (FUSE_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(fuse)
#endif

#if defined (MPU_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(mpu)
#endif

#if defined (PKA_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(pka)
#endif

#if defined (RNG_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(rng)
#endif

#if defined (SHA_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(sha)
#endif

#if defined (TWD_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(twd)
#endif

#if defined (UART_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_CHIP_DRV(uart)
#endif

#include "../npcm850/npcm850_if.h"

typedef enum TIP_SEC_BOOL_T
{
    TIP_SEC_FALSE           = 0x6,
    TIP_SEC_TRUE            = 0x9,
} TIP_SEC_BOOL_T;


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

DEFS_STATUS CHIP_MapPeripheralWindow (UINT windowNum, UINT32 mappedAddress, UINT32 windowSize);

DEFS_STATUS CHIP_MapSystemWindow (UINT windowNum, UINT32 mappedAddress, UINT32 windowSize);

#if defined (FUSE_MODULE_TYPE)
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_SecureFuseFieldRead                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  fuse_address - offset in fuse array                                                    */
/*                  fuse_length  - num of bytes to read                                                    */
/*                  fuse_ecc     - error correction type                                                   */
/*                  value (out)  - output value pointer to set                                             */
/*                                                                                                         */
/* Returns:         status                                                                                 */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine gets a value of a specified field from fuse array. In case of wrong       */
/*                  checksum it generates a sw security error (causes reset in case it is enabled).        */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS CHIP_SecureFuseFieldRead (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc, UINT8* value);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_FustrapRead                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  fuse_address - offset in fuse array                                                    */
/*                  fuse_length  - num of bytes to read                                                    */
/*                  fuse_ecc     - error correction type                                                   */
/*                                                                                                         */
/* Returns:         velue (UINT32) of FUSTRAP                                                              */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the value of a fustrap register 1/2.(using a secured read).       */
/*---------------------------------------------------------------------------------------------------------*/
UINT32 CHIP_FustrapRead (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_oSecBoot_On                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  none                                                                                   */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns whether a security boot should be performed according to the      */
/*                  value in the otp value (protected by majority ECC)                                     */
/*---------------------------------------------------------------------------------------------------------*/
SECURED_BOOLEAN_T CHIP_oSecBoot_On (void);
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_GetSecurityEvent                                                                  */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         security event register value                                                          */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns security event register value                                     */
/*---------------------------------------------------------------------------------------------------------*/
UINT16 CHIP_GetSecurityEvent (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_SecurityEventErrorOccured                                                         */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         security event error ocuured                                                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns TRUE if any security event error has occured, FALSE if hasn't     */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN CHIP_SecurityEventErrorOccured (void);

#if defined (GPIO_MODULE_TYPE)
void CHIP_GpioInit (UINT gpio, GPIO_DIR_T gpioDir, BOOLEAN pullUp, BOOLEAN pullDown, BOOLEAN vddDriven);
void CHIP_GpioWrite (UINT gpio, UINT pin_data);
UINT CHIP_ReadGpioOutput (UINT gpio);
#endif


#if defined (NVIC_MODULE_TYPE)
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_ConfigNotification                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  handler : ISR to be called when getting notifications from BMC                         */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets a handler for notifications from BMC (B2CPST)                        */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS CHIP_ConfigNotification (SW_HANDLER_T handler);
#endif

#endif //__NPCM850_TIP_IF_H__

