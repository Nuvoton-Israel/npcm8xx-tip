/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2013-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   nvic_drv.c                                                                                            */
/*            This file contains Nested Vectored Interrupt Controller (NVIC) driver implementation         */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_DRV()

#include "nvic_drv.h"
#include "nvic_regs.h"


/*---------------------------------------------------------------------------------------------------------*/
/* Module Dependencies                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#if defined SCS_MODULE_TYPE
#include __MODULE_IF_HEADER_FROM_DRV(scs)
#endif

#define NVIC_MSG_DEBUG(fmt,args...)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Maximum number of interrupts supported by ARMv7-M NVIC architecture                                     */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef NVIC_MAX_NUM_OF_INTERRUPTS
#define NVIC_MAX_NUM_OF_INTERRUPTS                 496
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Maximum number of exceptions (traps + interrupts) supported by ARMv7-M NVIC architecture                */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_MAX_NUM_OF_EXCEPTIONS                 (NVIC_MAX_NUM_OF_INTERRUPTS + NVIC_TRAP_NUM)

/*---------------------------------------------------------------------------------------------------------*/
/* Number of interrupts groups. Each groups contains 32 interrupts, total of 16 groups for 496 interrupts  */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_NUM_OF_INTERRUPT_GROUPS               (NVIC_MAX_NUM_OF_EXCEPTIONS / 32)

/*---------------------------------------------------------------------------------------------------------*/
/* Number of implemented exceptions (traps + interrupts)                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_NUM_OF_EXCEPTIONS                     (NVIC_INTERRUPT_NUM + NVIC_TRAP_NUM)

/*---------------------------------------------------------------------------------------------------------*/
/* Number of interrupts priority levels                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_PRIORITY_LEVELS                       (1 << NVIC_PRIORITY_BITS)

/*---------------------------------------------------------------------------------------------------------*/
/* Vrify the number of implemented exceptions is no more than the maximum number of exceptions supported   */
/*---------------------------------------------------------------------------------------------------------*/
#if (NVIC_NUM_OF_EXCEPTIONS > NVIC_MAX_NUM_OF_EXCEPTIONS)
#error "Unsupported number of exceptions"
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* The Vector table must be naturally aligned to a power of two whose alignment value is greater than or   */
/* equal to (Number of Exceptions supported x 4), with a minimum alignment of 128 bytes                    */
/* Quated from ARM DDI 0403D                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_VECTOR_TABLE_ALIGNMENT                ((NVIC_NUM_OF_EXCEPTIONS <= (32 << 0)) ? (32 << 2) :    \
                                                   ((NVIC_NUM_OF_EXCEPTIONS <= (32 << 1)) ? (32 << 3) :    \
                                                   ((NVIC_NUM_OF_EXCEPTIONS <= (32 << 2)) ? (32 << 4) :    \
                                                   ((NVIC_NUM_OF_EXCEPTIONS <= (32 << 3)) ? (32 << 5) :    \
                                                                                            (32 << 6)))))

/*---------------------------------------------------------------------------------------------------------*/
/* Extract Interupt Group number and input offset given the input number                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_GET_INTERRUPT_INPUT_NUM(nvic_num)     (UINT8)(nvic_num % 32)
#define NVIC_GET_INTERRUPT_GROUP_NUM(nvic_num)     (UINT8)(nvic_num / 32)

/*---------------------------------------------------------------------------------------------------------*/
/* Extract Priority Group number and input offset given the input number                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_GET_PRIORITY_INPUT_NUM(nvic_num)      (UINT8)(nvic_num % 4)
#define NVIC_GET_PRIORITY_GROUP_NUM(nvic_num)      (UINT8)(nvic_num / 4)

/*---------------------------------------------------------------------------------------------------------*/
/* Macro for defining TRAP routing functions                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define TRAP_ROUTER_FUNCTION(HANDLER, INT_NUM)                  \
_ISR_ void HANDLER ()                                           \
{                                                               \
    (NVIC_SW_DispatchTable[INT_NUM])(INT_NUM);                  \
}

#define TRAP_ROUTER_FUNCTION_NO_RET(HANDLER, INT_NUM)           \
_NORET_SPEC_(_ISR_ void HANDLER ())                             \
{                                                               \
    (NVIC_SW_DispatchTable[INT_NUM])(INT_NUM);                  \
    for(;;){}                                                   \
}


#ifndef EXTERNAL_DISPATCH_TABLE

/*---------------------------------------------------------------------------------------------------------*/
/* Use the nvic driver internal dispatch table,                                                            */
/* otherwise assume that the table is already defined outside of this driver.                              */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                       LOCAL FUNCTIONS DECLARATION                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* TRAP Handler                                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
_ISR_ void NVIC_TrapRouting_RES0_L (void);
_ISR_ void NVIC_TrapRouting_RST_L  (void);
_ISR_ void NVIC_TrapRouting_NMI_L  (void);
_ISR_ void NVIC_TrapRouting_HDF_L  (void);
_ISR_ void NVIC_TrapRouting_MMNG_L (void);
_ISR_ void NVIC_TrapRouting_BSF_L  (void);
_ISR_ void NVIC_TrapRouting_USGF_L (void);
_ISR_ void NVIC_TrapRouting_RES7_L (void);
_ISR_ void NVIC_TrapRouting_RES8_L (void);
_ISR_ void NVIC_TrapRouting_RES9_L (void);
_ISR_ void NVIC_TrapRouting_RES10_L(void);
_ISR_ void NVIC_TrapRouting_SVC_L  (void);
_ISR_ void NVIC_TrapRouting_DBG_L  (void);
_ISR_ void NVIC_TrapRouting_RES13_L(void);
_ISR_ void NVIC_TrapRouting_PSV_L  (void);
_ISR_ void NVIC_TrapRouting_SYST_L (void);


/*---------------------------------------------------------------------------------------------------------*/
/* DEFAULT HW/SW HANDLER                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
_ISR_ void NVIC_Default_HW_Handler_L (void);
      void NVIC_Default_SW_Handler_L (UINT16 int_num);


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                          CONSTANTS & VARIABLES                                          */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Hardware Dispatch Table                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef STATIC_HW_DISPATCH_TABLE
static const HW_HANDLER_T NVIC_HW_DefaultTrapTable[NVIC_TRAP_NUM] =
{
    /* Traps */
    /*  0 */    NVIC_TrapRouting_RES0_L,
    /*  1 */    NVIC_TrapRouting_RST_L,
    /*  2 */    NVIC_TrapRouting_NMI_L,
    /*  3 */    NVIC_TrapRouting_HDF_L,
    /*  4 */    NVIC_TrapRouting_MMNG_L,
    /*  5 */    NVIC_TrapRouting_BSF_L,
    /*  6 */    NVIC_TrapRouting_USGF_L,
    /*  7 */    NVIC_TrapRouting_RES7_L,
    /*  8 */    NVIC_TrapRouting_RES8_L,
    /*  9 */    NVIC_TrapRouting_RES9_L,
    /* 10 */    NVIC_TrapRouting_RES10_L,
    /* 11 */    NVIC_TrapRouting_SVC_L,
    /* 12 */    NVIC_TrapRouting_DBG_L,
    /* 13 */    NVIC_TrapRouting_RES13_L,
    /* 14 */    NVIC_TrapRouting_PSV_L,
    /* 15 */    NVIC_TrapRouting_SYST_L,
};

_ALIGN_(NVIC_VECTOR_TABLE_ALIGNMENT, static HW_HANDLER_T  __attribute__((section (".tiprom_ram_share")))   NVIC_HW_DispatchTable[NVIC_TABLE_SIZE]);
#else
_ALIGN_(NVIC_VECTOR_TABLE_ALIGNMENT, static const HW_HANDLER_T  __attribute__((section (".tiprom_ram_share")))   NVIC_HW_DispatchTable[NVIC_TABLE_SIZE]) =
{
    /* Traps */
    /*  0 */    NVIC_TrapRouting_RES0_L,
    /*  1 */    NVIC_TrapRouting_RST_L,
    /*  2 */    NVIC_TrapRouting_NMI_L,
    /*  3 */    NVIC_TrapRouting_HDF_L,
    /*  4 */    NVIC_TrapRouting_MMNG_L,
    /*  5 */    NVIC_TrapRouting_BSF_L,
    /*  6 */    NVIC_TrapRouting_USGF_L,
    /*  7 */    NVIC_TrapRouting_RES7_L,
    /*  8 */    NVIC_TrapRouting_RES8_L,
    /*  9 */    NVIC_TrapRouting_RES9_L,
    /* 10 */    NVIC_TrapRouting_RES10_L,
    /* 11 */    NVIC_TrapRouting_SVC_L,
    /* 12 */    NVIC_TrapRouting_DBG_L,
    /* 13 */    NVIC_TrapRouting_RES13_L,
    /* 14 */    NVIC_TrapRouting_PSV_L,
    /* 15 */    NVIC_TrapRouting_SYST_L,
    /* Interrupts */
    /*  0 */    NVIC_ISR,
    /*  1 */    NVIC_ISR,
    /*  2 */    NVIC_ISR,
    /*  3 */    NVIC_ISR,
    /*  4 */    NVIC_ISR,
    /*  5 */    NVIC_ISR,
    /*  6 */    NVIC_ISR,
    /*  7 */    NVIC_ISR,
    /*  8 */    NVIC_ISR,
    /*  9 */    NVIC_ISR,
    /* 10 */    NVIC_ISR,
    /* 11 */    NVIC_ISR,
    /* 12 */    NVIC_ISR,
    /* 13 */    NVIC_ISR,
    /* 14 */    NVIC_ISR,
    /* 15 */    NVIC_ISR,
    /* 16 */    NVIC_ISR,
    /* 17 */    NVIC_ISR,
    /* 18 */    NVIC_ISR,
    /* 19 */    NVIC_ISR,
    /* 20 */    NVIC_ISR,
    /* 21 */    NVIC_ISR,
    /* 22 */    NVIC_ISR,
    /* 23 */    NVIC_ISR,
    /* 24 */    NVIC_ISR,
    /* 25 */    NVIC_ISR,
    /* 26 */    NVIC_ISR,
    /* 27 */    NVIC_ISR,
    /* 28 */    NVIC_ISR,
    /* 29 */    NVIC_ISR,
    /* 30 */    NVIC_ISR,
    /* 31 */    NVIC_ISR,
    /* 32 */    NVIC_ISR,
    /* 33 */    NVIC_ISR,
    /* 34 */    NVIC_ISR,
    /* 35 */    NVIC_ISR,
    /* 36 */    NVIC_ISR,
    /* 37 */    NVIC_ISR,
    /* 38 */    NVIC_ISR,
    /* 39 */    NVIC_ISR,
    /* 40 */    NVIC_ISR,
    /* 41 */    NVIC_ISR,
    /* 42 */    NVIC_ISR,
    /* 43 */    NVIC_ISR,
    /* 44 */    NVIC_ISR,
    /* 45 */    NVIC_ISR,
    /* 46 */    NVIC_ISR,
    /* 47 */    NVIC_ISR,
    /* 48 */    NVIC_ISR,
    /* 49 */    NVIC_ISR,
    /* 50 */    NVIC_ISR,
    /* 51 */    NVIC_ISR,
    /* 52 */    NVIC_ISR,
    /* 53 */    NVIC_ISR,
    /* 54 */    NVIC_ISR,
    /* 55 */    NVIC_ISR,
    /* 56 */    NVIC_ISR,
    /* 57 */    NVIC_ISR,
    /* 58 */    NVIC_ISR,
    /* 59 */    NVIC_ISR,
    /* 60 */    NVIC_ISR,
    /* 61 */    NVIC_ISR,
    /* 62 */    NVIC_ISR,
    /* 63 */    NVIC_ISR,
#if (NVIC_INTERRUPT_NUM > 64)
    /* 64 */    NVIC_ISR,
    /* 65 */    NVIC_ISR,
    /* 66 */    NVIC_ISR,
    /* 67 */    NVIC_ISR,
    /* 68 */    NVIC_ISR,
    /* 69 */    NVIC_ISR,
    /* 70 */    NVIC_ISR,
    /* 71 */    NVIC_ISR,
    /* 72 */    NVIC_ISR,
    /* 73 */    NVIC_ISR,
    /* 74 */    NVIC_ISR,
    /* 75 */    NVIC_ISR,
    /* 76 */    NVIC_ISR,
    /* 77 */    NVIC_ISR,
    /* 78 */    NVIC_ISR,
    /* 79 */    NVIC_ISR,
    /* 80 */    NVIC_ISR,
    /* 81 */    NVIC_ISR,
    /* 82 */    NVIC_ISR,
    /* 83 */    NVIC_ISR,
    /* 84 */    NVIC_ISR,
    /* 85 */    NVIC_ISR,
    /* 86 */    NVIC_ISR,
    /* 87 */    NVIC_ISR,
    /* 88 */    NVIC_ISR,
    /* 89 */    NVIC_ISR,
    /* 90 */    NVIC_ISR,
    /* 91 */    NVIC_ISR,
    /* 92 */    NVIC_ISR,
    /* 93 */    NVIC_ISR,
    /* 94 */    NVIC_ISR,
    /* 95 */    NVIC_ISR,
#endif
};
#endif  /* STATIC_HW_DISPATCH_TABLE */

/*---------------------------------------------------------------------------------------------------------*/
/* Software Dispatch Table                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef STATIC_SW_DISPATCH_TABLE
static SW_HANDLER_T __attribute__((section (".tiprom_ram_share")))  NVIC_SW_DispatchTable[NVIC_TABLE_SIZE];
#else
const static SW_HANDLER_T  __attribute__((section (".tiprom_ram_share"))) NVIC_SW_DispatchTable[NVIC_TABLE_SIZE]
{
    /* Traps */
    /*  0 */    NVIC_Default_SW_Handler_L,
    /*  1 */    NVIC_Default_SW_Handler_L,
    /*  2 */    NVIC_Default_SW_Handler_L,
    /*  3 */    NVIC_Default_SW_Handler_L,
    /*  4 */    NVIC_Default_SW_Handler_L,
    /*  5 */    NVIC_Default_SW_Handler_L,
    /*  6 */    NVIC_Default_SW_Handler_L,
    /*  7 */    NVIC_Default_SW_Handler_L,
    /*  8 */    NVIC_Default_SW_Handler_L,
    /*  9 */    NVIC_Default_SW_Handler_L,
    /* 10 */    NVIC_Default_SW_Handler_L,
    /* 11 */    NVIC_Default_SW_Handler_L,
    /* 12 */    NVIC_Default_SW_Handler_L,
    /* 13 */    NVIC_Default_SW_Handler_L,
    /* 14 */    NVIC_Default_SW_Handler_L,
    /* 15 */    NVIC_Default_SW_Handler_L,
    /* Interrupts */
    /*  0 */    NVIC_Default_SW_Handler_L,
    /*  1 */    NVIC_Default_SW_Handler_L,
    /*  2 */    NVIC_Default_SW_Handler_L,
    /*  3 */    NVIC_Default_SW_Handler_L,
    /*  4 */    NVIC_Default_SW_Handler_L,
    /*  5 */    NVIC_Default_SW_Handler_L,
    /*  6 */    NVIC_Default_SW_Handler_L,
    /*  7 */    NVIC_Default_SW_Handler_L,
    /*  8 */    NVIC_Default_SW_Handler_L,
    /*  9 */    NVIC_Default_SW_Handler_L,
    /* 10 */    NVIC_Default_SW_Handler_L,
    /* 11 */    NVIC_Default_SW_Handler_L,
    /* 12 */    NVIC_Default_SW_Handler_L,
    /* 13 */    NVIC_Default_SW_Handler_L,
    /* 14 */    NVIC_Default_SW_Handler_L,
    /* 15 */    NVIC_Default_SW_Handler_L,
    /* 16 */    NVIC_Default_SW_Handler_L,
    /* 17 */    NVIC_Default_SW_Handler_L,
    /* 18 */    NVIC_Default_SW_Handler_L,
    /* 19 */    NVIC_Default_SW_Handler_L,
    /* 20 */    NVIC_Default_SW_Handler_L,
    /* 21 */    NVIC_Default_SW_Handler_L,
    /* 22 */    NVIC_Default_SW_Handler_L,
    /* 23 */    NVIC_Default_SW_Handler_L,
    /* 24 */    NVIC_Default_SW_Handler_L,
    /* 25 */    NVIC_Default_SW_Handler_L,
    /* 26 */    NVIC_Default_SW_Handler_L,
    /* 27 */    NVIC_Default_SW_Handler_L,
    /* 28 */    NVIC_Default_SW_Handler_L,
    /* 29 */    NVIC_Default_SW_Handler_L,
    /* 30 */    NVIC_Default_SW_Handler_L,
    /* 31 */    NVIC_Default_SW_Handler_L,
    /* 32 */    NVIC_Default_SW_Handler_L,
    /* 33 */    NVIC_Default_SW_Handler_L,
    /* 34 */    NVIC_Default_SW_Handler_L,
    /* 35 */    NVIC_Default_SW_Handler_L,
    /* 36 */    NVIC_Default_SW_Handler_L,
    /* 37 */    NVIC_Default_SW_Handler_L,
    /* 38 */    NVIC_Default_SW_Handler_L,
    /* 39 */    NVIC_Default_SW_Handler_L,
    /* 40 */    NVIC_Default_SW_Handler_L,
    /* 41 */    NVIC_Default_SW_Handler_L,
    /* 42 */    NVIC_Default_SW_Handler_L,
    /* 43 */    NVIC_Default_SW_Handler_L,
    /* 44 */    NVIC_Default_SW_Handler_L,
    /* 45 */    NVIC_Default_SW_Handler_L,
    /* 46 */    NVIC_Default_SW_Handler_L,
    /* 47 */    NVIC_Default_SW_Handler_L,
    /* 48 */    NVIC_Default_SW_Handler_L,
    /* 49 */    NVIC_Default_SW_Handler_L,
    /* 50 */    NVIC_Default_SW_Handler_L,
    /* 51 */    NVIC_Default_SW_Handler_L,
    /* 52 */    NVIC_Default_SW_Handler_L,
    /* 53 */    NVIC_Default_SW_Handler_L,
    /* 54 */    NVIC_Default_SW_Handler_L,
    /* 55 */    NVIC_Default_SW_Handler_L,
    /* 56 */    NVIC_Default_SW_Handler_L,
    /* 57 */    NVIC_Default_SW_Handler_L,
    /* 58 */    NVIC_Default_SW_Handler_L,
    /* 59 */    NVIC_Default_SW_Handler_L,
    /* 60 */    NVIC_Default_SW_Handler_L,
    /* 61 */    NVIC_Default_SW_Handler_L,
    /* 62 */    NVIC_Default_SW_Handler_L,
    /* 63 */    NVIC_Default_SW_Handler_L,
};
#endif  /* STATIC_SW_DISPATCH_TABLE */


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_InitTab                                                                           */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the dispatch-table (interrupt-vector).                        */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_InitTab (void)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* PSR register is read into this variable                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    UINT    saved_psr = 0;
#if !defined (STATIC_HW_DISPATCH_TABLE) || !defined (STATIC_SW_DISPATCH_TABLE)
    UINT    int_no;
#endif

    INTERRUPTS_SAVE_DISABLE(saved_psr);

#ifndef STATIC_HW_DISPATCH_TABLE
    /*-----------------------------------------------------------------------------------------------------*/
    /* Initial Traps                                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    for (int_no = 0; int_no < NVIC_TRAP_NUM; int_no++)
    {
        NVIC_HW_DispatchTable[int_no] = NVIC_HW_DefaultTrapTable[int_no];
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Initial Interrupts                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    for (int_no = NVIC_INT_BASE; int_no < NVIC_TABLE_SIZE; int_no++)
    {
        NVIC_HW_DispatchTable[int_no] = NVIC_ISR;
    }
#endif  /* STATIC_HW_DISPATCH_TABLE */

#ifndef STATIC_SW_DISPATCH_TABLE
    /*-----------------------------------------------------------------------------------------------------*/
    /* Initial Interrupts & Traps                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    for (int_no = 0; int_no < NVIC_TABLE_SIZE; int_no++)
    {
        NVIC_SW_DispatchTable[int_no] = NVIC_Default_SW_Handler_L;
    }
#endif  /* STATIC_SW_DISPATCH_TABLE */

#ifdef SCS_MODULE_TYPE
    SCS_SetVectorTable((UINT32)NVIC_SW_DispatchTable);
#endif

    INTERRUPTS_RESTORE(saved_psr);
}
#endif /* EXTERNAL_DISPATCH_TABLE */


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
/*lint -e{715}      Suppress 'inttable' not referenced                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_Init (BOOLEAN inttable)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable Interrupts                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    DISABLE_INTERRUPTS();

    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable all interrupts and clear pending interrupts                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    NVIC_Reset();

#ifndef EXTERNAL_DISPATCH_TABLE
    if (inttable)
    {
        NVIC_InitTab();
    }
#endif
    /*-----------------------------------------------------------------------------------------------------*/
    /* Note that at this point interrupts are still disabled, since the E bit in PSR register is cleared.  */
    /* The upper-layer should call ENABLE_INTERRUPTS to set the E bit when desired                         */
    /*-----------------------------------------------------------------------------------------------------*/
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_Reset                                                                             */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine resets NVIC module                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_Reset (void)
{
    UINT group_no;

    for (group_no = 0; group_no < NVIC_NUM_OF_INTERRUPT_GROUPS; group_no++)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Mask All Interrupts                                                                             */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(NVIC_ICER(group_no), 0xFFFFFFFF);

        /*-------------------------------------------------------------------------------------------------*/
        /* Clear all pending interrupts                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(NVIC_ICPR(group_no), 0xFFFFFFFF);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_ClearAll                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine resets NVIC module                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_ClearAll (void)
{
    UINT group_no;

    for (group_no = 0; group_no < NVIC_NUM_OF_INTERRUPT_GROUPS; group_no++)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Clear all pending interrupts                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(NVIC_ICPR(group_no), 0xFFFFFFFF);
    }
}


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
void NVIC_InstallHwTrap (
    NVIC_TRAP_SRC_T  trap_no,
    HW_HANDLER_T     proc
)
{
    ASSERT(trap_no < NVIC_TRAP_NUM);

    ATOMIC_OP((NVIC_HW_DispatchTable[trap_no] = proc));
}


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
void NVIC_InstallHwHandler (
    NVIC_INT_SRC_T   int_no,
    HW_HANDLER_T     proc
)
{
    ASSERT(int_no < NVIC_INTERRUPT_NUM);

    ATOMIC_OP((NVIC_HW_DispatchTable[NVIC_INT_BASE + int_no] = proc));
}
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
void NVIC_InstallSwTrap (
    NVIC_TRAP_SRC_T  trap_no,
    SW_HANDLER_T     proc
)
{
    ASSERT(trap_no < NVIC_TRAP_NUM);

    NVIC_MSG_DEBUG("Install trap %d proc %#010lx \n", trap_no, (UINT32)proc);

    ATOMIC_OP((NVIC_SW_DispatchTable[trap_no] = proc));
}


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
/*lint -e{661,662}  Suppress Possible access of out-of-bounds pointer                                      */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_InstallSwHandler (
    NVIC_INT_SRC_T   int_no,
    SW_HANDLER_T     proc
)
{
    ASSERT(int_no < NVIC_INTERRUPT_NUM);

    NVIC_MSG_DEBUG("Install int %d proc %#010lx \n", int_no, (UINT32)proc);


    ATOMIC_OP((NVIC_SW_DispatchTable[NVIC_INT_BASE + int_no] = proc));
}
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
void NVIC_EnableInt (NVIC_INT_SRC_T int_no, BOOLEAN enable)
{
    UINT8  group_no;
    UINT8  input_no;
    UINT32 iser = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check for Illegal Cases                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    ASSERT(int_no < NVIC_INTERRUPT_NUM);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Determine the interrupt register and its offset inside the register                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    group_no = NVIC_GET_INTERRUPT_GROUP_NUM(int_no);
    input_no = NVIC_GET_INTERRUPT_INPUT_NUM(int_no);

    if (enable)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Enable interrupt                                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        SET_REG_BIT(NVIC_ISER(group_no), input_no);
    }
    else
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Disable interrupt                                                                               */
        /*-------------------------------------------------------------------------------------------------*/
        SET_VAR_BIT(iser, input_no);
        REG_WRITE(NVIC_ICER(group_no), iser);
    }
}


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
BOOLEAN NVIC_IntEnabled (NVIC_INT_SRC_T int_no)
{
    UINT8 group_no;
    UINT8 input_no;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check for Illegal Cases                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    ASSERT(int_no < NVIC_INTERRUPT_NUM);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Determine the interrupt register and its offset inside the register                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    group_no = NVIC_GET_INTERRUPT_GROUP_NUM(int_no);
    input_no = NVIC_GET_INTERRUPT_INPUT_NUM(int_no);

    return (BOOLEAN)READ_REG_BIT(NVIC_ISER(group_no), input_no);
}


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
void NVIC_ClearInt (NVIC_INT_SRC_T int_no)
{
    UINT8  group_no;
    UINT8  input_no;
    UINT32 icpr = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check for Illegal Cases                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    ASSERT(int_no < NVIC_INTERRUPT_NUM);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Determine the interrupt register and its offset inside the register                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    group_no = NVIC_GET_INTERRUPT_GROUP_NUM(int_no);
    input_no = NVIC_GET_INTERRUPT_INPUT_NUM(int_no);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear the pending interrupt                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_VAR_BIT(icpr, input_no);
    REG_WRITE(NVIC_ICPR(group_no), icpr);
}


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
BOOLEAN NVIC_PendingInt (NVIC_INT_SRC_T int_no)
{
    UINT8 group_no;
    UINT8 input_no;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check for Illegal Cases                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    ASSERT(int_no < NVIC_INTERRUPT_NUM);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Determine the interrupt register and its offset inside the register                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    group_no = NVIC_GET_INTERRUPT_GROUP_NUM(int_no);
    input_no = NVIC_GET_INTERRUPT_INPUT_NUM(int_no);

    return (BOOLEAN)READ_REG_BIT(NVIC_ISPR(group_no), input_no);
}


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
void NVIC_ConfigPriority (NVIC_INT_SRC_T int_no, UINT8 priority)
{
    UINT8 group_no;
    UINT8 input_no;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check for Illegal Cases                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    ASSERT(int_no < NVIC_INTERRUPT_NUM);
    ASSERT(priority < NVIC_PRIORITY_LEVELS);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Determine the priority register and its offset inside the register                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    group_no = NVIC_GET_PRIORITY_GROUP_NUM(int_no);
    input_no = NVIC_GET_PRIORITY_INPUT_NUM(int_no);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Determine the actual priority value to be set                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    priority <<= (8 - NVIC_PRIORITY_BITS);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the priority                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(NVIC_IPR(group_no), NVIC_IPR_PRI_N(input_no), priority);
}


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
void NVIC_ConfigTrapPriority (NVIC_TRAP_SRC_T trap_no, UINT8 priority)
{
    UINT8 group_no;
    UINT8 input_no;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check for Illegal Cases                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    ASSERT(trap_no < NVIC_TRAP_NUM);
    ASSERT(trap_no > NVIC_TRAP_HARD_FAULT);
    ASSERT(priority < NVIC_PRIORITY_LEVELS);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Determine the priority register and its offset inside the register                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    group_no = NVIC_GET_PRIORITY_GROUP_NUM(trap_no) - 1;
    input_no = NVIC_GET_PRIORITY_INPUT_NUM(trap_no);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Determine the actual priority value to be set                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    priority <<= (8 - NVIC_PRIORITY_BITS);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the priority                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
#ifdef SCS_MODULE_TYPE
    SCS_SetTrapPriority(group_no, input_no, priority);
#endif
}


#ifndef EXTERNAL_DISPATCH_TABLE
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
void NVIC_Isr_L (void)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Retrieve ISR vector number                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    UINT16 vector = SCS_GetActiveVectorNumber();

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear the pending interrupt                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    NVIC_ClearInt((NVIC_INT_SRC_T)(vector - NVIC_TRAP_NUM));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Invoke appropriate vector                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    (NVIC_SW_DispatchTable[vector])(vector - NVIC_TRAP_NUM);
}
#endif


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_PrintRegs                                                                         */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_PrintRegs (void)
{
    UINT i;

    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     NVIC     */\n");
    HAL_PRINT("/*--------------*/\n\n");

    for (i = 0; i < NVIC_NUM_OF_INTERRUPT_GROUPS; i++)
    {
        HAL_PRINT("ISER%d%*s=%#010lx ", i, (UINT)(i<10), "", REG_READ(NVIC_ISER(i)));
    }
    HAL_PRINT("\n");

    for (i = 0; i < NVIC_NUM_OF_INTERRUPT_GROUPS; i++)
    {
        HAL_PRINT("ICER%d%*s=%#010lx ", i, (UINT)(i<10), "", REG_READ(NVIC_ICER(i)));
    }
    HAL_PRINT("\n");

    for (i = 0; i < NVIC_NUM_OF_INTERRUPT_GROUPS; i++)
    {
        HAL_PRINT("ISPR%d%*s=%#010lx ", i, (UINT)(i<10), "", REG_READ(NVIC_ISPR(i)));
    }
    HAL_PRINT("\n");

    for (i = 0; i < NVIC_NUM_OF_INTERRUPT_GROUPS; i++)
    {
        HAL_PRINT("ICPR%d%*s=%#010lx ", i, (UINT)(i<10), "", REG_READ(NVIC_ICPR(i)));
    }
    HAL_PRINT("\n");

    for (i = 0; i < NVIC_NUM_OF_INTERRUPT_GROUPS; i++)
    {
        HAL_PRINT("IABR%d%*s=%#010lx ", i, (UINT)(i<10), "", REG_READ(NVIC_IABR(i)));
    }
    HAL_PRINT("\n");

    for (i = 0; i < NVIC_NUM_OF_INTERRUPT_GROUPS; i++)
    {
        HAL_PRINT("IPR%d%*s=%#010lx ", i, (UINT)(i<10), "", REG_READ(NVIC_IPR(i)));
    }

	HAL_PRINT("\n");
	
	for(int i = 0; i < NVIC_NUM_OF_INTERRUPT_GROUPS; i++)
	{
		UINT32 reg =  REG_READ(NVIC_IABR(i));
		for (int j = 0; j < 32 ; j++)
		{
			if (reg & (1L << j))
			{
				HAL_PRINT("NVIC Int %d is active\n", i*32 + j);
			}
				
		}
	}

    
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        NVIC_PrintVersion                                                                      */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_PrintVersion (void)
{
    HAL_PRINT("NVIC     = %X\n", MODULE_VERSION(NVIC_MODULE_TYPE));
}


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#ifndef EXTERNAL_DISPATCH_TABLE
/*---------------------------------------------------------------------------------------------------------*/
/* TRAP Routing functions                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_RES0_L,       NVIC_TRAP_INIT_SP)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_RST_L,        NVIC_TRAP_RESET)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_NMI_L,        NVIC_TRAP_NMI)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_HDF_L,        NVIC_TRAP_HARD_FAULT)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_MMNG_L,       NVIC_TRAP_MEM_MANAGE)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_BSF_L,        NVIC_TRAP_BUS_FAULT)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_USGF_L,       NVIC_TRAP_USG_FAULT)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_RES7_L,       NVIC_TRAP_RES7)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_RES8_L,       NVIC_TRAP_RES8)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_RES9_L,       NVIC_TRAP_RES9)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_RES10_L,      NVIC_TRAP_RES10)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_DBG_L,        NVIC_TRAP_DBG_MON)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_RES13_L,      NVIC_TRAP_RES13)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_SYST_L,       NVIC_TRAP_SYST)

#if defined __PRODUCT_THREADX__ || defined EL_PRODUCT_THREADX
/*---------------------------------------------------------------------------------------------------------*/
/* The ThreadX scheduler uses the SVC/PEND_SV system event for task switch; this function never returns    */
/*---------------------------------------------------------------------------------------------------------*/
TRAP_ROUTER_FUNCTION_NO_RET(NVIC_TrapRouting_SVC_L, NVIC_TRAP_SVC)

TRAP_ROUTER_FUNCTION_NO_RET(NVIC_TrapRouting_PSV_L, NVIC_TRAP_PEND_SV)
#else
TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_SVC_L,        NVIC_TRAP_SVC)

TRAP_ROUTER_FUNCTION(NVIC_TrapRouting_PSV_L,        NVIC_TRAP_PEND_SV)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* DEFAULT HW HANDLER                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
_ISR_ void NVIC_Default_HW_Handler_L (void)
{
    ASSERT(FALSE);
}

/*---------------------------------------------------------------------------------------------------------*/
/* DEFAULT SW HANDLER                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void NVIC_Default_SW_Handler_L (_UNUSED_ UINT16 int_num)
{
    ASSERT(FALSE);
}
#endif

