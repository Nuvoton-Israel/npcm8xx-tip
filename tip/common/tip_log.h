/*
 *  Nuvoton Technology Corporation confidential
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *<<<-------------------------------------------------------------------------
 * File Contents:
 *   tip_log.h
 *            This file contains API of routines for handling the PCI MailBox log
 *  Project:
 *            Arbel
 */
#ifndef _TIP_LOG_H_
#define _TIP_LOG_H_
#include "hal.h"

#define ST_ROM_STATUS_NUM                       0x40
#define ST_ROM_STATUS_INFO_NUM                  0x10
/*---------------------------------------------------------------------------------------------------------*/
/* General TIP ROM status, size 0x158                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#pragma pack(push, 1)
typedef struct TIP_LOG_Arr_tag
{
    uint32_t TIP_LOG_ResetCounter;
    uint32_t TIP_LOG_LastBmcReset;
    uint32_t TIP_LOG_LastTipReset;
    uint32_t TIP_LOG_StatusArr[ST_ROM_STATUS_NUM];
    uint32_t TIP_LOG_StatusCounter;
    uint32_t TIP_LOG_InfoArr[ST_ROM_STATUS_INFO_NUM];
    uint32_t TIP_LOG_InfoCounter;
    uint32_t TIP_LOG_ProductionFailure;
} TIP_LOG_Arr_T;
#pragma pack(pop)

#endif /* _TIP_LOG_H_ */
