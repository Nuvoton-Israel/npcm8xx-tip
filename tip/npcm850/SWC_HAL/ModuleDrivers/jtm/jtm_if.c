/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2020 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   jtm_if.c                                                                                              */
/*            This file contains JTM module driver selector                                                */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
//#error  jtm_if.c    
#include __CHIP_H_FROM_IF()

#if defined(JTM_MODULE_TYPE)
    #include __MODULE_DRIVER(jtm, JTM_MODULE_TYPE)
#endif

