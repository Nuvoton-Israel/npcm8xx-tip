/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2020 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   npcm850_tip.c                                                                                         */
/*            This file contains chip dependent functionality implementation                               */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_CHIP()

#include "npcm850_tip_if.h"
#include "npcm850_tip_regs.h"

#include "hal_regs.h"


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 MACROS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_MapPeripheralWindow                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  windowNum   , mappedAddress, windowSize.    if windowSize == 0 window disabled         */
/* Returns:         status                                                                                 */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine tmap a peripheral window                                                  */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS CHIP_MapPeripheralWindow (UINT windowNum, UINT32 mappedAddress, UINT32 windowSize)
{
    UINT16 reg_val = 0;

    // check for legal values
    if ((windowNum >= PWIN_NUM) ||
        (mappedAddress > 0xFFFF0000) || (mappedAddress & 0xFFF)||
        (windowSize > _128KB_) || (windowSize & 0xFFF) )
    {
        return DEFS_STATUS_INVALID_PARAMETER;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /*  Disable the window                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    if (windowSize == 0)
    {
        SET_VAR_FIELD(reg_val, PWIN0C1_WINEN, 0);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Open and enable the window                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    else
    {
        SET_VAR_FIELD(reg_val, PWIN0C1_WINEN, 1);
    }

    SET_VAR_FIELD(reg_val, PWIN0C1_WINSIZE, (windowSize >> 12) - 1);
    REG_WRITE(PWINnC1(windowNum), reg_val);
    REG_WRITE(PWINnC2(windowNum), (mappedAddress & 0x0FFFF000) >> 12);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_MapSystemWindow                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  windowNum   , mappedAddress, windowSize . if windowSize == 0 window disabled           */
/* Returns:         status                                                                                 */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine map a system window                                                       */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS CHIP_MapSystemWindow (UINT windowNum, UINT32 mappedAddress, UINT32 windowSize)
{
    UINT16 reg_val = 0;

    // check for legal values
    if ((windowNum >= SWIN_NUM) ||
        (mappedAddress & 0xFFFF)||  /* base address has to be a 10KB address aligned. */
        (windowSize > _2MB_) || (windowSize & 0xFFF) )
    {
        return DEFS_STATUS_INVALID_PARAMETER;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /*  Disable the window                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    if (windowSize == 0)
    {
        SET_VAR_FIELD(reg_val, SWIN0C1_WINEN, 0);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Open and enable the window                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    else
    {
        SET_VAR_FIELD(reg_val, SWIN0C1_WINEN, 1);
    }

    SET_VAR_FIELD(reg_val, SWIN0C1_WINSIZE, (windowSize >> 12) - 1);
    REG_WRITE(SWINnC1(windowNum), reg_val);
    REG_WRITE(SWINnC2(windowNum), mappedAddress >> 16); // Address of first byte in window: WINSTART * 10000h.

    return DEFS_STATUS_OK;
}

DEFS_STATUS CHIP_DisableMappingWindows()
{
    DEFS_STATUS status;

    /*-----------------------------------------------------------------------------------------------------*/
    /*  Disable all peripheral windows (later open only the ones we need)                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    if( (status = CHIP_MapPeripheralWindow(0, 0xF0000000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapPeripheralWindow(1, 0xF0080000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapPeripheralWindow(2, 0xF0100000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapPeripheralWindow(3, 0xF0180000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapPeripheralWindow(4, 0xF0200000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapPeripheralWindow(5, 0xF0800000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapPeripheralWindow(6, 0xF0000000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapPeripheralWindow(7, 0xFFFF0000, 0)) != DEFS_STATUS_OK) return status;

    /*-----------------------------------------------------------------------------------------------------*/
    /*  Disable all peripheral windows (later open only the ones we need)                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    if( (status = CHIP_MapSystemWindow(0, 0x00000000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapSystemWindow(1, 0x00200000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapSystemWindow(2, 0x00400000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapSystemWindow(3, 0x00600000, 0)) != DEFS_STATUS_OK) return status;
    if( (status = CHIP_MapSystemWindow(4, 0x00800000, 0)) != DEFS_STATUS_OK) return status;

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_SecureMemcpy                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  dest    - Pointer to the destination array where the content is to be copied           */
/*                  src     - Pointer to the source of data to be copied                                   */
/*                  n       - Number of bytes to be copied.                                                */
/*                                                                                                         */
/* Returns:         a pointer to destination, which is dest.                                               */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function copies from source pointer to destination n bytes. It uses the value of  */
/*                  the DWT cycle count as the 'random' value.                                             */
/*---------------------------------------------------------------------------------------------------------*/
void* CHIP_SecureMemcpy (void* dest, const void* src, UINT n)
{
    UINT32 rnd = 0;
#if defined (SCS_MODULE_TYPE) && defined (DWT_MODULE_TYPE)
    static SECURED_BOOLEAN_T scs_and_dwt_init;

    if (scs_and_dwt_init != SECURED_TRUE)
    {
        SCS_TraceEnable(TRUE);
        (void)DWT_EnableCycleCount(TRUE);

        scs_and_dwt_init = SECURED_TRUE;
    }

    rnd = (UINT32)DWT_GetCycleCount();
#endif

    return HAL_SEC_memcpy(dest, src, n, rnd);
}

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
DEFS_STATUS CHIP_SecureFuseFieldRead (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc, UINT8* value)
{
    volatile DEFS_STATUS status = DEFS_STATUS_FAIL;

    /* When Reading OTP, fields with errors should be corrected (using 64/72 error correction / Nibble parity / Majority
       role). If the number of errors exceeds the number of error that can be corrected, the ROM code should set the
       SW_SEC_ERR (Software Security Error) bit in the SEC_ERR register, resulting a TIP security error. */
    status = FUSE_WRPR_get(fuse_address, fuse_length, fuse_ecc, value);
    if (status == DEFS_STATUS_BAD_CHECKSUM)
    {
        REG_WRITE(SEC_ERR, 0xAA00); /* SEC_QUAL */
        REG_WRITE(SEC_ERR ,BUILD_FIELD_VAL(SEC_ERR_SW_SEC_ERR, 1));
        /* no need to clear SEC_ERR to 0 since a reset will occur */
    }

    return status;
}

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
UINT32 CHIP_FustrapRead (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc)
{
    UINT32 fustrap = 0xFFFFFFFF;

    if (CHIP_SecureFuseFieldRead(fuse_address, fuse_length, fuse_ecc, (UINT8*)&fustrap) != DEFS_STATUS_OK)
        return 0xFFFFFFFF;

    return fustrap;
}

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
SECURED_BOOLEAN_T CHIP_oSecBoot_On (void)
{
    UINT32 fustrap = 0xFFFFFFFF;

    if (CHIP_SecureFuseFieldRead(FUSTRAP1_PROPERTY, (UINT8*)&fustrap) != DEFS_STATUS_OK)
        return SECURED_TRUE;

    return (READ_VAR_FIELD(fustrap, FUSTRAP1_OSECBOOT) ? SECURED_TRUE : SECURED_FALSE);
}
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
UINT16 CHIP_GetSecurityEvent (void)
{
    return REG_READ(SEC_EV);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_SecurityEventErrorOccured                                                         */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         security event error ocuured                                                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns TRUE if any security event error has occured, FALSE if hasn't     */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN CHIP_SecurityEventErrorOccured (void)
{
    return READ_REG_FIELD(SEC_ERR, SEC_ERR_SEC_EV_ERR) != 0x2;
}

#if defined (GPIO_MODULE_TYPE)
void CHIP_GpioInit (UINT gpio, GPIO_DIR_T gpioDir, BOOLEAN pullUp, BOOLEAN pullDown, BOOLEAN vddDriven)
{
    // set direction to output
    SET_REG_FIELD(GPIODOD, GPIODOD_GPOE(gpio), gpioDir);

    // enable/disable pull-up & pull-down
    SET_REG_FIELD(GPIOPUD, GPIOPUD_GPPU(gpio), pullUp);
    SET_REG_FIELD(GPIOPUD, GPIOPUD_GPPD(gpio), pullDown);

    // configure if VDD driven
    SET_REG_FIELD(GPIOVDD, GPIOVDD_GPVD(gpio), vddDriven);
}

void CHIP_GpioWrite (UINT gpio, UINT pin_data)
{
    SET_REG_FIELD(GPIODOD, GPIODOD_GPDO(gpio), pin_data);
}

UINT CHIP_ReadGpioOutput (UINT gpio)
{
    return READ_REG_FIELD(GPIODIN, GPIODIN_GPI(gpio));
}
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
DEFS_STATUS CHIP_ConfigNotification (SW_HANDLER_T handler)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* parameters validity                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK(handler != NULL, DEFS_STATUS_INVALID_PARAMETER);


    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear all pending notification                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(B2TIPST0, 0xFFFF);
    REG_WRITE(B2TIPST1, 0xFFFF);

    /*-----------------------------------------------------------------------------------------------------*/
    /* HW bug, try to repeate                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(B2TIPST0, 0xFFFF);
    REG_WRITE(B2TIPST1, 0xFFFF);


    /*-----------------------------------------------------------------------------------------------------*/
    /* install handler and enable                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_REGISTER_AND_ENABLE(CHIP_INTERRUPT_PROVIDER,  NVIC_INT_2, handler, INTERRUPT_POLARITY_LEVEL_HIGH, 0);


    return DEFS_STATUS_OK;
}
#endif


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        CHIP_PrintRegs                                                                         */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void CHIP_PrintRegs (void)
{

    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     CHIP     */\n");
    HAL_PRINT("/*--------------*/\n\n");

    HAL_PRINT("B2TIPST0 = %#010lx \n", REG_READ(B2TIPST0));
    HAL_PRINT("B2TIPST1 = %#010lx \n", REG_READ(B2TIPST1));
    HAL_PRINT("TIP2BNT0 = %#010lx \n", REG_READ(TIP2BNT0));
    HAL_PRINT("TIP2BNT1 = %#010lx \n", REG_READ(TIP2BNT1));
}


#include "../npcm850/npcm850.c"

