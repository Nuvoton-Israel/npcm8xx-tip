/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2013-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   i2c_if.h                                                                                              */
/*            This file contains I2C Logical Module driver                                                 */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef __I2C_IF_H__
#define __I2C_IF_H__

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               DEFINITIONS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Master/Slave Support                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef I2C_SLAVE_ONLY
#define I2C_MASTER_SUPPORT
#endif
#ifndef I2C_MASTER_ONLY
#define I2C_SLAVE_SUPPORT
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* I2C channel type                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
typedef  UINT I2C_CHANNEL_T;

/*---------------------------------------------------------------------------------------------------------*/
/* I2C event type                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    I2C_DONE = 0,
    I2C_RECEIVE_DONE,
    I2C_SENT_DONE,
    I2C_READ_AFTER_WRITE,
    I2C_UNDEFINED_EVENT,
} I2C_TYPE_OF_EVENT;

/*---------------------------------------------------------------------------------------------------------*/
/* I2C callaback type                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
typedef void (*I2C_EVENT_FINISHED_T) (
    I2C_CHANNEL_T       channel,
    UINT8               address,
    UINT16              actualSize,
    DEFS_STATUS         status,
    I2C_TYPE_OF_EVENT   eventType
);


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                              API FUNCTIONS                                              */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Init                                                                               */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes I2C module                                                    */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Init(void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_StateIsBusy                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch               - Channel number                                                      */
/*                                                                                                         */
/* Returns:         bool: State is busy (true)                                                             */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if I2C state is busy performing a transaction                      */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN I2C_StateIsBusy (I2C_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_ModuleIsBusy                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch               - Channel number                                                      */
/*                                                                                                         */
/* Returns:         bool: module is busy (true)                                                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if SMB module is busy performing a transaction                     */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN I2C_ModuleIsBusy (I2C_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_BusIsBusy                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch               - Channel number                                                      */
/*                                                                                                         */
/* Returns:         bool: bus is busy (true)                                                               */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if the SMB bus is busy                                             */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN I2C_BusIsBusy (I2C_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_InterruptIsPending                                                                 */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         BOOLEAN                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if we have pending interrupt                                       */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN I2C_InterruptIsPending (void);

#ifdef I2C_MASTER_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_Init                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch      - Channel number                                                               */
/*                  freq    - Channel frequency in kHZ                                                     */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes master channel                                                */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Master_Init (I2C_CHANNEL_T ch, UINT32 freq);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_DeInit                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch      - Channel number                                                               */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine disables master channel                                                   */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Master_DeInit (I2C_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_ChangeFrequency                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch      - Channel number                                                               */
/*                  freq    - Channel frequency (in HZ)                                                    */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if the change was done or the frequency was the same as the             */
/*                  original frequency. Otherwise return DEFS_STATUS_FAIL                                  */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine changes the channel frequency                                             */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Master_ChangeFrequency (I2C_CHANNEL_T ch, UINT32 freq);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_WriteRead                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch                  - Channel number                                                   */
/*                  slaveAddr           - Slave address in 7-bit format                                    */
/*                  writeBuffer         - Write buffer                                                     */
/*                  writeSize           - Write buffer size                                                */
/*                  readBuffer          - Read buffer                                                      */
/*                  readSize            - Read buffer size                                                 */
/*                  readWriteFinished   - ReadWrite finished callback                                      */
/*                  use_PEC             - Use PEC CRC or not.                                              */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs I2C master write-read transaction                                */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Master_WriteRead (  I2C_CHANNEL_T           ch,
                                    UINT8                   slaveAddr,
                                    void*                   writeBuffer,
                                    UINT16                  writeSize,
                                    void*                   readBuffer,
                                    UINT16                  readSize,
                                    I2C_EVENT_FINISHED_T    readWriteFinished,
                                    BOOLEAN                 use_PEC);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_WriteReadBlocking                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch                  - Channel number                                                   */
/*                  slaveAddr           - Slave address in 7-bit format                                    */
/*                  writeBuffer         - Write buffer                                                     */
/*                  writeSize           - Write buffer size                                                */
/*                  readBuffer          - Read buffer                                                      */
/*                  readSize            - Read buffer size                                                 */
/*                  readWriteFinished   - ReadWrite finished callback                                      */
/*                  use_PEC             - Use PEC CRC or not.                                              */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs blocking I2C master write-read transaction                       */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Master_WriteReadBlocking (  I2C_CHANNEL_T   ch,
                                            UINT8           slaveAddr,
                                            void*           writeBuffer,
                                            UINT16          writeSize,
                                            void*           readBuffer,
                                            UINT16          readSize,
                                            BOOLEAN         use_PEC);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_Recover                                                                     */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reinitialize the I2C module and recover SMBus.                            */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Master_Recover (I2C_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_Read                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch              - Channel number                                                       */
/*                  slaveAddr       - Slave address in 7-bit format                                        */
/*                  buffer          - Data buffer                                                          */
/*                  size            - Data buffer size                                                     */
/*                  readFinished    - Read finished callback                                               */
/*                  PEC_en          - Use PEC CRC or not.                                                  */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables non-blocking master read from slave device                        */
/*---------------------------------------------------------------------------------------------------------*/
#define I2C_Master_Read(ch, slaveAddr, buffer, size, readFinished, PEC_en)                                  \
            I2C_Master_WriteRead(ch, slaveAddr, NULL, 0, buffer, size, readFinished, PEC_en)

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_Write                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch              - Channel number                                                       */
/*                  slaveAddr       - Slave address in 7-bit format                                        */
/*                  buffer          - Data buffer                                                          */
/*                  size            - Data buffer size                                                     */
/*                  writeFinished   - Write finished callback                                              */
/*                  PEC_en          - Use PEC CRC or not.                                                  */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs non-blocking master write                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define I2C_Master_Write(ch, slaveAddr, buffer, size, writeFinished, PEC_en)                                \
            I2C_Master_WriteRead(ch, slaveAddr, buffer, size, NULL, 0, writeFinished, PEC_en)

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_ReadBlocking                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch        - Channel number                                                             */
/*                  slaveAddr - Slave address in 7-bit format                                              */
/*                  buffer    - Data buffer                                                                */
/*                  size      - Data buffer size                                                           */
/*                  PEC_en    - Use PEC CRC or not.                                                        */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs blocking read                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define I2C_Master_ReadBlocking(ch, slaveAddr, buffer, size, PEC_en)                                        \
            I2C_Master_WriteReadBlocking(ch, slaveAddr, NULL, 0, buffer, size, PEC_en)

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_WriteBlocking                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch        - Channel number                                                             */
/*                  slaveAddr - Slave address in 7-bit format                                              */
/*                  buffer    - Data buffer                                                                */
/*                  size      - Data buffer size                                                           */
/*                  PEC_en    - Use PEC CRC or not.                                                        */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs blocking write                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define I2C_Master_WriteBlocking(ch, slaveAddr, buffer, size, PEC_en)                                       \
            I2C_Master_WriteReadBlocking(ch, slaveAddr, buffer, size, NULL, 0, PEC_en)

#endif  // I2C_MASTER_SUPPORT


#ifdef I2C_SLAVE_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_Init                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch           - Channel number                                                          */
/*                  freq         - Channel frequency (in HZ)                                               */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes slave module. Set to max frequecny (400kHz)                   */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_Init (I2C_CHANNEL_T ch, UINT32 freq);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_DeInit                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch    - Channel number                                                                 */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine disables the I2C in hardware as well as software                          */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_DeInit (I2C_CHANNEL_T ch);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_Add                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch           - Channel number                                                          */
/*                  slaveAddr    - Slave address to add in 7-bit format                                    */
/*                  use_PEC      - Use PEC CRC or not.                                                     */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine adds a slave address to the SMB module                                    */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_Add (I2C_CHANNEL_T ch, UINT8 slaveAddr, BOOLEAN use_PEC);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_Remove                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch           - Channel number                                                          */
/*                  slaveAddr    - Slave address to remove in 7-bit format                                 */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine clears a slave address from the SMB module                                */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_Remove (I2C_CHANNEL_T ch, UINT8 slaveAddr);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_Read                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch              - Channel number                                                       */
/*                  buffer          - Data buffer                                                          */
/*                  size            - Data buffer size                                                     */
/*                  readFinished    - Read finished callback                                               */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables non-blocking slave read from master device                        */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_Read (I2C_CHANNEL_T ch, void* buffer, UINT16 size, I2C_EVENT_FINISHED_T readFinished);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_ReadContinue                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch              - Channel number                                                       */
/*                  buffer          - Data buffer                                                          */
/*                  size            - Data buffer size                                                     */
/*                  readFinished    - Read finished callback                                               */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine continues slave read . It's used for fragmented slave read .              */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_ReadContinue (I2C_CHANNEL_T ch, void* buffer, UINT16 size, I2C_EVENT_FINISHED_T readFinished);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_Write                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch               - Channel number                                                      */
/*                  buffer           - Data buffer                                                         */
/*                  size             - Data buffer size                                                    */
/*                  writeFinished    - Write finished callback                                             */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs non-blocking slave write                                         */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_Write (I2C_CHANNEL_T ch, void* buffer, UINT16 size, I2C_EVENT_FINISHED_T writeFinished);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_ReadBlocking                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch              - Channel number                                                       */
/*                  buffer          - Data buffer                                                          */
/*                  size            - Data buffer size                                                     */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables blocking slave read from master device                            */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_ReadBlocking (I2C_CHANNEL_T ch, void* buffer, UINT16 size);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_WriteBlocking                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch               - Channel number                                                      */
/*                  buffer           - Data buffer                                                         */
/*                  size             - Data buffer size                                                    */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs blocking slave write                                             */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_WriteBlocking (I2C_CHANNEL_T ch, void* buffer, UINT16 size);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_Recover                                                                      */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reinitialize the I2C module and recover SMBus.                            */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_Recover (I2C_CHANNEL_T ch);

#endif // I2C_SLAVE_SUPPORT

#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_ConfigTimeout                                                                      */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/*                  module_num      - SMBus module to initialize.                                          */
/*                  scl_low_timeout - TRUE: Enable 25ms SCL suck low timeout; FALSE: Disable this timeout  */
/*                  frame_timeout   - configures the "Strat To Stop" timeout. Can be any value between     */
/*                                    35 to 1011. Other values will disable this timeout.                  */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module variables                                               */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_ConfigTimeout (I2C_CHANNEL_T ch, BOOLEAN scl_low_timeout , UINT16 frame_timeout);
#endif

#ifdef I2C_HANDLE_TIMEOUT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_EnableTimeout                                                                      */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/*                  ch       - Channel number.                                                             */
/*                  enable   - TRUE: Enable Timeout handling; FALSE to disable Timeout handling.           */
/*                                                                                                         */
/* Returns:         none.                                                                                  */
/*                                                                                                         */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enable (or disable) the I2C driver capability to automatically handle     */
/*                  the SMBus HW Timeout during transaction (enable/config/disable when needed)            */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_EnableTimeout (I2C_CHANNEL_T ch, BOOLEAN enable);
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_PrintVars                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module variables                                               */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_PrintVars (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_PrintModuleVars                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  channel_num - The module to be printed.                                                */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module instance variables                                      */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_PrintModuleVars (I2C_CHANNEL_T channel_num);

#endif //__I2C_IF_H__

