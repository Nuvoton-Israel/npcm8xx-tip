/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2020 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   flm_if.h                                                                                              */
/*            This file contains FLM module interface                                                      */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef FLM_IF_H
#define FLM_IF_H

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()

#if defined FLM_MODULE_TYPE
#include __MODULE_HEADER(flm, FLM_MODULE_TYPE)
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* FLM mode                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    FLM_MODE_ABORT = 0,
    FLM_MODE_MONITORING
} FLM_MODE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM module number                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    FLM_MODULE_0  = 0,
	FLM_MODULE_1  = 1,
	FLM_MODULE_2  = 2,
	FLM_MODULE_3  = 3,
    FLM_MODULE_LAST
} FLM_MODULE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM flash device size                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    FLM_DEV_SIZE_2_MB,
    FLM_DEV_SIZE_4_MB,
    FLM_DEV_SIZE_8_MB,
    FLM_DEV_SIZE_16_MB,
    FLM_DEV_SIZE_32_MB,
    FLM_DEV_SIZE_64_MB,
    FLM_DEV_SIZE_128_MB,
    FLM_DEV_SIZE_256_MB,
    FLM_DEV_SIZE_IGNORE
} FLM_DEV_SIZE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM command address size                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    FLM_ADDR_SIZE_LIM_CMD_3_BYTES,
    FLM_ADDR_SIZE_LIM_CMD_4_BYTES
} FLM_ADDR_SIZE_LIM_CMD_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM transferred bits per clock                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    FLM_BIT_PER_CLK_1,
    FLM_BIT_PER_CLK_2,
    FLM_BIT_PER_CLK_4,
} FLM_BIT_PER_CLK_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM number of dummy bytes                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    FLM_DUMMY_BYTES_0,
    FLM_DUMMY_BYTES_1,
    FLM_DUMMY_BYTES_2,
    FLM_DUMMY_BYTES_3
} FLM_DUMMY_BYTES_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM qualifier bytes                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    FLM_QUAL_BYTE_AFTER_CMD_1  = 1,
    FLM_QUAL_BYTE_AFTER_CMD_2,
    FLM_QUAL_BYTE_AFTER_CMD_3,
    FLM_QUAL_BYTE_AFTER_CMD_4,
    FLM_QUAL_BYTE_AFTER_CMD_5,
    FLM_QUAL_BYTE_AFTER_CMD_6,
    FLM_QUAL_BYTE_AFTER_CMD_7
} FLM_QUAL_BYTE_AFTER_CMD_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM qualifier bytes polarity                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    FLM_QUAL_COMP_POL_PASS_BYTE_MATCH,
    FLM_QUAL_COMP_POL_PASS_BYTE_MISMATCH
} FLM_QUAL_COMP_POL_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM SPI inputs signals during Command Byte checking                                                     */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    FLM_CMD_CHECK_ALL_SPI_SIGNALS_DURING_CMD_BYTE = 0,
    FLM_CMD_CHECK_ONLY_DATA_INPUT0_DURING_CMD_BYTE
} FLM_CMD_CHECK_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM status fields                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct FLM_STATUS_FIELDS
{
    /*----------------------------------------------------------------------------------------------------*/
    /* When set, it indicates the FLM discovered an illegal transaction and rejected it.                  */
    /* FALSE: There was no rejected transaction since this bit was last cleared.                          */
    /* TRUE : There was at least one rejected transaction since this bit was last cleared.                */
    /* The details of the first 2 transactions are logged and can be read by calling                      */
    /* FLM_GetRejectionInfo()                                                                             */
    /*----------------------------------------------------------------------------------------------------*/
    BOOLEAN rejectionEvent;

    /*----------------------------------------------------------------------------------------------------*/
    /* Set when FLM_CSI (in Abort Mode) or FLM_CSIO (in Monitoring Mode) transitions from                 */
    /* high to low (active).                                                                              */
    /* FALSE: Event was not detected since this bit was last cleared.                                     */
    /* TRUE : Event was detected since this bit was last cleared.                                         */
    /*----------------------------------------------------------------------------------------------------*/
    BOOLEAN chipSelectLowEvent;

    /*----------------------------------------------------------------------------------------------------*/
    /* Set on most-significant-bit change of any of the FLM Transaction counters                          */
    /* FALSE: Event was not detected since this bit was last cleared.                                     */
    /* TRUE : Event was detected since this bit was last cleared.                                         */
    /*----------------------------------------------------------------------------------------------------*/
    BOOLEAN transactionCounterEvent;

    /*----------------------------------------------------------------------------------------------------*/
    /* Stores the rejected transaction type of the first rejected transaction after "rejectionEvent"      */
    /*  was last cleared, using FLM_ClearEventsStatus(). In case of ambiguity                             */
    /* (e.g., transaction rejected due to multiple reasons) the higher type value is reported.            */
    /* Possible Values are:                                                                               */
    /* 0h: Command byte not in list                                                                       */
    /* 1h: Command address out of range                                                                   */
    /* 2h: Reserved                                                                                       */
    /* 3h: Command data going above the valid address range (in case of burst)                            */
    /* 4h: Data lines 1-3 (FLM_DI1-3) are not stable during command byte transmission.                    */
    /* 5h: Transaction was disqualified by one of the Command Qualifiers                                  */
    /*----------------------------------------------------------------------------------------------------*/
    UINT8   rejectionType1;

    /*----------------------------------------------------------------------------------------------------*/
    /* Same as rejectionType1, but for the second rejected transaction.                                   */
    /*----------------------------------------------------------------------------------------------------*/
    UINT8   rejectionType2;

    /*----------------------------------------------------------------------------------------------------*/
    /* Stores the number of rejected transactions. In case of overflow,                                   */
    /* the value stays at the maximum value (0xFFFF).                                                     */
    /* This field is cleared after clearing "rejectionEvent" using FLM_ClearEventsStatus().               */
    /* Note: Rejected translations are counted even when the chip is in a sleep state.                    */
    /*----------------------------------------------------------------------------------------------------*/
    UINT16  rejectionNumber;

} FLM_STATUS_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM rejection fields                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct FLM_REJ_FIELDS
{
    /*----------------------------------------------------------------------------------------------------*/
    /* Stores the first three bytes of the original address of the rejected transaction                   */
    /* since the "Rejection Event" was cleared (using FLM_ClearEventsStatus() )                           */
    /* If the recorded rejected transaction is shorter than 32 bits, the LSBs of this field will remain 0.*/
    /*----------------------------------------------------------------------------------------------------*/
    UINT32 rejectionAddress;

    /*----------------------------------------------------------------------------------------------------*/
    /* Stores the rejected transaction command byte since the "Rejection Event" was cleared.              */
    /*----------------------------------------------------------------------------------------------------*/
    UINT8 rejectionCommand;

} FLM_REJECT_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM command fields                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct FLM_CMD_FIELDS
{
    /*----------------------------------------------------------------------------------------------------*/
    /* Sets the value of a Command byte.                                                                  */
    /*----------------------------------------------------------------------------------------------------*/
    UINT8                   cmdByte;

    /*----------------------------------------------------------------------------------------------------*/
    /* Defines the command address size of command for limit analysis.                                    */
    /*----------------------------------------------------------------------------------------------------*/
    FLM_ADDR_SIZE_LIM_CMD_T AddrSize;

    /*----------------------------------------------------------------------------------------------------*/
    /* Selects how many data bits are transferred per clock. This parameter is used to count the burst    */
    /* size and ensure it is within the specified range.                                                  */
    /*----------------------------------------------------------------------------------------------------*/
    FLM_BIT_PER_CLK_T       DataBitPerClock;

    /*----------------------------------------------------------------------------------------------------*/
    /* Selects how many address bits are transferred per clock. Note that this field should be            */
    /* set to a value of 0  (single) or to the same value as the DATPCK field.                            */
    /*----------------------------------------------------------------------------------------------------*/
    FLM_BIT_PER_CLK_T       AddrBitPerClock;

    /*----------------------------------------------------------------------------------------------------*/
    /* Selects how many dummy bits are transferred per clock.                                             */
    /*----------------------------------------------------------------------------------------------------*/
    FLM_BIT_PER_CLK_T       DummyBitPerClock;

    /*----------------------------------------------------------------------------------------------------*/
    /* Selects how many dummy bytes are used in this command. This parameter is used to count the         */
    /* burst size and ensure it is within the specified range.                                            */
    /*----------------------------------------------------------------------------------------------------*/
    FLM_DUMMY_BYTES_T       DummyBytes;

    /*----------------------------------------------------------------------------------------------------*/
    /* Defines which address ranges to use for the command.                                               */
    /* Each bit in this field is associated with an FLM_RANGn register; if a bit is 1,                    */
    /* the corresponding FLM_RANGn register is used.                                                      */
    /* CARSEL[0]: Command allowed with limited address range as defined in register FLM_RANG0             */
    /* CARSEL[1]: Command allowed with limited address range as defined in register FLM_RANG1             */
    /* CARSEL[2]: Command allowed with limited address range as defined in register FLM_RANG2             */
    /* CARSEL[3]: Command allowed with limited address range as defined in register FLM_RANG3             */
    /* CARSEL[4]: Command allowed with limited address range as defined in register FLM_RANG4             */
    /* CARSEL[5]: Command allowed with limited address range as defined in register FLM_RANG5             */
    /* CARSEL[6]: Command allowed with limited address range as defined in register FLM_RANG6             */
    /* CARSEL[7]: Command allowed with limited address range as defined in register FLM_RANG7             */
    /*----------------------------------------------------------------------------------------------------*/
    UINT8                   AddrRangeSelect;
} FLM_COMMAND_T;

/*---------------------------------------------------------------------------------------------------------*/
/* FLM qualifier fields                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct FLM_QUAL_FIELDS
{
    /*----------------------------------------------------------------------------------------------------*/
    /* Defines a bit-mask for the byte comparison. For each SPI bit in a byte defined by qualByte         */
    /* 0: SPI bit is not compared (don't care).                                                           */
    /* 1: SPI bit is compared with the corresponding qualVal value.                                       */
    /*----------------------------------------------------------------------------------------------------*/
    UINT8                       qualMask;

    /*----------------------------------------------------------------------------------------------------*/
    /* Defines the compare value of the compared byte.                                                    */
    /*----------------------------------------------------------------------------------------------------*/
    UINT8                       qualVal;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Defines the compared byte index within the transaction.                                             */
    /* 0: Reserved (command byte).                                                                         */
    /* 1: 1st byte after command byte                                                                      */
    /* 2: 2nd byte after command byte                                                                      */
    /* 3: 3rd byte after command byte                                                                      */
    /* 4: 4th byte after command byte                                                                      */
    /* 5: 5th byte after command byte                                                                      */
    /* 6: 6th byte after command byte                                                                      */
    /* 7: 7th byte after command byte                                                                      */
    /* Other:Reserved.                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    FLM_QUAL_BYTE_AFTER_CMD_T   qualByte;

    /*----------------------------------------------------------------------------------------------------*/
    /* Controls the "polarity" of qualifier byte compare logic.                                           */
    /* 0: Qualification pass in case of byte match                                                        */
    /* 1: Qualification pass in case of byte mismatch                                                     */
    /*----------------------------------------------------------------------------------------------------*/
    FLM_QUAL_COMP_POL_T         qualPol;

    /*----------------------------------------------------------------------------------------------------*/
    /* Each qualifier is associated with a group of 8 Commands.                                           */
    /* - Qualifier 0 is associated with commands 0 to 7                                                   */
    /* - Qualifier 1 is associated with commands 8 to 15                                                  */
    /* - Qualifier 2 is associated with commands 26 to 23                                                 */
    /* - Qualifier 3 is associated with commands 24 to 31                                                 */
    /* When a bit in this field is set, the command qualifier functionality is enabled for the            */
    /* corresponding Command. The mapping between qualEn bits and Commands is described by                */
    /* the following formula:                                                                             */
    /* Qualifier J , qualEn bit K : Corresponds to Command{J*8+K}, where: 0<=j<=3 and 0<=k<=7             */
    /*----------------------------------------------------------------------------------------------------*/
    UINT8                       qualEn;

} FLM_QUALIFIER_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Macros to be used as parameters for FLM_InterruptEnable/FLM_ClearEventsStatus                           */
/*---------------------------------------------------------------------------------------------------------*/
#define FLM_INTER_REJECTION_EVENT           0x01
#define FLM_INTER_CHIP_SELECT_LOW_EVENT     0x02
#define FLM_INTER_TRANSACTION_COUNTER_EVENT 0x08
#define FLM_INTER_ALL                       (FLM_INTER_REJECTION_EVENT          | \
                                             FLM_INTER_CHIP_SELECT_LOW_EVENT    | \
                                             FLM_INTER_TRANSACTION_COUNTER_EVENT)

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        Callback function                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module               - FLM module number.                                              */
/*                  flmStatus            - The actual value that was read from the FLM Status register     */
/*                                         bit 0  : When set indicate Rejection Event                      */
/*                                         bit 1  : When set indicate Chip-Select Active Low Event         */
/*                                         bit 3  : When set indicate Transaction Counter Event            */
/*                                         bit 4-6: The first rejected transaction type.                   */
/*                                                  0 - Command byte not in list                           */
/*                                                  1 - Command address out of range                       */
/*                                                  2 - Reserved                                           */
/*                                                  3 - Command data going above the valid address range   */
/*                                                  4 - Data lines 1-3 not stable during command byte      */
/*                                                  5 - Transaction was disqualified                       */
/*                                         bit 7-9: The second rejected transaction type (same as bit 4-6) */
/*                                                                                                         */
/* Description:                                                                                            */
/*                  Callback function for the FLM module. Called from the FLM interrupt.                   */
/*---------------------------------------------------------------------------------------------------------*/
typedef void (*FLM_CALLBACK_T)(FLM_MODULE_T module, UINT32 flmStatus);


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
void FLM_Init (void);

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
/*---------------------------------------------------------------------------------------------------------*/
void FLM_Config (FLM_MODULE_T module, FLM_MODE_T mode, BOOLEAN quadEn);

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
);


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
DEFS_STATUS FLM_GetEventsStatus (FLM_MODULE_T module, FLM_STATUS_T* status);

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
DEFS_STATUS FLM_GetRejectionInfo (FLM_MODULE_T module, UINT8 logNum, FLM_REJECT_T* rej);

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
DEFS_STATUS FLM_ClearEventsStatus (FLM_MODULE_T module, UINT32 clearMask);

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
DEFS_STATUS FLM_InterruptEnable (FLM_MODULE_T module, UINT32 intMask);

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
DEFS_STATUS FLM_ConfigRange (FLM_MODULE_T module, UINT8 rangeIndex, UINT32 startValue, UINT32 endValue);

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
DEFS_STATUS FLM_ConfigCommand (FLM_MODULE_T module, UINT8 index, const FLM_COMMAND_T* cmd);

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
DEFS_STATUS FLM_ConfigByteCommand (FLM_MODULE_T module, UINT8 index, UINT8 cmdByte);

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
DEFS_STATUS FLM_CommandsEnable (FLM_MODULE_T module, UINT32 cmdEnable);

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
DEFS_STATUS FLM_ByteCommandsEnable (FLM_MODULE_T module, UINT32 cmdEnable);

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
DEFS_STATUS FLM_CmdDataVerifyEnable (FLM_MODULE_T module, UINT32 enable);
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
DEFS_STATUS FLM_ClearTranasctionInfo (FLM_MODULE_T module);

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
DEFS_STATUS FLM_GetValidTransactionInfo (FLM_MODULE_T module, UINT32* validCmd, UINT32* validByteCmd);

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
DEFS_STATUS FLM_ConfigQualifier (FLM_MODULE_T module, UINT8 index, const FLM_QUALIFIER_T* qual);

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
DEFS_STATUS FLM_GetTranscationCounter (FLM_MODULE_T module, UINT8 index, UINT32* transactionCountValue);

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
DEFS_STATUS FLM_ConfigTranscationCounter (FLM_MODULE_T module, UINT8 index, UINT8 cmdCountEnByte, UINT8 cmbCountEnByte);

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
DEFS_STATUS FLM_Enable (FLM_MODULE_T module, BOOLEAN enable);

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
BOOLEAN FLM_Enabled (FLM_MODULE_T module);

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
/*---------------------------------------------------------------------------------------------------------*/
void FLM_PermanentLock (FLM_MODULE_T module);

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
void FLM_ReversibleLock (FLM_MODULE_T module, BOOLEAN lock);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FLM_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void FLM_PrintRegs (void);

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
/*---------------------------------------------------------------------------------------------------------*/
void FLM_PrintModuleRegs (FLM_MODULE_T module_num);

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
DEFS_STATUS FLM_SelfTest (void);
#endif

#endif /* _FLM_IF_H */

