/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2014-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   fuse_if.c                                                                                             */
/*            This file contains FUSE module driver selector                                               */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#include "hal.h"
#include __CHIP_H_FROM_IF()

#if defined(FUSE_MODULE_TYPE)
    #include __MODULE_DRIVER(fuse, FUSE_MODULE_TYPE)
#endif

