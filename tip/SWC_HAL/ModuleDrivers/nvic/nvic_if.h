/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2013-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   nvic_if.h                                                                                             */
/*            This file contains Nested Vectored Interrupt Controller (NVIC) interface                     */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef _NVIC_IF_H
#define _NVIC_IF_H

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()

#if defined NVIC_MODULE_TYPE
#include __MODULE_HEADER(nvic, NVIC_MODULE_TYPE)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Trap Mapping                                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    NVIC_TRAP_INIT_SP     = 0,    /* Initialize Stack Pointer          */
    NVIC_TRAP_RESET       = 1,    /* Reset                             */
    NVIC_TRAP_NMI         = 2,    /* NMI (Non-Maskable Interrupt)      */
    NVIC_TRAP_HARD_FAULT  = 3,    /* Hardware fault                    */
    NVIC_TRAP_MEM_MANAGE  = 4,    /* Memory protection faults (MPU)    */
    NVIC_TRAP_BUS_FAULT   = 5,    /* Instruction/data memory faults    */
    NVIC_TRAP_USG_FAULT   = 6,    /* Non-memory related faults         */
    NVIC_TRAP_RES7        = 7,    /* Reserved                          */
    NVIC_TRAP_RES8        = 8,    /* Reserved                          */
    NVIC_TRAP_RES9        = 9,    /* Reserved                          */
    NVIC_TRAP_RES10       = 10,   /* Reserved                          */
    NVIC_TRAP_SVC         = 11,   /* Supervisor Call                   */
    NVIC_TRAP_DBG_MON     = 12,   /* Debug monitor fault               */
    NVIC_TRAP_RES13       = 13,   /* Reserved                          */
    NVIC_TRAP_PEND_SV     = 14,   /* Pending Supervisor Call Interrupt */
    NVIC_TRAP_SYST        = 15    /* System-Tick timer Interrupt       */
} NVIC_TRAP_SRC_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Interrupt Mapping                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    NVIC_INT_0   = 0,
    NVIC_INT_1,
    NVIC_INT_2,
    NVIC_INT_3,
    NVIC_INT_4,
    NVIC_INT_5,
    NVIC_INT_6,
    NVIC_INT_7,
    NVIC_INT_8,
    NVIC_INT_9,
    NVIC_INT_10,
    NVIC_INT_11,
    NVIC_INT_12,
    NVIC_INT_13,
    NVIC_INT_14,
    NVIC_INT_15,
    NVIC_INT_16,
    NVIC_INT_17,
    NVIC_INT_18,
    NVIC_INT_19,
    NVIC_INT_20,
    NVIC_INT_21,
    NVIC_INT_22,
    NVIC_INT_23,
    NVIC_INT_24,
    NVIC_INT_25,
    NVIC_INT_26,
    NVIC_INT_27,
    NVIC_INT_28,
    NVIC_INT_29,
    NVIC_INT_30,
    NVIC_INT_31,
    NVIC_INT_32,
    NVIC_INT_33,
    NVIC_INT_34,
    NVIC_INT_35,
    NVIC_INT_36,
    NVIC_INT_37,
    NVIC_INT_38,
    NVIC_INT_39,
    NVIC_INT_40,
    NVIC_INT_41,
    NVIC_INT_42,
    NVIC_INT_43,
    NVIC_INT_44,
    NVIC_INT_45,
    NVIC_INT_46,
    NVIC_INT_47,
    NVIC_INT_48,
    NVIC_INT_49,
    NVIC_INT_50,
    NVIC_INT_51,
    NVIC_INT_52,
    NVIC_INT_53,
    NVIC_INT_54,
    NVIC_INT_55,
    NVIC_INT_56,
    NVIC_INT_57,
    NVIC_INT_58,
    NVIC_INT_59,
    NVIC_INT_60,
    NVIC_INT_61,
    NVIC_INT_62,
    NVIC_INT_63,
    NVIC_INT_64,
    NVIC_INT_65,
    NVIC_INT_66,
    NVIC_INT_67,
    NVIC_INT_68,
    NVIC_INT_69,
    NVIC_INT_70,
    NVIC_INT_71,
    NVIC_INT_72,
    NVIC_INT_73,
    NVIC_INT_74,
    NVIC_INT_75,
    NVIC_INT_76,
    NVIC_INT_77,
    NVIC_INT_78,
    NVIC_INT_79,
    NVIC_INT_80,
    NVIC_INT_81,
    NVIC_INT_82,
    NVIC_INT_83,
    NVIC_INT_84,
    NVIC_INT_85,
    NVIC_INT_86,
    NVIC_INT_87,
    NVIC_INT_88,
    NVIC_INT_89,
    NVIC_INT_90,
    NVIC_INT_91,
    NVIC_INT_92,
    NVIC_INT_93,
    NVIC_INT_94,
    NVIC_INT_95,

    NVIC_INT_UNDEF = 0xFF
} NVIC_INT_SRC_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Interrupt Dispatch Table Size (includes all traps and interrupts)                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_TABLE_SIZE          (NVIC_TRAP_NUM + NVIC_INTERRUPT_NUM)

/*---------------------------------------------------------------------------------------------------------*/
/* First table enteries are dedicated for traps, followed by interrupts                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_INT_BASE            NVIC_TRAP_NUM

#ifdef EXTERNAL_DISPATCH_TABLE
/* External dispatch table must be static */
#define STATIC_HW_DISPATCH_TABLE
#define STATIC_SW_DISPATCH_TABLE
#else

/* Default HW Dispatch Table is Static */
#ifndef DYNAMIC_HW_DISPATCH_TABLE
#define STATIC_HW_DISPATCH_TABLE
#endif

/* Default SW Dispatch Table is Dynamic */
#ifndef STATIC_SW_DISPATCH_TABLE
#define DYNAMIC_SW_DISPATCH_TABLE
#endif

/* NVIC_ISR can be defined extenaly. Default: NVIC_Isr_L */
#ifndef NVIC_ISR
#define NVIC_ISR NVIC_Isr_L
#else
extern _ISR_ void NVIC_ISR (void);
#define EXTERNAL_NVIC_ISR
#endif
#endif


#ifdef STATIC_HW_DISPATCH_TABLE
/*---------------------------------------------------------------------------------------------------------*/
/* Allow the same code for static dispatch table (handler installation is ignored)                         */
/*---------------------------------------------------------------------------------------------------------*/
#define     NVIC_InstallHwHandler(INT_NO, PROC)    {}
#define     NVIC_InstallHwTrap(TRAP_NO, PROC)      {}
#endif

#ifdef STATIC_SW_DISPATCH_TABLE
/*---------------------------------------------------------------------------------------------------------*/
/* Allow the same code for static dispatch table (handler installation is ignored)                         */
/*---------------------------------------------------------------------------------------------------------*/
#define     NVIC_InstallSwHandler(INT_NO, PROC)    {}
#define     NVIC_InstallSwTrap(TRAP_NO, PROC)      {}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Hardware dispatch table entry type                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
TYPEDEF_ISR_PTR(HW_HANDLER_T);

/*---------------------------------------------------------------------------------------------------------*/
/* Software dispatch table entry type                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
typedef void (*SW_HANDLER_T)(UINT16 int_num);

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_Init                                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  inttable - TRUE: Install null_handlers for all interrupts                              */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:    This function only sets the Global Interrupt Enable (I bit in PSR register).           */
/*                  The Local Interrupt Enable (E bit in PSR register) remains cleared.                    */
/*                  The upper-layer should call ENABLE_INTERRUPTS to set the E bit when desired.           */
/* Description:                                                                                            */
/*                  This routine:                                                                          */
/*                  1. Masks all interrupts.                                                               */
/*                  2. Clears all pending interrupts.                                                      */
/*                  3. if inttable is TRUE: Installs the default interrupt dispatch-table                  */
/*                     (null_handlers for all interrupts).                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_Init (BOOLEAN inttable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_Reset                                                                             */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine resets NVIC module                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_Reset (void);

#ifndef EXTERNAL_DISPATCH_TABLE
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_InitTab                                                                           */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the dispatch-table (interrupt-vector).                        */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_InitTab (void);

#endif /* EXTERNAL_DISPATCH_TABLE */

#ifndef STATIC_HW_DISPATCH_TABLE
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_InstallHwTrap                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  trap_no - number of trap for which the handler should be installed.                    */
/*                  proc   - trap handler procedure to be installed.                                       */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine installs the provided trap handler in the dispatch table.                 */
/*                  The function is used to install or change the trap handler when the                    */
/*                  dispatch-table is placed in RAM.                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_InstallHwTrap (NVIC_TRAP_SRC_T trap_no, HW_HANDLER_T proc);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_InstallHwHandler                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_no - number of interrupt for which the handler should be installed.                */
/*                  proc   - interrupt handler procedure to be installed.                                  */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine installs the provided interrupt handler in the dispatch table.            */
/*                  The function is used to install or change the interrupt handler when the               */
/*                  dispatch-table is placed in RAM.                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_InstallHwHandler (NVIC_INT_SRC_T int_no, HW_HANDLER_T proc);

#endif  /* STATIC_HW_DISPATCH_TABLE */

#ifndef STATIC_SW_DISPATCH_TABLE
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_InstallSwTrap                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  trap_no - number of trap for which the handler should be installed.                    */
/*                  proc   - trap handler procedure to be installed.                                       */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine installs the provided trap handler in the dispatch table.                 */
/*                  The function is used to install or change the trap handler when the                    */
/*                  dispatch-table is placed in RAM.                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_InstallSwTrap (NVIC_TRAP_SRC_T trap_no, SW_HANDLER_T proc);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_InstallSwHandler                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_no - number of interrupt for which the handler should be installed.                */
/*                  proc   - interrupt handler procedure to be installed.                                  */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine installs the provided interrupt handler in the dispatch table.            */
/*                  The function is used to install or change the interrupt handler when the               */
/*                  dispatch-table is placed in RAM.                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_InstallSwHandler (NVIC_INT_SRC_T int_no, SW_HANDLER_T proc);

#endif  /* STATIC_SW_DISPATCH_TABLE */

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_EnableInt                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_no  - interrupt number                                                             */
/*                  enable  - TRUE to enable interrupt, FALSE to disable                                   */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables/disables interrupt number int_no.                                 */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_EnableInt (NVIC_INT_SRC_T int_no, BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_IntEnabled                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_no    - Number of interrupt to check if enabled                                    */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if given interrupt is enabled                                      */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN NVIC_IntEnabled (NVIC_INT_SRC_T int_no);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_ClearInt                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_no - interrupt number                                                              */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine clears interrupt number int_no.                                           */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_ClearInt (NVIC_INT_SRC_T int_no);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_PendingInt                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_no - interrupt number                                                              */
/*                                                                                                         */
/* Returns:         interrupts status                                                                      */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns TRUE if given int_no interrupt is pending                         */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN NVIC_PendingInt (NVIC_INT_SRC_T int_no);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_ConfigPriority                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_no   - interrupt number                                                            */
/*                  priority - interrupt priority level                                                    */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets int_no interrupt's priority.                                         */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_ConfigPriority (NVIC_INT_SRC_T int_no, UINT8 priority);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_ConfigTrapPriority                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  trap_no  - trap number                                                                 */
/*                  priority - trap priority level                                                         */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets trap_no trap's priority.                                             */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_ConfigTrapPriority (NVIC_TRAP_SRC_T trap_no, UINT8 priority);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_Isr_L                                                                             */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine implements Interrupt Service Routine                                      */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef EXTERNAL_NVIC_ISR
_ISR_
#endif
void NVIC_Isr_L (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_PrintRegs                                                                         */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_PrintRegs (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_PrintVersion                                                                      */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_PrintVersion (void);

#endif /* _NVIC_IF_H */

