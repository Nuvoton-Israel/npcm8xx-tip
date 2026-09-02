/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2019 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   mpu_if.c                                                                                              */
/*            This file contains MPU module driver selector                                                */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#include __CHIP_H_FROM_IF()

#if defined(MPU_MODULE_TYPE)
    #include __MODULE_DRIVER(mpu, MPU_MODULE_TYPE)
#endif

