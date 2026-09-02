/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2014-2022 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   tmc_if.c                                                                                              */
/*            This file contains TMC module driver selector                                                */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#include __CHIP_H_FROM_IF()
#include "hal.h"

#if defined(TMC_MODULE_TYPE)
    #include __MODULE_DRIVER(tmc, TMC_MODULE_TYPE)
#endif

