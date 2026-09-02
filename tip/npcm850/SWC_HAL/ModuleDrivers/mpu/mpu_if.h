/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2019 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   mpu_if.h                                                                                              */
/*            This file contains MPU module interface                                                      */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef MPU_IF_H
#define MPU_IF_H

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()

#if defined MPU_MODULE_TYPE
#include __MODULE_HEADER(mpu, MPU_MODULE_TYPE)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               DEFINITIONS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

typedef enum
{
    REGION_0        = 0,
    REGION_1        = 1,
    REGION_2        = 2,
    REGION_3        = 3,
    REGION_4        = 4,
    REGION_5        = 5,
    REGION_6        = 6,
    REGION_7        = 7,
} MPU_REGION;

typedef enum
{
    REGION_SIZE_32B     = 4,
    REGION_SIZE_64B     = 5,
    REGION_SIZE_128B    = 6,
    REGION_SIZE_256B    = 7,
    REGION_SIZE_512B    = 8,
    REGION_SIZE_1KB     = 9,
    REGION_SIZE_2KB     = 10,
    REGION_SIZE_4KB     = 11,
    REGION_SIZE_8KB     = 12,
    REGION_SIZE_16KB    = 13,
    REGION_SIZE_32KB    = 14,
    REGION_SIZE_64KB    = 15,
    REGION_SIZE_128KB   = 16,
    REGION_SIZE_256KB   = 17,
    REGION_SIZE_512KB   = 18,
    REGION_SIZE_1MB     = 19,
    REGION_SIZE_2MB     = 20,
    REGION_SIZE_4MB     = 21,
    REGION_SIZE_8MB     = 22,
    REGION_SIZE_16MB    = 23,
    REGION_SIZE_32MB    = 24,
    REGION_SIZE_64MB    = 25,
    REGION_SIZE_128MB   = 26,
    REGION_SIZE_256MB   = 27,
    REGION_SIZE_512MB   = 28,
    REGION_SIZE_1GB     = 29,
    REGION_SIZE_2GB     = 30,
    REGION_SIZE_4GB     = 31
} MPU_REGION_SIZE;

/*---------------------------------------------------------------------------------------------------------*/
/* MPU AP Encodings                                                                                        */
/* MPU_AP_<PRIVILEGED_PERMISSIONS>_<UNPRIVILEGED_PERMISSIONS>                                              */
/*---------------------------------------------------------------------------------------------------------*/

typedef enum
{
    MPU_AP_NO_ACCES_NO_ACCESS = 0,
    MPU_AP_RW_NO_ACCESS       = 1,
    MPU_AP_RW_RO              = 2,
    MPU_AP_RW_RW              = 3,
    MPU_AP_RO_NO_ACCESS       = 4,
    MPU_AP_RO_RO              = 6,
    MPU_AP_RO_RO2             = 7,
} MPU_AP;

typedef struct MPU_REGION_SETTINGS
{
    MPU_REGION      num;                        // Region number
    UINT32          address;                    // Region base address
    BOOLEAN         enable;                     // When the MPU is enabled, this region is enabled/disabled
    MPU_REGION_SIZE size;                       // Region size in bytes = 2^(SIZE+1)
    UINT8           subRegionDisableMask;       // Controls whether one of the eight equal subregions is enabled
    UINT8           accPermissionAttr;          // Bit 0 - B value, Bit 1 - C value, Bit 2 - S value, Bits 3-5 - TEX value
    MPU_AP          accPermission;              // Access and privilege properties of the region
    BOOLEAN         intsructionFetchDisable;    // Disable execution of an instruction fetched from this region
} MPU_REGION_SETTINGS;


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
void MPU_Init (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_Reset                                                                              */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine resets the MPU module                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_Reset (void);

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
void MPU_Config (BOOLEAN defaultMemMapEnable, BOOLEAN operateDuringHardFault);

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
DEFS_STATUS MPU_RegionConfig (const MPU_REGION_SETTINGS* regionSettings);

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
void MPU_Enable (BOOLEAN enable);

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
void MPU_RegionEnable (MPU_REGION num, BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_PrintRegs (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        MPU_PrintVersion                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void MPU_PrintVersion (void);

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
DEFS_STATUS MPU_SelfTest (void);
#endif

#endif /* __MPU_IF_H__ */

