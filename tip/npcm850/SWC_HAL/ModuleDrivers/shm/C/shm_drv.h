/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2016-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   shm_drv.h                                                                                             */
/*            This file contains SHM driver interface                                                      */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef SHM_DRV_H
#define SHM_DRV_H

#define SHM_CAPABILITY_SEMAPHORE_SMI_GENERATION
#define SHM_CAPABILITY_OFFSET
#define SHM_CAPABILITY_ABSOLUTE_WIN_BASE_ADDR
#define SHM_CAPABILITY_EXTENDED_8_AREA_PROTECTION
#define SHM_CAPABILITY_INDIRECT_MEMORY_ACCESS
#define SHM_CAPABILITY_VERSION
#define SHM_CAPABILITY_HOST_OFFSET
#define SHM_CAPABILITY_EXTENDED_SHM
#define SHM_CAPABILITY_REMAP_HOST_REGS
#define SHM_CAPABILITY_HW_HOST_EVENT_WIN_3_4

#include __MODULE_IF_HEADER_FROM_DRV(shm)

#endif  /* SHM_DRV_H */

