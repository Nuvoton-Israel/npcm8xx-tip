/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2010-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   smb_drv.h                                                                                             */
/*            This file contains SMB driver interface                                                      */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef SMB_DRV_H
#define SMB_DRV_H

#ifdef GLUE_BASE_ADDR
#define SMB_CAPABILITY_WAKEUP_SUPPORT
#endif
#define SMB_CAPABILITY_FAST_MODE_SUPPORT
#define SMB_CAPABILITY_FAST_MODE_PLUS_SUPPORT

// Using SW PEC instead of HW PEC:
//#define SMB_CAPABILITY_HW_PEC_SUPPORT
#define SMB_STALL_TIMEOUT_SUPPORT

#include __MODULE_IF_HEADER_FROM_DRV(smb)

#endif  /* SMB_DRV_H */

