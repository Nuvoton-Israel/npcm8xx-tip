/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2013-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   scs_drv.c                                                                                             */
/*            This file contains The System Control Space (SCS) driver implementation                      */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_DRV()

#include "scs_drv.h"
#include "scs_regs.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Module Dependencies                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#if defined NVIC_MODULE_TYPE
#include __MODULE_IF_HEADER_FROM_DRV(nvic)
#endif

#define SCS_MSG_DEBUG(fmt,args...)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                  LOCAL FUNCTIONS FORWARD DECLARATIONS                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

static void     SCS_CPControlAccess (CP_NUM_T cpNum, CP_ACCESS_TYPE_T accType);


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
void SCS_Init (void)
{
    UINT32 tmp;

    REG_WRITE(SCS_SHCSR, 0);

    /* Enable all traps */
    REG_WRITE(SCS_SHCSR,
        BUILD_FIELD_VAL(SCS_SHCSR_MEMFAULTACT, 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_BUSFAULTACT   , 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_USGFAULTACT   , 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_SVCALLACT     , 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_MONITORACT    , 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_PENDSVACT     , 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_SYSTICKACT    , 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_USGFAULTPENDED, 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_MEMFAULTPENDED, 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_BUSFAULTPENDED, 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_SVCALLPENDED  , 0) |
        BUILD_FIELD_VAL(SCS_SHCSR_MEMFAULTENA   , 1) |
        BUILD_FIELD_VAL(SCS_SHCSR_BUSFAULTENA   , 1) |
        BUILD_FIELD_VAL(SCS_SHCSR_USGFAULTENA   , 1) );

    REG_WRITE(SCS_ICSR,
        BUILD_FIELD_VAL(SCS_ICSR_VECTPENDING, 0) |
        BUILD_FIELD_VAL(SCS_ICSR_ISRPENDING , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_ISRPREEMPT , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_PENDSTCLR  , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_PENDSTSET  , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_PENDSVCLR  , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_PENDSVSET  , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_NMIPENDSET , 0) );

    REG_WRITE(SCS_ICSR,
        BUILD_FIELD_VAL(SCS_ICSR_VECTPENDING, 0) |
        BUILD_FIELD_VAL(SCS_ICSR_ISRPENDING , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_ISRPREEMPT , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_PENDSTCLR  , 1) |
        BUILD_FIELD_VAL(SCS_ICSR_PENDSTSET  , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_PENDSVCLR  , 1) |
        BUILD_FIELD_VAL(SCS_ICSR_PENDSVSET  , 0) |
        BUILD_FIELD_VAL(SCS_ICSR_NMIPENDSET , 0) );


    tmp = REG_READ(SCS_AIRCR);

    REG_WRITE(SCS_AIRCR,  tmp |
        BUILD_FIELD_VAL(SCS_AIRCR_VECTKEY, 0x05FA) |
        BUILD_FIELD_VAL(SCS_AIRCR_VECTCLRACTIVE , 1) );
}

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
UINT16 SCS_GetActiveVectorNumber (void)
{
    return READ_REG_FIELD(SCS_ICSR, SCS_ICSR_VECTACTIVE);
}

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
UINT16 SCS_GetPendingVectorNumber (void)
{
    return READ_REG_FIELD(SCS_ICSR, SCS_ICSR_VECTPENDING);
}

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
void SCS_SetVectorTable (UINT32 table)
{

    SCS_MSG_DEBUG("SCS table at %#010lx \n", table);

    SET_REG_FIELD(SCS_VTOR, SCS_VTOR_TBLOFF, READ_VAR_FIELD(table, SCS_VTOR_TBLOFF));
}

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
void SCS_ClearPendingSysTickInt (void)
{
    UINT32 icsr;

    icsr = REG_READ(SCS_ICSR);
    SET_VAR_FIELD(icsr, SCS_ICSR_PENDSVCLR, FALSE);
    SET_VAR_FIELD(icsr, SCS_ICSR_PENDSTCLR, TRUE);
    REG_WRITE(SCS_ICSR, icsr);
}

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
void SCS_SetTrapPriority (UINT8 group_no, UINT8 input_no, UINT8 priority)
{
    SET_REG_FIELD(SCS_SHPR(group_no), SCS_SHPR_PRI_N(input_no), priority);
}

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
void SCS_FPLazyContextSave (BOOLEAN enable)
{
    SET_REG_FIELD(SCS_FPCCR, SCS_FPCCR_LSPEN, enable);
}

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
void SCS_FPStatePreservation (BOOLEAN enable)
{
    SET_REG_FIELD(SCS_FPCCR, SCS_FPCCR_ASPEN, enable);
}

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
void SCS_FPEnableAccess (BOOLEAN enable)
{
    CP_ACCESS_TYPE_T accType = enable ? CP_FULL_ACCESS : CP_ACCESS_DENIED;

    SCS_CPControlAccess(CP10, accType);
    SCS_CPControlAccess(CP11, accType);
}

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
void SCS_TraceEnable (BOOLEAN enable)
{
    SET_REG_FIELD(SCS_DEMCR, SCS_DEMCR_TRCENA, enable);
}

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
void SCS_DivideByZeroEnable (BOOLEAN enable)
{
    SET_REG_FIELD(SCS_CCR, SCS_CCR_DIV_0_TRP, enable);
}

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
void SCS_EnableTrap (NVIC_TRAP_SRC_T trap, BOOLEAN enable)
{
    switch (trap)
    {
    case NVIC_TRAP_MEM_MANAGE:
        SET_REG_FIELD(SCS_SHCSR, SCS_SHCSR_MEMFAULTENA, enable);
        break;

    case NVIC_TRAP_BUS_FAULT:
        SET_REG_FIELD(SCS_SHCSR, SCS_SHCSR_BUSFAULTENA, enable);
        break;

    case NVIC_TRAP_USG_FAULT:
        SET_REG_FIELD(SCS_SHCSR, SCS_SHCSR_USGFAULTENA, enable);
        break;

    default:
        break;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_PrintRegs (void)
{
    UINT i;

    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     SCS      */\n");
    HAL_PRINT("/*--------------*/\n\n");

    HAL_PRINT("CPUID       = %#010lx\n", REG_READ(SCS_CPUID));
    HAL_PRINT("ICSR        = %#010lx\n", REG_READ(SCS_ICSR));
    HAL_PRINT("VTOR        = %#010lx\n", REG_READ(SCS_VTOR));
    HAL_PRINT("AIRCR       = %#010lx\n", REG_READ(SCS_AIRCR));
    HAL_PRINT("SCR         = %#010lx\n", REG_READ(SCS_SCR));
    HAL_PRINT("CCR         = %#010lx\n", REG_READ(SCS_CCR));
    HAL_PRINT("SHPR1       = %#010lx\n", REG_READ(SCS_SHPR(0)));
    HAL_PRINT("SHPR2       = %#010lx\n", REG_READ(SCS_SHPR(1)));
    HAL_PRINT("SHPR3       = %#010lx\n", REG_READ(SCS_SHPR(2)));
    HAL_PRINT("SHCSR       = %#010lx\n", REG_READ(SCS_SHCSR));
    HAL_PRINT("CFSR        = %#010lx  (Configurable Fault Status Register)\n", REG_READ(SCS_CFSR));
    HAL_PRINT("HFSR        = %#010lx  (HardFault Status Register)\n", REG_READ(SCS_HFSR));
    HAL_PRINT("UFSR        = %#010lx  (UsageFault Status Register)\n", REG_READ(SCS_UFSR));
    HAL_PRINT("DFSR        = %#010lx\n", REG_READ(SCS_DFSR));
    HAL_PRINT("MMFSR       = %#010lx  (MemManage Fault Status Register)\n", REG_READ(SCS_MMFSR));
    HAL_PRINT("MMFAR       = %#010lx  (MemManage Fault Address Register)\n", REG_READ(SCS_MMFAR));
    HAL_PRINT("BFAR        = %#010lx  (BusFault Address Register)\n", REG_READ(SCS_BFAR));
    HAL_PRINT("BFSR        = %#010lx  (BusFault Status  Register)\n", REG_READ(SCS_BFSR));
    HAL_PRINT("AFSR        = %#010lx\n", REG_READ(SCS_AFSR));
    HAL_PRINT("CPACR       = %#010lx\n", REG_READ(SCS_CPACR));
    HAL_PRINT("DEMCR       = %#010lx\n", REG_READ(SCS_DEMCR));
    HAL_PRINT("FPCCR       = %#010lx\n", REG_READ(SCS_FPCCR));
    HAL_PRINT("FPCAR       = %#010lx\n", REG_READ(SCS_FPCAR));
    HAL_PRINT("FPDSCR      = %#010lx\n", REG_READ(SCS_FPDSCR));
    HAL_PRINT("MVFR0       = %#010lx\n", REG_READ(SCS_MVFR0));
    HAL_PRINT("MVFR1       = %#010lx\n", REG_READ(SCS_MVFR1));
    HAL_PRINT("ICTR        = %#010lx\n", REG_READ(SCS_ICTR));
    HAL_PRINT("ACTLR       = %#010lx\n", REG_READ(SCS_ACTLR));

    for (i = 0; i < SCS_PID_NUM; i++)
    {
        HAL_PRINT("PID%d=%#010lx ", i, REG_READ(SCS_PID(i)));
    }
    HAL_PRINT("\n");

    for (i = 0; i < SCS_CID_NUM; i++)
    {
        HAL_PRINT("CID%d=%#010lx ", i, REG_READ(SCS_CID(i)));
    }
    HAL_PRINT("\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_PrintVersion                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void SCS_PrintVersion (void)
{
    HAL_PRINT("SCS         = %X\n", MODULE_VERSION(SCS_MODULE_TYPE));
}


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SCS_CPControlAccess                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  cpNum   - Coprocessor number.                                                          */
/*                  accType - Access Privileges for ARM's Coprocessor.                                     */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine controls the access privileges for a given coprocessor.                   */
/*---------------------------------------------------------------------------------------------------------*/
static void SCS_CPControlAccess (CP_NUM_T cpNum, CP_ACCESS_TYPE_T accType)
{
    SET_REG_FIELD(SCS_CPACR, SCS_CPACR_CP(cpNum), accType);
}
