/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2019 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   mpu_drv.c                                                                                             */
/*            This file contains MPU module driver implementation                                          */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_DRV()

#include "mpu_regs.h"
#include "mpu_drv.h"


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               DEFINITIONS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#define REGION_MAX_SIZE         REGION_SIZE_4GB
#define REGION_MIN_SIZE         REGION_SIZE_32B
#define REGION_MAX_NUM          8


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                              API FUNCTIONS                                              */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_Init                                                                               */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the MPU module                                                */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_Init (void)
{

}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_Reset                                                                              */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine resets the MPU module                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_Reset (void)
{
    INT i;

    for (i = (REGION_MAX_NUM - 1); i >= 0; i--)
    {
        SET_REG_FIELD(MPU_RNR, MPU_RNR_REGION, i);
        REG_WRITE(MPU_RASR, 0);
        REG_WRITE(MPU_RBAR, 0);
    }

    REG_WRITE(MPU_CTRL, 0);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_Config                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  defaultMemMapEnable    - enables use of the default memory map as a background region  */
/*                                           for privileged software accesses.                             */
/*                  operateDuringHardFault - enables the operation of MPU during hard fault, NMI, and      */
/*                                           FAULTMASK handlers..                                          */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the MPU module                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_Config (BOOLEAN defaultMemMapEnable, BOOLEAN operateDuringHardFault)
{
    SET_REG_FIELD(MPU_CTRL, MPU_CTRL_PRIVDEFENA, defaultMemMapEnable);
    SET_REG_FIELD(MPU_CTRL, MPU_CTRL_HFNMIENA,   operateDuringHardFault);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_RegionConfig                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  regionSettings - a pointer to the region settings struct                               */
/* Returns:                                                                                                */
/*                  DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures MPU region                                                     */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS MPU_RegionConfig (const MPU_REGION_SETTINGS* regionSettings)
{
    UINT32  size        = (UINT32)regionSettings->size;
    UINT32  sizePlusOne = size + 1;
    UINT32  sizeInBytes = 1 << sizePlusOne; // 2^(size + 1)
    UINT32  address     = regionSettings->address;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check size and alignment                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK(((size <= REGION_MAX_SIZE) && (size >= REGION_MIN_SIZE)), DEFS_STATUS_INVALID_PARAMETER);
    DEFS_STATUS_COND_CHECK((address % sizeInBytes == 0), DEFS_STATUS_INVALID_PARAMETER);

    MPU_RegionEnable(regionSettings->num, regionSettings->enable);

    SET_REG_FIELD(MPU_RBAR, MPU_RBAR_VALID, 0);
    SET_REG_FIELD(MPU_RBAR, MPU_RBAR_ADDR(sizePlusOne), (address >> sizePlusOne));

    SET_REG_FIELD(MPU_RASR, MPU_RASR_SIZE, size);
    SET_REG_FIELD(MPU_RASR, MPU_RASR_B_C_S_TEX, regionSettings->accPermissionAttr);

    /*-----------------------------------------------------------------------------------------------------*/
    /* For regions of 256 bytes or larger, each bit of this field controls whether one of the eight equal  */
    /* subregions is enabled. For regions of 128 bytes or less, write the SRD field as 0x00.               */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(MPU_RASR, MPU_RASR_SRD, sizeInBytes >= 256 ? regionSettings->subRegionDisableMask : 0x00);

    SET_REG_FIELD(MPU_RASR, MPU_RASR_AP, regionSettings->accPermission);
    SET_REG_FIELD(MPU_RASR, MPU_RASR_XN, regionSettings->intsructionFetchDisable);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_Enable                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  enable - if True, enables the module, if false, disables it                            */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables/disables the MPU                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_Enable (BOOLEAN enable)
{
    SET_REG_FIELD(MPU_CTRL, MPU_CTRL_ENABLE, enable);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_RegionEnable                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  num    - the region number (0-7)                                                       */
/*                  enable - if True, enables the region configuration, if false, disables it              */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables/disables a MPU region configuration                               */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_RegionEnable (MPU_REGION num, BOOLEAN enable)
{
    SET_REG_FIELD(MPU_RNR,  MPU_RNR_REGION,  num);
    SET_REG_FIELD(MPU_RASR, MPU_RASR_ENABLE, enable);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_PrintRegs (void)
{
    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     MPU      */\n");
    HAL_PRINT("/*--------------*/\n\n");

    HAL_PRINT("MPU_TYPE            = 0x%08X\n", REG_READ(MPU_TYPE));
    HAL_PRINT("MPU_CTRL            = 0x%08X\n", REG_READ(MPU_CTRL));
    HAL_PRINT("MPU_RNR             = 0x%08X\n", REG_READ(MPU_RNR));
    HAL_PRINT("MPU_RBAR            = 0x%08X\n", REG_READ(MPU_RBAR));
    HAL_PRINT("MPU_RASR            = 0x%08X\n", REG_READ(MPU_RASR));

    HAL_PRINT("\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_PrintVersion                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_PrintVersion (void)
{
    HAL_PRINT("MPU         = %X\n", MODULE_VERSION(MPU_MODULE_TYPE));
}

#ifdef MPU_SELF_TEST
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_SelfTest                                                                           */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine test the MPU driver                                                       */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS MPU_SelfTest (void)
{
    int *point = (int *) 0x100A0001;
    int x = 30;

    MPU_REGION_SETTINGS const settings = {REGION_0, 0x100A0000, 1, 4, 0, 250, MPU_AP_RO_RO, 0};

    MPU_Reset();
    (void)MPU_RegionConfig(&settings);
    MPU_Config(TRUE, TRUE);
    MPU_Enable(TRUE);

    *point = x;

    return DEFS_STATUS_OK;
}
#endif

