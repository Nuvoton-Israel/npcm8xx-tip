/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2022 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   lms_if.c                                                                                              */
/*            This file contains LMS module driver selector                                                */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#include __CHIP_H_FROM_IF()

#if defined(LMS_MODULE_TYPE)
    #include __MODULE_DRIVER(lms, LMS_MODULE_TYPE)
#endif

