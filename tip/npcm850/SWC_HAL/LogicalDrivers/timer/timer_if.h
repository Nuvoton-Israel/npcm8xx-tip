/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2012-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   timer_if.h                                                                                            */
/*            This file contains Timer interface                                                           */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef __TIMER_IF_H__
#define __TIMER_IF_H__

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                  TYPES                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* TIMER Allocation Table example to be included in hal_config.h file                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* #define TIMER_DEFINITION_TABLE                                                          \               */
/* {                                                                                       \               */
/* { TIMER_HW_ITIM8,   ITIM8_MODULE_1  },          *//* TIMER_CHANNEL_1  *//*              \               */
/* { TIMER_HW_ITIM8,   ITIM8_MODULE_2  },          *//* TIMER_CHANNEL_2  *//*              \               */
/* { TIMER_HW_ITIM8,   ITIM8_MODULE_3  },          *//* TIMER_CHANNEL_3  *//*              \               */
/* { TIMER_HW_TWD,     0               },          *//* TIMER_CHANNEL_4  *//*              \               */
/* { TIMER_HW_SYST,    0               },          *//* TIMER_CHANNEL_5  *//*              \               */
/* }                                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Timer hardware type                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    TIMER_HW_ITIM8  = 0,
    TIMER_HW_TWD,
    TIMER_HW_SYST,
    TIMER_HW_MFT,
    TIMER_HW_TMC
} TIMER_HW_TYPE;

/*---------------------------------------------------------------------------------------------------------*/
/* Timer channel                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    TIMER_CHANNEL_1,
    TIMER_CHANNEL_2,
    TIMER_CHANNEL_3,
    TIMER_CHANNEL_4,
    TIMER_CHANNEL_5,
    TIMER_CHANNEL_6,
    TIMER_CHANNEL_7,
    TIMER_CHANNEL_8,
    TIMER_CHANNEL_9,
    TIMER_CHANNEL_10,
    TIMER_CHANNEL_11,
    TIMER_CHANNEL_12,
    TIMER_CHANNEL_13,
    TIMER_CHANNEL_14,
    TIMER_CHANNEL_15,
    TIMER_CHANNEL_16,

    TIMER_CHANNEL_NONE,
} TIMER_CHANNEL_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Timer operation mode                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    TIMER_OPMODE_ONESHOT_MODE,
    TIMER_OPMODE_PERIODIC_MODE,
    TIMER_OPMODE_NO_TICK,
} TIMER_OPMODE_T ;

/*---------------------------------------------------------------------------------------------------------*/
/* Timer event callback                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
typedef void (*TIMER_CALLBACK_T)(void* arg);

/*---------------------------------------------------------------------------------------------------------*/
/* Timer event execution type                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    TIMER_EVENT_EXEC_SINGLE,
    TIMER_EVENT_EXEC_PERIODIC,
} TIMER_EVENT_EXEC_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Timer working unit                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    TIMER_UNIT_SEC,
    TIMER_UNIT_MSEC,
    TIMER_UNIT_USEC,
    TIMER_UNIT_NANOSEC,
} TIMER_UNIT_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Timer allocation structure                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct
{
    TIMER_HW_TYPE       hwType;
    UINT16              hwNum;
} TIMER_ALLOC_T;

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Init                                                                             */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the TIMER module                                              */
/*---------------------------------------------------------------------------------------------------------*/
void TIMER_Init(void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Open                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch          - Timer channel                                                            */
/*                  unit        - Timer working units (sec/milli/micro/nano)                               */
/*                  tickPeriod  - Period between two ticks (in timer units)                                */
/*                  opMode      - Operation mode                                                           */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine opens a specific timer channel in given operation mode                    */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_Open(TIMER_CHANNEL_T ch, TIMER_UNIT_T unit, UINT32 tickPeriod, TIMER_OPMODE_T opMode);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Close                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch  - Timer channel                                                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine closes timer channel                                                      */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_Close(TIMER_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_SetTimerEvent                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch                  - Timer channel to use                                             */
/*                  periodToTrigger     - Period of time (in timer units) till event is triggered          */
/*                  eventType           - Event execution type                                             */
/*                  timerCallback       - Timer event                                                      */
/*                  timerCallbackArg    - Timer event parameter                                            */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets a new event for given timer channel, multiple events can be set per  */
/*                  single timer channel. (Number of multiple events per channel defined by TBD DEFINE)    */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_SetTimerEvent(TIMER_CHANNEL_T     ch,
                                UINT32              periodToTrigger,
                                TIMER_EVENT_EXEC_T  eventType,
                                TIMER_CALLBACK_T    timerCallback,
                                void*               timerCallbackArg);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_RemoveTimerEvent                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch              - Timer channel                                                        */
/*                  timerCallback   - Timer event to remove                                                */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine removes given timer event                                                 */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_RemoveTimerEvent(TIMER_CHANNEL_T ch, TIMER_CALLBACK_T timerCallback);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_ClearEvents                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch  - Timer channel                                                                    */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine cleans all events from given channel                                      */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_ClearEvents(TIMER_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Start                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch  - Timer channel                                                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine starts the given timer channel                                            */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_Start(TIMER_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Started                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch  - Timer channel                                                                    */
/*                                                                                                         */
/* Returns:         TRUE is timer is started or FALSE otherwise                                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if given timer channel was started                                 */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN TIMER_Started(TIMER_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Stop                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch  - Timer channel                                                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine stops the given timer channel                                             */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_Stop(TIMER_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Pause                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch  - Timer channel                                                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine pauses given timer channel tick count                                     */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_Pause(TIMER_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Continue                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch  - Timer channel                                                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine continues previously paused timer                                         */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_Continue(TIMER_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Reset                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch  - Timer channel                                                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine resets timer time count for given channel                                 */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TIMER_Reset(TIMER_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_GetTime                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch  - Timer channel                                                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if no errors occurred, or DEFS_STATUS_x on error                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns passed time (from start/reset) in timer units                     */
/*---------------------------------------------------------------------------------------------------------*/
UINT32 TIMER_GetTime(TIMER_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TIMER_Delay                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  time    - Amount of time to wait (in timer units)                                      */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine executes blocking delay for given amount of time (in timer units)         */
/*---------------------------------------------------------------------------------------------------------*/
void TIMER_Delay(UINT32 time);

#endif //__TIMER_IF_H__

