/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2013-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   scs_if.h                                                                                              */
/*            This file contains The System Control Space (SCS) interface                                  */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef _SCS_IF_H
#define _SCS_IF_H

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()

#if defined SCS_MODULE_TYPE
#include __MODULE_HEADER(scs, SCS_MODULE_TYPE)
#endif

#if defined NVIC_MODULE_TYPE
#include __MODULE_HEADER(nvic, NVIC_MODULE_TYPE)
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* ARM's supported Coprocessors                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    CP0     = 0,    /* do not change - match ARM spec */
    CP1     = 1,
    CP2     = 2,
    CP3     = 3,
    CP4     = 4,
    CP5     = 5,
    CP6     = 6,
    CP7     = 7,
    CP10    = 10,
    CP11    = 11,
} CP_NUM_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Access Privileges for ARM's Coprocessor                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    CP_ACCESS_DENIED            = 0,    /* do not change - match ARM spec */
    CP_PRIVILEGED_ACCESS_ONLY,
    CP_ACCESS_RESERVED,
    CP_FULL_ACCESS
} CP_ACCESS_TYPE_T;


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_Init                                                                               */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs enables all traps                                                */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_Init (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_GetActiveVectorNumber                                                              */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/*                                                                                                         */
/* Returns:         The exception number of the current executing exception.                               */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reads the ISR vector number of the current executing exception.           */
/*---------------------------------------------------------------------------------------------------------*/
UINT16 SCS_GetActiveVectorNumber (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_GetPendingVectorNumber                                                             */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/*                                                                                                         */
/* Returns:         The exception number of the current pending exception.                                 */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reads the ISR vector number of the current pending exception.             */
/*---------------------------------------------------------------------------------------------------------*/
UINT16 SCS_GetPendingVectorNumber (void);


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_SetVectorTable                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  table      - the vector table pointer.                                                 */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets the vector table pointer .                                           */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_SetVectorTable (UINT32 table);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_ClearPendingSysTickInt                                                             */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine clears the pending status of the SYST interrupt.                          */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_ClearPendingSysTickInt (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_SetTrapPriority                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  group_no  - priority register number                                                   */
/*                  input_no  - priority offset inside the register                                        */
/*                  priority - trap priority level                                                         */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets trap's priority.                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_SetTrapPriority (UINT8 group_no, UINT8 input_no, UINT8 priority);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_FPLazyContextSave                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  enable - TRUE to enable automatic lazy context save of FP state; FALSE otherwise.      */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables automatic lazy context save of FP state.                          */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_FPLazyContextSave (BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_FPStatePreservation                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  enable - TRUE to enable automatic FP state preservation; FALSE otherwise.              */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables automatic FP state preservation.                                  */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_FPStatePreservation (BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_FPEnableAccess                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  enable - TRUE to enable access to the Floating-point coprocessor; FALSE otherwise.     */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables access to the Floating-point coprocessor (CP10 and CP11)          */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_FPEnableAccess (BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_TraceEnable                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  enable - TRUE to enable all DWT and ITM blocks; FALSE otherwise.                       */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the Global enable for all DWT and ITM features.                */
/*                  To use the watchpoint and trace (DWT) features, such as cycle count, set the           */
/*                  DEMCR.TRCENA bit to 1                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_TraceEnable (BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_DivideByZeroEnable                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  enable - TRUE to enable Divide-By-Zero trap; FALSE otherwise.                          */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the Divide-By-Zero trap.                                       */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_DivideByZeroEnable (BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_EnableTrap                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  trap   - number of trap which should be configured.                                    */
/*                  enable - TRUE to enable the trap; FALSE otherwise.                                     */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables/disabled a given trap.                                            */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_EnableTrap (NVIC_TRAP_SRC_T trap, BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_PrintRegs (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_PrintVersion                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_PrintVersion (void);

#endif  /* _SCS_IF_H */

