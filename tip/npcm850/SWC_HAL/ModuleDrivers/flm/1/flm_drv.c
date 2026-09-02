/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2020 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   flm_drv.c                                                                                             */
/*            This file contains FLM module driver implementation                                          */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_DRV()

#include "flm_regs.h"
#include "flm_drv.h"


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               DEFINITIONS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define LOG_MAX_INDEX               1
#define RANGE_MAX_INDEX             7
#define CMD_MAX_INDEX               31
#define CMB_MAX_INDEX               31
#define TCR_MAX_INDEX               7
#define TCRA_MAX_INDEX              3
#define TCC_MAX_INDEX               7
#define TCCA_MAX_INDEX              3
#define QUAL_MAX_INDEX              3
#define CLEAR_MASK_MAX              FLM_INTER_ALL
#define INT_MASK_MAX                FLM_INTER_ALL
#define FLM_ENABLE_TIMEOUT          0xFFFFFFFF
#define FLM_LOCKED_VALUE            0x28 // random value, different from 0xA9
#define FLM_NOT_LOCKED_VALUE        0xA9
#define FLM_ENABLE(module, enable)  SET_REG_FIELD(FLM_CTL(module),  FLM_CTL_MEN, enable)
#define FLM_ENABLED(module)         (READ_REG_FIELD(FLM_CTL(module), FLM_CTL_MEN) == 0x01)
#define FLM_LOCKED(module)          (READ_REG_FIELD(FLM_CTL(module), FLM_CTL_RLCK) != FLM_NOT_LOCKED_VALUE)
#define WAIT_TO_READY(module)       BUSY_WAIT_TIMEOUT((READ_REG_FIELD(FLM_CTL(module), FLM_CTL_RDY) == 0), FLM_ENABLE_TIMEOUT)
#ifndef FLM_SELECT
#define FLM_SELECT(enable)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 MACROS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define FLM_BEFORE_DYNAMIC_PARAM_UPDATE(n)                  \
{                                                           \
    FLM_ReversibleLock(n, FALSE);                           \
    WAIT_TO_READY(n);                                       \
}

#define FLM_AFTER_DYNAMIC_PARAM_UPDATE(n)                   \
{                                                           \
    SET_REG_FIELD(FLM_CTL((UINT)n), FLM_CTL_CHANGE, 1);     \
    WAIT_TO_READY(n);                                       \
}

#define FLM_DYNAMIC_PARAM_UPDATE(n, action)                 \
{                                                           \
    if (FLM_ENABLED(n))                                     \
    {                                                       \
        FLM_BEFORE_DYNAMIC_PARAM_UPDATE(n);                 \
        action;                                             \
        FLM_AFTER_DYNAMIC_PARAM_UPDATE(n);                  \
    }                                                       \
    else                                                    \
    {                                                       \
        FLM_STATIC_PARAM_UPDATE(n, action);                 \
    }                                                       \
}

#define FLM_STATIC_PARAM_UPDATE(n, action)                  \
{                                                           \
    if (FLM_ENABLED(n))                                     \
    {                                                       \
        FLM_ENABLE(n, FALSE);                               \
        action;                                             \
        FLM_ENABLE(n, TRUE);                                \
    }                                                       \
    else                                                    \
    {                                                       \
        action;                                             \
    }                                                       \
}


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            GLOBAL VARIABLES                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

static FLM_CALLBACK_T FLM_callback[FLM_NUM_OF_MODULES];


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                       LOCAL FUNCTIONS DECLARATION                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
void    FLM_IntHandler  (UINT16 int_num);


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_Init                                                                               */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function initiates FLM driver global variables.                                   */
/*---------------------------------------------------------------------------------------------------------*/
void FLM_Init (void)
{
    UINT i;

    for (i = 0; i < FLM_NUM_OF_MODULES; i++)
    {
        FLM_callback[i] = NULL;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_Config                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number.                                                            */
/*                  mode   - Select Abort or Monitoring mode pin muxing                                    */
/*                           * If mode = FLM_MODE_ABORT The FLM will block (abort) unauthorized transaction*/
/*                             In Abort mode both FLM_CSI# and FLM_CSIO# signals are selected to device    */
/*                             pins, FLM_CSI# as an input and FLM_CSIO# as an output.                      */
/*                           * If mode = FLM_MODE_MONITORING - The FLM only monitor and does not block     */
/*                             transaction. In Monitoring mode only FLM_CSIO# signal is selected to the    */
/*                             device pin, this time as an input.                                          */
/*                  quadEn - When TRUE, selecting the FLM Quad signals to the device pins                  */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures an FLM module by selecting FLM inputs interface on             */
/*                  multiplexed pins.                                                                      */
/*lint -e{715}      Suppress 'module' not referenced                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void FLM_Config (FLM_MODULE_T module, FLM_MODE_T mode, BOOLEAN quadEn)
{
    FLM_MUX(mode, quadEn);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_InitModule                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module               - FLM module number.                                              */
/*                  flashSize            - Defines the size of the monitored flash device. To ignore       */
/*                                         the flash size use FLM_DEV_SIZE_IGNORE                          */
/*                  cmdcheckingMode      - FLM_CMD_CHECK_ALL_SPI_SIGNALS_DURING_CMD_BYTE - use all SPI     */
/*                                         inputs signals during Command Byte checking.                    */
/*                                         FLM_CMD_CHECK_ONLY_DATA_IN0_DURING_CMD_BYTE - Disables checking */
/*                                         D1 to D3 signals (and only use D0) during Command Byte checking */
/*                  bypassAbort          - When True the "Abort" mode is bypassed. Meaning that the FLM    */
/*                                         will not stop illegal transaction even if the HW is configured  */
/*                                         (in terms of pin muxing) to "Abort" mode.                       */
/*                                         BypassAbortMode is ignored in "Monitoring" mode                 */
/*                  callback             - function to be called on an FLM interrupt                       */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function handles the FLM configuration.                                           */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_InitModule (
    FLM_MODULE_T    module,
    FLM_DEV_SIZE_T  flashSize,
    FLM_CMD_CHECK_T cmdcheckingMode,
    BOOLEAN         bypassAbort,
    FLM_CALLBACK_T  callback
)
{
    UINT32 configReg;

    ASSERT(module < FLM_NUM_OF_MODULES);

    FLM_callback[module] = callback;

    configReg = REG_READ(FLM_CFG(module));

    /* --------------------------------------------------------------------------------------------------- */
    /* Config flash size                                                                                   */
    /* --------------------------------------------------------------------------------------------------- */
    if (flashSize == FLM_DEV_SIZE_IGNORE)
    {
        SET_VAR_FIELD(configReg, FLM_CFG_ALIAS, FALSE);
    }
    else
    {
        SET_VAR_FIELD(configReg, FLM_CFG_DEVSIZ, flashSize);
        SET_VAR_FIELD(configReg, FLM_CFG_ALIAS, TRUE);
    }

    SET_VAR_FIELD(configReg, FLM_CFG_EBCHKDIS, cmdcheckingMode);
    SET_VAR_FIELD(configReg, FLM_CFG_CSOBYP,   bypassAbort);

    FLM_STATIC_PARAM_UPDATE(module, REG_WRITE(FLM_CFG(module), configReg));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Interrupt configuration                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_REGISTER_AND_ENABLE(FLM_INTERRUPT_PROVIDER, FLM_INTERRUPT(module), FLM_IntHandler,
                                  FLM_INTERRUPT_POLARITY, FLM_INTERRUPT_PRIORITY);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_GetEventsStatus                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number.                                                            */
/*                  status - a pointer to store the status information                                     */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function fills the given status pointer with the status information.              */
/*                  Note!!: Should only be used when FLM interrupts are disabled. When interrupts are      */
/*                          enabled the status is passed to the callback function and automatically        */
/*                          cleared by the handler.                                                        */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_GetEventsStatus (FLM_MODULE_T module, FLM_STATUS_T* status)
{
    UINT32 flm_stat;

    ASSERT(module < FLM_NUM_OF_MODULES);

    flm_stat                        = REG_READ(FLM_STAT(module));
    status->rejectionEvent          = READ_VAR_FIELD(flm_stat, FLM_STAT_RJ_EV);
    status->chipSelectLowEvent      = READ_VAR_FIELD(flm_stat, FLM_STAT_CSI_EV);
    status->transactionCounterEvent = READ_VAR_FIELD(flm_stat, FLM_STAT_TCR_EV);
    status->rejectionType1          = READ_VAR_FIELD(flm_stat, FLM_STAT_RJ_TP1);
    status->rejectionType2          = READ_VAR_FIELD(flm_stat, FLM_STAT_RJ_TP2);
    status->rejectionNumber         = READ_VAR_FIELD(flm_stat, FLM_STAT_RJ_NO);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_GetRejectionInfo                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number.                                                            */
/*                  logNum - 0 for the first rejected transaction, 1 for the second.                       */
/*                  rej    - a pointer to store the rejected transaction information                       */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function fills the given rej pointer with the rejected transaction information.   */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_GetRejectionInfo (
    FLM_MODULE_T    module,
    UINT8           logNum,
    FLM_REJECT_T*   rej
)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    DEFS_STATUS_COND_CHECK((logNum <= LOG_MAX_INDEX), DEFS_STATUS_INVALID_PARAMETER);

    rej->rejectionAddress = READ_REG_FIELD(FLM_LOG(logNum, module), FLM_LOG_RJ_ADDR);
    rej->rejectionCommand = READ_REG_FIELD(FLM_LOG(logNum, module), FLM_LOG_RJ_CMD);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_ClearEventsStatus                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module    - FLM module number.                                                         */
/*                  clearMask - a mask which determines the events to be cleared                           */
/*                              when bit 0 is set, clears all Rejection Event related info                 */
/*                              when bit 1 is set, clears the Chip-Select Active Low Event                 */
/*                              when bit 3 is set, clears the Transaction Counter Event.                   */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function clears the status fields according to the given mask.                    */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_ClearEventsStatus (FLM_MODULE_T module, UINT32 clearMask)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    DEFS_STATUS_COND_CHECK(((clearMask | CLEAR_MASK_MAX) == CLEAR_MASK_MAX), DEFS_STATUS_INVALID_PARAMETER);

    REG_WRITE(FLM_STAT(module), clearMask);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_InterruptEnable                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module  - FLM module number.                                                           */
/*                  intMask - a mask which determines which interrupts to enable/disable                   */
/*                            when bit 0 is set, Enables the Rejection Event Interrupt,                    */
/*                            when bit 1 is set, Enables the Chip-Select Active Low Interrupt,             */
/*                            when bit 3 is set, Enables the Transaction Counter Event Interrupt           */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function enables/disables the interrupts (and wakeup) according to the given mask */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_InterruptEnable (FLM_MODULE_T module, UINT32 intMask)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    DEFS_STATUS_COND_CHECK(((intMask | INT_MASK_MAX) == INT_MASK_MAX), DEFS_STATUS_INVALID_PARAMETER);

    REG_WRITE(FLM_IE(module), intMask);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_ConfigRange                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module     - FLM module number.                                                        */
/*                  rangeIndex - The index of the range (0 - 7)                                            */
/*                  startValue - Range start address (must be 4K aligned)                                  */
/*                  endValue   - Range end address (must be 4K aligned)                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function configures the FLM range.                                                */
/*                  the configured range will be from startValue till (endValue-1) for example:            */
/*                  startValue = 0 ,  endValue = 0x2000(8K) , Range = 0 to 0x1FFF                          */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_ConfigRange (
    FLM_MODULE_T    module,
    UINT8           rangeIndex,
    UINT32          startValue,
    UINT32          endValue
)
{
    UINT32 configRangeReg;
    UINT16 startAddress;
    UINT16 endAddress;

    ASSERT(module < FLM_NUM_OF_MODULES);

    DEFS_STATUS_COND_CHECK((rangeIndex <= RANGE_MAX_INDEX), DEFS_STATUS_INVALID_PARAMETER);

    startAddress    = (UINT16)(startValue >> 12);
    endAddress      = (UINT16)((endValue >> 12) - 1);

    configRangeReg = REG_READ(FLM_RANG(rangeIndex, module));

    SET_VAR_FIELD(configRangeReg, FLM_RANG_STRTRANGm, startAddress);
    SET_VAR_FIELD(configRangeReg, FLM_RANG_LASTRANGm, endAddress);

    FLM_DYNAMIC_PARAM_UPDATE(module, REG_WRITE(FLM_RANG(rangeIndex, module), configRangeReg));

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_ConfigCommand                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number                                                             */
/*                  index  - The index of the command (0 - 31)                                             */
/*                  cmd    - a pointer to a command structure                                              */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Add cmd to the list of supported commands. Note that the functions does not enable     */
/*                  the command. To enable/disable commands call FLM_CommandsEnable()                      */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_ConfigCommand (
    FLM_MODULE_T            module,
    UINT8                   index,
    const FLM_COMMAND_T*    cmd
)
{
    UINT32  cmdReg = 0;
    UINT32  otherCmd;
    UINT8   i;

    ASSERT(module < FLM_NUM_OF_MODULES);

    DEFS_STATUS_COND_CHECK((index <= CMD_MAX_INDEX), DEFS_STATUS_INVALID_PARAMETER);

    SET_VAR_FIELD(cmdReg, FLM_CMD_CMD,     cmd->cmdByte);
    SET_VAR_FIELD(cmdReg, FLM_CMD_ADDSZ,   cmd->AddrSize);
    SET_VAR_FIELD(cmdReg, FLM_CMD_DATPCK,  cmd->DataBitPerClock);
    SET_VAR_FIELD(cmdReg, FLM_CMD_ADBPCK,  cmd->AddrBitPerClock);
    SET_VAR_FIELD(cmdReg, FLM_CMD_DUMBPCK, cmd->DummyBitPerClock);
    SET_VAR_FIELD(cmdReg, FLM_CMD_DUMB,    cmd->DummyBytes);
    SET_VAR_FIELD(cmdReg, FLM_CMD_CLAR,    (BOOLEAN)(cmd->AddrRangeSelect != 0));
    SET_VAR_FIELD(cmdReg, FLM_CMD_CARSEL,  cmd->AddrRangeSelect);

    for (i = 0; i <= CMD_MAX_INDEX; i++)
    {
        if (i != index)
        {
            otherCmd = REG_READ(FLM_CMD(i, module));

            if (READ_VAR_FIELD(otherCmd, FLM_CMD_CMD) == READ_VAR_FIELD(cmdReg, FLM_CMD_CMD))
            {
                /*-----------------------------------------------------------------------------------------*/
                /* If two CMD fields in two different FLM_CMDn registers have the same value, all other    */
                /* fields except CLAR, and CARSEL (to 12 bits) must be the same.                           */
                /*-----------------------------------------------------------------------------------------*/
                if ((cmdReg & 0x000FFFFF) !=  (otherCmd & 0x000FFFFF))
                {
                    return DEFS_STATUS_FAIL;
                }
            }
        }
    }

    FLM_DYNAMIC_PARAM_UPDATE(module, REG_WRITE(FLM_CMD(index, module), cmdReg));

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_ConfigByteCommand                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module  - FLM module number                                                            */
/*                  index   - The index of the "Byte" command (0 - 31)                                     */
/*                  cmdByte - The value of the Command byte.                                               */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Add cmdByte to the list of supported "Byte" commands. Note that the functions does not */
/*                  enable the command. To enable/disable "Byte" commands call FLM_ByteCommandsEnable()    */
/*                  Note that "Byte" commands are not coupled to an address ranges.                        */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_ConfigByteCommand (
    FLM_MODULE_T    module,
    UINT8           index,
    UINT8           cmdByte
)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    DEFS_STATUS_COND_CHECK((index <= CMB_MAX_INDEX), DEFS_STATUS_INVALID_PARAMETER);

    FLM_STATIC_PARAM_UPDATE(module, SET_REG_FIELD(FLM_CMB(index, module), FLM_CMB_CMD, cmdByte));

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_CommandsEnable                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module    - FLM module number                                                          */
/*                  cmdEnable - a 32 bitmask that defines which commands should be enabled/disabled        */
/*                              0: Command disabled                                                        */
/*                              1: Command enabled.                                                        */
/*                              Each bit in cmdEnable is associated with a command index. bit 0 with       */
/*                              index 0, bit 1 with index 1 up to bit 31 with index 31.                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function enables/disables Commands according to 'cmdEnable'                       */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_CommandsEnable (FLM_MODULE_T module, UINT32 cmdEnable)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    FLM_DYNAMIC_PARAM_UPDATE(module, REG_WRITE(FLM_CMDEN(module), cmdEnable));

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_ByteCommandsEnable                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module    - FLM module number                                                          */
/*                  cmdEnable - a 32 bitmask that defines which "Byte" commands should be enabled/disabled */
/*                              0: Command disabled                                                        */
/*                              1: Command enabled.                                                        */
/*                              Each bit in cmdEnable is associated with a "Byte" command index.           */
/*                              bit 0 with index 0, bit 1 with index 1 up to bit 31 with index 31.         */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function enables/disables Byte Commands according to 'cmdEnable'                  */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_ByteCommandsEnable (FLM_MODULE_T module, UINT32 cmdEnable)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    FLM_DYNAMIC_PARAM_UPDATE(module, REG_WRITE(FLM_CMBEN(module), cmdEnable));

    return DEFS_STATUS_OK;
}

#ifdef FDM_MODULE_TYPE
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_CmdDataVerifyEnable                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number                                                             */
/*                  enable - Defines if the command that is defined in the corresponding FLM_CMDn register */
/*                           is used for FDM data verification.                                            */
/*                           0: Command is not used for data verification (default).                       */
/*                           1: Command is used for data verification if the corresponding                 */
/*                              CMEN bit in FLM_CMDn register is set.                                      */
/*                           as follows:                                                                   */
/*                           CDEN[0] is associated with FLM_CMD0 register                                  */
/*                           CDEN[1] is associated with FLM_CMD1 register...                               */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function enables/disables command data verification                               */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_CmdDataVerifyEnable (FLM_MODULE_T module, UINT32 enable)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    FLM_DYNAMIC_PARAM_UPDATE(module, REG_WRITE(FLM_CDVEN(module), enable));

    return DEFS_STATUS_OK;
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_ClearTranasctionInfo                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number                                                             */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Clears all the Transaction Counters and all the Valid Transactions status              */
/*                  Transaction Counters can be read by FLM_GetTranscationCounter()                        */
/*                  Valid transactions info can be read by FLM_GetValidTransactionInfo()                   */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_ClearTranasctionInfo (FLM_MODULE_T module)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    REG_WRITE(FLM_TCGC(module), 0x01);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_GetValidTransactionInfo                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module        - FLM module number.                                                     */
/*                  validCmd      - a pointer to a 32 bit mask where each bit is set when a corresponding  */
/*                                  Command (a command with the corresponding index) is identified as      */
/*                                  a valid transaction.                                                   */
/*                  validByteCmd  - Same as "validCmd" but for the "Byte Commands"                         */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function returns the information on the current Valid Command transactions        */
/*                  and Valid Byte Command transactions.                                                   */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_GetValidTransactionInfo (
    FLM_MODULE_T    module,
    UINT32*         validCmd,
    UINT32*         validByteCmd
)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    *validCmd       = REG_READ(FLM_CMDEV(module));
    *validByteCmd   = REG_READ(FLM_CMBEV(module));

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_ConfigQualifier                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number                                                             */
/*                  index  - qualifier index: 0 to 3                                                       */
/*                  qual   - a pointer to the qualifier fields                                             */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  A qualifier enables checking one byte value with a configurable index in an SPI        */
/*                  transaction. Each qualifier is associated with a group of 8 Commands.                  */
/*                  Qualifier 0 is associated with commands 0 to 7                                         */
/*                  Qualifier 1 is associated with commands 8 to 15                                        */
/*                  Qualifier 2 is associated with commands 26 to 23                                       */
/*                  Qualifier 3 is associated with commands 24 to 31                                       */
/*                  If the qualification fails, the transaction is rejected.                               */
/*                  When a qualifier is enabled, the following describes the byte qualification logic:     */
/*                  If (((SPI_BYTE[QBYTE] & QMASK ) == (QVAL & QMASK)) ‘XOR' QPOL):                        */
/*                     Qualification passed                                                                */
/*                  else                                                                                   */
/*                     Qualification failed                                                                */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_ConfigQualifier (
    FLM_MODULE_T            module,
    UINT8                   index,
    const FLM_QUALIFIER_T*  qual
)
{
    UINT32 qualReg;

    ASSERT(module < FLM_NUM_OF_MODULES);

    DEFS_STATUS_COND_CHECK((index <= QUAL_MAX_INDEX), DEFS_STATUS_INVALID_PARAMETER);

    qualReg = REG_READ(FLM_CQ(index, module));

    SET_VAR_FIELD(qualReg, FLM_CQ_QMASK, qual->qualMask);
    SET_VAR_FIELD(qualReg, FLM_CQ_QVAL,  qual->qualVal);
    SET_VAR_FIELD(qualReg, FLM_CQ_QBYTE, qual->qualByte);
    SET_VAR_FIELD(qualReg, FLM_CQ_QPOL,  qual->qualPol);
    SET_VAR_FIELD(qualReg, FLM_CQ_QEN,   qual->qualEn);

    FLM_STATIC_PARAM_UPDATE(module, REG_WRITE(FLM_CQ(index, module), qualReg));

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_GetTranscationCounter                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module            - FLM module number                                                  */
/*                  index             - transaction counters (TCR) index: 0 to 7 (see more info in         */
/*                                      the explanation of the FLM_ConfigTranscationCounter() function)    */
/*                  transactionCount  - a pointer to store the transaction counter value                   */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  The FLM has 8 SPI transaction counters (TCR) for counting valid transactions           */
/*                  On SPI transaction termination, the relevant TCR counter is incremented by one,        */
/*                  if the transaction (associated with the counter) is recognized as valid Command or     */
/*                  Byte Command.                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_GetTranscationCounter (
    FLM_MODULE_T    module,
    UINT8           index,
    UINT32*         transactionCount
)
{
    UINT32          firstReadVal, secondReadVal, thirdReadVal;
    UINT32          firstDif        = 0;
    UINT32          secondDif       = 0;
    BOOLEAN         counterValReady = FALSE;
    UINT volatile   savedInt        = 0;

    ASSERT(module < FLM_NUM_OF_MODULES);

    DEFS_STATUS_COND_CHECK((index <= TCR_MAX_INDEX), DEFS_STATUS_INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* To read the current count from a transaction counter, perform the following:                        */
    /*  - Disable interrupts.                                                                              */
    /*  - Read 3 consecutive times the register value into three different Core registers                  */
    /*   (to ensure that this operation is "atomic").                                                      */
    /*  - Compare every two consecutive readings (i.e., 1st with 2nd and 2nd with 3rd).                    */
    /*     - If the difference is less than or equal to 2, keep the two readings, otherwise discard them.  */
    /*     - If both differences are higher than 2, exit, enable interrupts and retry.                     */
    /*  - From the pair with the lowest difference, select the reading with the higher value.              */
    /*  - Enable interrupts.                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    do
    {
        INTERRUPTS_SAVE_DISABLE(savedInt);

        firstReadVal  = REG_READ(FLM_TCR(index, module));
        secondReadVal = REG_READ(FLM_TCR(index, module));
        thirdReadVal  = REG_READ(FLM_TCR(index, module));

        firstDif  = (secondReadVal > firstReadVal) ? (secondReadVal - firstReadVal) : (firstReadVal - secondReadVal);
        secondDif = (thirdReadVal > secondReadVal) ? (thirdReadVal - secondReadVal) : (secondReadVal - thirdReadVal);

        if (firstDif <= 2 || secondDif <= 2)
        {
            *transactionCount   = (firstDif < secondDif) ? secondReadVal : thirdReadVal;
            counterValReady     = TRUE;
        }

        INTERRUPTS_RESTORE(savedInt);

    } while (!counterValReady);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_ConfigTranscationCounter                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module         - FLM module number                                                     */
/*                  index          - transaction counters (TCR) index 0 to 7 (see more below)              */
/*                  commandEn      - Controls the association between a TCR and a Command                  */
/*                                   Each bit in this field relates to a Command                           */
/*                                   When a bit in this field is set the counter counts Valid transactions */
/*                                   identified by the corresponding Command .                             */
/*                                   The mapping between a bit in commandEn and a Command is:              */
/*                                   For TCR index j, Bit k : FLM_CMD{(j mod 4)*8+k}                       */
/*                                   where: 0<=j<=7, 0<=k<=7                                               */
/*                  byteCommandEn  - same as commandEn, but for "Byte" Commands.                           */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:     This function configures the FLM transaction counters.                                 */
/*                  The FLM has 8 SPI transaction counters (TCR) for counting valid transactions           */
/*                  The counters are separated to 4 groups in the following way                            */
/*                  TCR 0,4 are associated with Commands 0 to 7   and Byte Commands 0 to 7                 */
/*                  TCR 1,5 are associated with Commands 8 to 15  and Byte Commands 8 to 15                */
/*                  TCR 2,6 are associated with Commands 16 to 23 and Byte Commands 16 to 23               */
/*                  TCR 3,7 are associated with Commands 24 to 31 and Byte Commands 24 to 32               */
/*                  To enable a TCR to count a the required commands (or Byte commands) set the            */
/*                  appropriate bit in commandEn and/or byteCommandEn. Example:                            */
/*                  - Use TCR0 to count Commands 0 to 3 : FLM_ConfigTranscationCounter(0, 0, 0x0F, 0x00)   */
/*                  - Use TCR4 to count Commands 4 to 7 : FLM_ConfigTranscationCounter(0, 4, 0xF0, 0x00)   */
/*                  - Use TCR1 to count Command 8 and                                                      */
/*                    Byte Command 9                    : FLM_ConfigTranscationCounter(0, 1, 0x01, 0x02)   */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_ConfigTranscationCounter (
    FLM_MODULE_T    module,
    UINT8           index,
    UINT8           cmdCountEnByte,
    UINT8           cmbCountEnByte
)
{
    UINT32  tccReg;

    ASSERT(module < FLM_NUM_OF_MODULES);

    DEFS_STATUS_COND_CHECK((index <= TCC_MAX_INDEX), DEFS_STATUS_INVALID_PARAMETER);

    tccReg = REG_READ(FLM_TCC(index, module));

    SET_VAR_FIELD(tccReg, FLM_TCC_CMDCE, cmdCountEnByte);
    SET_VAR_FIELD(tccReg, FLM_TCC_CMBCE, cmbCountEnByte);

    FLM_DYNAMIC_PARAM_UPDATE(module, REG_WRITE(FLM_TCC(index, module), tccReg));

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_Enable                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number                                                             */
/*                  enable - If TRUE enables the module, if FALSE disables it                              */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function enables/disables the module                                              */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_Enable (FLM_MODULE_T module, BOOLEAN enable)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    FLM_ENABLE(module, enable);

    FLM_SELECT(enable);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_Enabled                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number                                                             */
/*                                                                                                         */
/* Returns:         TRUE in case FLM module is enabled; FALSE otherwise                                    */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the FLM module enable state                                       */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN FLM_Enabled (FLM_MODULE_T module)
{
    return FLM_ENABLED(module);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_PermanentLock                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number                                                             */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function locks the module (write to regs is ignored),                             */
/* Note:            this operation is irreversible                                                         */
/*lint -e{648}      Suppress Overflow in computing constant for operation: 'shift left'                    */
/*---------------------------------------------------------------------------------------------------------*/
void FLM_PermanentLock (FLM_MODULE_T module)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    SET_REG_FIELD(FLM_CTL(module), FLM_CTL_LCK, 0x01);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_ReversibleLock                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - FLM module number                                                             */
/*                  lock   - when TRUE, locks the module, when FALSE unlocks the module                    */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function locks the FLM, please note: this operation is reversible                 */
/*---------------------------------------------------------------------------------------------------------*/
void FLM_ReversibleLock (FLM_MODULE_T module, BOOLEAN lock)
{
    ASSERT(module < FLM_NUM_OF_MODULES);

    SET_REG_FIELD(FLM_CTL(module), FLM_CTL_RLCK, (lock ? FLM_LOCKED_VALUE : FLM_NOT_LOCKED_VALUE));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void FLM_PrintRegs (void)
{
    UINT i;

    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     FLM      */\n");
    HAL_PRINT("/*--------------*/\n\n");

    for (i = 0; i < FLM_NUM_OF_MODULES; i++)
    {
        FLM_PrintModuleRegs((FLM_MODULE_T)i);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_PrintModuleRegs                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - The module to be printed.                                                 */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module instance registers                                      */
/*lint -e{715}      Suppress 'module_num' not referenced                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void FLM_PrintModuleRegs (FLM_MODULE_T module_num)
{
    UINT8 i;

    ASSERT(module_num < FLM_NUM_OF_MODULES);

    HAL_PRINT("FLM%d:\n", (module_num+1));
    HAL_PRINT("--------\n");
    HAL_PRINT("FLM_CFG             = 0x%08X\n", REG_READ(FLM_CFG(module_num)));
    HAL_PRINT("FLM_STAT            = 0x%08X\n", REG_READ(FLM_STAT(module_num)));

    for (i = 0; i <= LOG_MAX_INDEX; i++)
    {
        HAL_PRINT("FLM_LOG%d            = 0x%08X\n", i+1, REG_READ(FLM_LOG(i, module_num)));
    }

    HAL_PRINT("FLM_IE              = 0x%08X\n", REG_READ(FLM_IE(module_num)));
    HAL_PRINT("FLM_CTL             = 0x%08X\n", REG_READ(FLM_CTL(module_num)));

    for (i = 0; i <= RANGE_MAX_INDEX; i++)
    {
        HAL_PRINT("FLM_RANG%d           = 0x%08X\n", i+1, REG_READ(FLM_RANG(i, module_num)));
    }

    HAL_PRINT("FLM_CMDEN           = 0x%08X\n", REG_READ(FLM_CMDEN(module_num)));
    HAL_PRINT("FLM_CMBEN           = 0x%08X\n", REG_READ(FLM_CMBEN(module_num)));
    HAL_PRINT("FLM_CDVEN           = 0x%08X\n", REG_READ(FLM_CDVEN(module_num)));

    for (i = 0; i <= CMD_MAX_INDEX; i++)
    {
        HAL_PRINT("FLM_CMD%d%*s           = 0x%08X\n", i+1, (UINT)((i+1)<10), "", REG_READ(FLM_CMD(i, module_num)));
    }

    for (i = 0; i <= TCR_MAX_INDEX; i++)
    {
        if (i <= TCRA_MAX_INDEX)
        {
            HAL_PRINT("FLM_TCRA%d           = 0x%08X\n", i+1, REG_READ(FLM_TCR(i, module_num)));
        }
        else
        {
            HAL_PRINT("FLM_TCRB%d           = 0x%08X\n", i+1, REG_READ(FLM_TCR(i, module_num)));
        }
    }

    for (i = 0; i <= TCC_MAX_INDEX; i++)
    {
        if (i <= TCCA_MAX_INDEX)
        {
            HAL_PRINT("FLM_TCCA%d           = 0x%08X\n", i+1, REG_READ(FLM_TCC(i, module_num)));
        }
        else
        {
            HAL_PRINT("FLM_TCCB%d           = 0x%08X\n", i+1, REG_READ(FLM_TCC(i, module_num)));
        }
    }

    HAL_PRINT("FLM_CMDEV           = 0x%08X\n", REG_READ(FLM_CMDEV(module_num)));
    HAL_PRINT("FLM_CMBEV           = 0x%08X\n", REG_READ(FLM_CMBEV(module_num)));
    HAL_PRINT("FLM_TCGC            = 0x%08X\n", REG_READ(FLM_TCGC(module_num)));

    for (i = 0; i <= QUAL_MAX_INDEX; i++)
    {
        HAL_PRINT("FLM_CQ%d             = 0x%08X\n", i+1, REG_READ(FLM_CQ(i, module_num)));
    }

    for (i = 0; i <= CMB_MAX_INDEX; i++)
    {
        HAL_PRINT("FLM_CMB%d%*s           = 0x%08X\n", i+1, (UINT)((i+1)<10), "", REG_READ(FLM_CMB(i, module_num)));
    }

    HAL_PRINT("\n");
}

#ifdef FLM_SELF_TEST
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_SelfTest                                                                           */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine test the FLM driver                                                       */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FLM_SelfTest (void)
{
    FLM_COMMAND_T const cmd     = {0x88, FLM_ADDR_SIZE_LIM_CMD_3_BYTES, FLM_BIT_PER_CLK_1, FLM_BIT_PER_CLK_1,
                                   FLM_BIT_PER_CLK_1, FLM_DUMMY_BYTES_0, 0x11};
    FLM_COMMAND_T const cmd1    = {0x88, FLM_ADDR_SIZE_LIM_CMD_3_BYTES, FLM_BIT_PER_CLK_2, FLM_BIT_PER_CLK_2,
                                   FLM_BIT_PER_CLK_2, FLM_DUMMY_BYTES_0, 0x11};
#if 0
    FLM_Init();
    FLM_Config(FLM_MODULE_0, FLM_MODE_ABORT, FALSE);
#endif

    FLM_ConfigCommand(FLM_MODULE_0, 0, &cmd);
    FLM_ConfigCommand(FLM_MODULE_0, 1, &cmd1);
    FLM_ConfigRange(FLM_MODULE_0, 0, 0xFFFF, 0xFFFF);
    FLM_ReversibleLock(FLM_MODULE_0, TRUE);

    FLM_Enable(FLM_MODULE_0, TRUE);
    FLM_InitModule(FLM_MODULE_0, FLM_DEV_SIZE_2_MB, FLM_CMD_CHECK_ONLY_DATA_INPUT0_DURING_CMD_BYTE, FALSE, NULL);

    DEFS_STATUS_RET_CHECK(FLM_InterruptEnable(0, 1));
    FLM_ByteCommandsEnable(FLM_MODULE_0, 0x00000000);
    FLM_ReversibleLock(FLM_MODULE_0, TRUE);
    FLM_Enable(FLM_MODULE_0, FALSE);

    FLM_ByteCommandsEnable(FLM_MODULE_0, 0xFFFFFFFF);
    FLM_ReversibleLock(FLM_MODULE_0, FALSE);
    FLM_Enable(FLM_MODULE_0, TRUE);
    FLM_ByteCommandsEnable(FLM_MODULE_0, 0xFFFFFFFF);

    FLM_Enable(FLM_MODULE_0, FALSE);

    return DEFS_STATUS_OK;
}
#endif //FLM_SELF_TEST


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_IntHandler                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_num - number of triggered interrupt                                                */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Interrupt handlers for FLM interrupts.                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void FLM_IntHandler (UINT16 int_num)
{
    FLM_MODULE_T module     = FLM_INTERRUPT_MODULE(int_num);
    UINT32       intEnable  = REG_READ(FLM_IE(module));
    UINT32       flmStatus  = REG_READ(FLM_STAT(module));
    UINT32       status     = intEnable & flmStatus &
                              (MASK_FIELD(FLM_IE_RJ_IE) | MASK_FIELD(FLM_IE_CSI_IE) | MASK_FIELD(FLM_IE_TCR_IE));

    if (status != 0)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Clear status                                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(FLM_STAT(module), status);

        /*-------------------------------------------------------------------------------------------------*/
        /* Call user callback                                                                              */
        /*-------------------------------------------------------------------------------------------------*/
        EXECUTE_FUNC(FLM_callback[module], (module, flmStatus));
    }
}

