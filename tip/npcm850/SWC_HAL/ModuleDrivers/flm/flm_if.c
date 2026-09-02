/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2020 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   flm_if.c                                                                                              */
/*            This file contains FLM module driver selector                                                */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#include __CHIP_H_FROM_IF()

#if defined(FLM_MODULE_TYPE)
    #include __MODULE_DRIVER(flm, FLM_MODULE_TYPE)
#endif

