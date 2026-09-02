/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2010-2019 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   smb_if.h                                                                                              */
/*            This file contains System Management Bus (SMB) interface                                     */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef _SMB_IF_H
#define _SMB_IF_H

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()

#if defined SMB_MODULE_TYPE
#include __MODULE_HEADER(smb, SMB_MODULE_TYPE)
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* SMB Master/Slave Support                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef SMB_SLAVE_ONLY
#define SMB_MASTER_SUPPORT
#endif
#ifndef SMB_MASTER_ONLY
#define SMB_SLAVE_SUPPORT
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* SMB HW/SW PEC Support                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef SMB_SW_PEC
#undef SMB_CAPABILITY_HW_PEC_SUPPORT
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Special values for SMB_start_master_transaction nwrite/nread arguments                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define SMB_BYTES_QUICK_PROT                        0xFFFF
#define SMB_BYTES_BLOCK_PROT                        0xFFFE
#define SMB_BYTES_EXCLUDE_BLOCK_SIZE_FROM_BUFFER    0xFFFD

/*---------------------------------------------------------------------------------------------------------*/
/* ARP Address match value                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define ARP_ADDRESS_VAL            0x61

/*---------------------------------------------------------------------------------------------------------*/
/* SMB module number. Each module has one bus                                                              */
/*---------------------------------------------------------------------------------------------------------*/
typedef UINT SMB_MODULE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* SMB device mode                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum {
    SMB_SLAVE = 1,
    SMB_MASTER
} SMB_MODE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* External SMB Interface driver states values, which indicate to the upper-level layer the status of the  */
/* operation it initiated or wake up events from one of the buses                                          */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum {
    SMB_NO_STATUS_IND,
    SMB_SLAVE_RCV_IND,
    SMB_SLAVE_XMIT_IND,
#ifdef SMB_WRAP_AROUND_BUFFER
    SMB_SLAVE_XMIT_MISSING_DATA_IND,
#endif
    SMB_SLAVE_RESTART_IND,
    SMB_SLAVE_DONE_IND,
    SMB_MASTER_DONE_IND,
    SMB_NO_DATA_IND,
    SMB_NACK_IND,
    SMB_BUS_ERR_IND,
    SMB_WAKE_UP_IND,
    SMB_MASTER_PEC_ERR_IND,
    SMB_MASTER_BLOCK_BYTES_ERR_IND,
    SMB_SLAVE_PEC_ERR_IND
} SMB_STATE_IND_T;

/*---------------------------------------------------------------------------------------------------------*/
/* SMB Address Type                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum {
    SMB_SLAVE_ADDR1,
    SMB_SLAVE_ADDR2,
    SMB_SLAVE_ADDR3,
    SMB_SLAVE_ADDR4,
    SMB_SLAVE_ADDR5,
    SMB_SLAVE_ADDR6,
    SMB_SLAVE_ADDR7,
    SMB_SLAVE_ADDR8,
    SMB_SLAVE_ADDR9,
    SMB_SLAVE_ADDR10,
    SMB_GC_ADDR,
    SMB_ARP_ADDR
} SMB_ADDR_T;


typedef void (*SMB_CALLBACK_T)(SMB_MODULE_T module_num, SMB_STATE_IND_T op_status, UINT16 info);

#ifdef SMB_SAMPLE
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB Callback function                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMBus module to initialize.                                               */
/*                  op_status  - the status of the operation it initiated or wake up events from one of    */
/*                               the buses                                                                 */
/*                  info       - specific information according to op_status                               */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:     See the below sample code                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_callback (SMB_MODULE_T module_num, SMB_STATE_IND_T op_status, UINT16 info)
{
    switch (op_status)
    {
        case SMB_SLAVE_RCV_IND:
            // Slave got an address match with direction bit clear so it should receive data
            //     the interrupt must call SMB_StartSlaveReceive()
            // info: the enum SMB_ADDR_T address match
            extern UINT16 read_size;
            extern UINT8* read_data_buf;
            SMB_StartSlaveReceive (module_num, read_size, read_data_buf);
            break;
        case SMB_SLAVE_XMIT_IND:
            // Slave got an address match with direction bit set so it should transmit data
            //     the interrupt must call SMB_StartSlaveTransmit()
            // info: the enum SMB_ADDR_T address match
            extern UINT16 write_size;
            extern UINT8* write_data_buf;
            SMB_StartSlaveTransmit (module_num, write_size, write_data_buf);
            break;
        case SMB_SLAVE_DONE_IND:
            // Slave done transmitting or receiving
            // info:
            //     on receive: number of actual bytes received
            //     on transmit: number of actual bytes transmitted,
            //                  when PEC is used 'info' should be (nwrite+1) which means that 'nwrite' bytes
            //                     were sent + the PEC byte
            //                     'nwrite' is the second parameter SMB_StartSlaveTransmit()
            break;
        case SMB_MASTER_DONE_IND:
            // Master transaction finished and all transmit bytes were sent
            // info: number of bytes actually received after the Master receive operation
            //       (if Master didn't issue receive it should be 0)
            break;
        case SMB_NO_DATA_IND:
            // Notify that not all data was received on Master or Slave
            // info:
            //     on receive: number of actual bytes received
            //                 when PEC is used even if 'info' is the expected number of bytes,
            //                     it means that PEC error occurred.
            break;
        case SMB_NACK_IND:
            // MASTER transmit got a NAK before transmitting all bytes
            // info: number of transmitted bytes
            break;
        case SMB_BUS_ERR_IND:
            // Bus error occurred
            // info: has no meaning
            break;
        case SMB_WAKE_UP_IND:
            // SMBus wake up occurred
            // info: has no meaning
            break;
        default:
            break;
    }
}
#endif    /* SMB_SAMPLE */

#ifdef SMB_CAPABILITY_FORCE_SCL_SDA
typedef enum
{
    SMB_LEVEL_LOW  = 0,
    SMB_LEVEL_HIGH = 1
}SMB_LEVEL_T;
#endif // SMB_CAPABILITY_FORCE_SCL_SDA

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_Init                                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  operation_done - pointer to a function which will be called whenever a requested SMBus */
/*                                   operation is done                                                     */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:    This function should be called before any other SMB functions.                         */
/* Description:                                                                                            */
/*                  This routine performs the following:                                                   */
/*                  - Initializes the SMB Interface Driver internal state.                                 */
/*                  - Initializes the SMBus interface.                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_Init (SMB_CALLBACK_T operation_done);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_InitModule                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMBus module to initialize.                                               */
/*                  bus_freq   - Frequency, in KHz, to set for all the buses of the module.                */
/*                                                                                                         */
/* Returns:         FALSE if a module has already been initialized; TRUE otherwise.                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the specified SMB module.                                     */
/*                  This function should be called for a module before calling any function which performs */
/*                  any operation on the module.                                                           */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN SMB_InitModule (SMB_MODULE_T module_num, UINT16 bus_freq);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_Disable                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine clear all slave addresses and disable the module                          */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_Disable (SMB_MODULE_T module_num);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_ModuleIsBusy                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         TRUE in case SMB module is busy; FALSE otherwise.                                      */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns whether the specified SMBus module is busy performing a           */
/*                  transaction, i.e.:                                                                     */
/*                  - SMB issued a START condition                                                         */
/*                  - SMB is the active bus master                                                         */
/*                  - START condition was detected on the bus                                              */
/*                  - SMB is the active slave                                                              */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN SMB_ModuleIsBusy (SMB_MODULE_T module_num);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_BusIsBusy                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         TRUE in case SMB bus is busy; FALSE otherwise.                                         */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns whether the bus is busy performing a transaction, i.e.:           */
/*                  - Bus is active (i.e., a low level on either SDAn or SCLn)                             */
/*                  - Start condition was issued                                                           */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN SMB_BusIsBusy (SMB_MODULE_T module_num);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_IntHandler                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_num - number of triggered interrupt                                                */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Interrupt handlers for SMBus interrupts.                                               */
/*                  Since there are separate ICU interrupts for different modules, the handlers simply     */
/*                  call the main SMBus event handling routine after finding out which SMB module was the  */
/*                  source of the interrupt.                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_IntHandler (UINT16 int_num);

#if defined (MIWU_MODULE_TYPE) && defined (SMB_CAPABILITY_WAKEUP_SUPPORT)
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_WakeupEnable                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB Module number to be configured.                                       */
/*                  enable     - TRUE to enable SMB wake-up; FALSE otherwise.                              */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables/disables SMB wake-up operation for the specified SMB module.      */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_WakeupEnable (SMB_MODULE_T module_num, BOOLEAN enable);
#endif  /* (MIWU_MODULE_TYPE) && (SMB_CAPABILITY_WAKEUP_SUPPORT) */

#ifdef SMB_MASTER_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_StartMasterTransaction                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                  slave_addr - The address of the slave, device, in 7-bit format.                        */
/*                  nwrite     - number of data bytes to write; maximum value 32767;                       */
/*                                      0 = no write transaction.                                          */
/*                               SMB_BYTES_QUICK_PROT = Quick Command - only slave address sent.           */
/*                  nread      - number of bytes to read; maximum value 32767;                             */
/*                                      0 = no read transaction.                                           */
/*                               SMB_BYTES_QUICK_PROT = Quick Command - only slave address sent.           */
/*                               SMB_BYTES_BLOCK_PROT = Block Read protocol is used - i.e., it is assumed  */
/*                               that first read byte holds number of bytes to read. In this case,         */
/*                               the number of read bytes will be returned in callback's "info" argument.  */
/*                  write_data - Buffer containing data to be written.                                     */
/*                  read_data  - Pointer to where read data should be stored.                              */
/*                  use_PEC    - Use PEC CRC or not.                                                       */
/*                                                                                                         */
/* Returns:         FALSE if write operation cannot be started, e.g. because bus is busy or illegal        */
/*                  argument values; TRUE otherwise.                                                       */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine Initiates a master write and/or read SMBus transaction.                   */
/*                  In receive mode, nread is set to the value of the first read byte in accordance with   */
/*                  SMBus Block Write/Read protocol; the initial (input) value should be either:           */
/*                  - 0 = no master receive.                                                               */
/*                  - 1 = receive one byte.                                                                */
/*                  - 2 = read two bytes.                                                                  */
/*                  - 3 = read three or more bytes (Block Read or Block Write/Read).                       */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN SMB_StartMasterTransaction (SMB_MODULE_T module_num, UINT8 slave_addr, UINT16 nwrite, UINT16 nread,
                                    UINT8 *write_data, UINT8 *read_data, BOOLEAN use_PEC);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_MasterAbort                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine cleans up the SMB interface and registers                                 */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_MasterAbort (SMB_MODULE_T module_num);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_Recovery                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs a complete recovery to the SMBus:                                */
/*                  - performs data abort on the bus                                                       */
/*                  - resets the bus                                                                       */
/*                  - re-enable interrupt if interrupt was enabled before reset                            */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_Recovery (SMB_MODULE_T module_num);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_StartStop                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine resets the SMBus by asserting SMBus Start and Stop conditions             */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_StartStop (SMB_MODULE_T module_num);
#endif  /* SMB_MASTER_SUPPORT */

#ifdef SMB_SLAVE_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_IsSlaveAddressExist                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - The SMB module number                                                     */
/*                  addr       - The slave address required                                                */
/*                                                                                                         */
/* Returns:         TRUE if the required slave address is already enabled                                  */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if the given slave address is already configure                    */
/*                  (and also probably enabled) in the specific SMBus module.                              */
/*                  It will return TRUE if the required slave address is already enabled.                  */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN SMB_IsSlaveAddressExist (SMB_MODULE_T module_num, UINT8 addr);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_SetSlaveAddress                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num        - The SMB module number                                              */
/*                  slaveAddrToAssign - Address to assign                                                  */
/*                  use_PEC           - Use PEC CRC or not                                                 */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if succeeded,                                                           */
/*                  DEFS_STATUS_FAIL if all the slots for the addresses were full                          */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine receives an address to set to be recognized as a valid slave address      */
/*                  and adds it to the first free register it finds.                                       */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS SMB_AddSlaveAddress (SMB_MODULE_T module_num, UINT8 slaveAddrToAssign, BOOLEAN use_PEC);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_RemSlaveAddress                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num        - The SMB module number                                              */
/*                  slaveAddrToRemove - Address to remove                                                  */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if succeeded or if it doesn't exist                                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine receives an address to remove from being recognized as a valid slave      */
/*                  address and removes it if it was assigned.                                             */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS SMB_RemSlaveAddress (SMB_MODULE_T module_num, UINT8 slaveAddrToRemove);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_GetCurrentSlaveAddress                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK - if the parameter was valid                                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  The slave address of the current/last SMB transaction                                  */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS SMB_GetCurrentSlaveAddress (SMB_MODULE_T module_num, UINT8* currSlaveAddr);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_SlaveGlobalCallEnable                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB Module number to be configured.                                       */
/*                  enable     - When TRUE,  enable Global call                                            */
/*                               When FALSE, disable Global call                                           */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables or disables Global call                                           */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS SMB_SlaveGlobalCallEnable (SMB_MODULE_T module_num, BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_SlaveARPEnable                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB Module number to be configured.                                       */
/*                  enable     - When TRUE,  enable ARP                                                    */
/*                               When FALSE, disable ARP                                                   */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables or disables ARP                                                   */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS SMB_SlaveARPEnable (SMB_MODULE_T module_num, BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_StartSlaveReceive                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                  nread      - Number of bytes to read or buffer size; maximum 32767.                    */
/*                               Should include the PEC byte if PEC is used.                               */
/*                               Actual number of read bytes will be returned in callback's "info"         */
/*                               argument                                                                  */
/*                               SMB_BYTES_BLOCK_PROT = Block Read protocol is used - i.e. it is assumed   */
/*                               that first read byte holds number of bytes to read. In this case, the     */
/*                               number of read bytes will be returned in callback's "info" argument.      */
/*                  read_data  - Pointer to where read data should be stored                               */
/*                                                                                                         */
/* Returns:         FALSE if write operation cannot be started, e.g. because bus is busy; TRUE otherwise.  */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initiates a slave read SMBus transaction.                                 */
/*                  Since in slave mode the number of bytes to read is unknown, the maximum buffer size    */
/*                  should be provided (master will terminate the transaction).                            */
/*                                                                                                         */
/*                  NOTE: This function should be called from the operation done callback to eliminate a   */
/*                  condition of slave being notified but not yet starting transaction - and thus an       */
/*                  endless interrupt from SDAST for the slave RCV or TX !                                 */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN SMB_StartSlaveReceive (SMB_MODULE_T module_num, UINT16 nread, UINT8 *read_data);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_StartSlaveTransmit                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                  nwrite     - Number of bytes to write (including the address).                         */
/*                  write_data - Buffer containing data to be written.                                     */
/*                                                                                                         */
/* Returns:         FALSE if write operation cannot be started, e.g. because bus is busy; TRUE otherwise.  */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initiates a slave write SMBus transaction.                                */
/*                                                                                                         */
/*                  NOTE: This function should be called from the operation done callback to eliminate a   */
/*                  condition of slave being notified but not yet starting transaction - and thus an       */
/*                  endless interrupt from SDAST for the slave RCV or TX !                                 */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN SMB_StartSlaveTransmit (SMB_MODULE_T module_num, UINT16 nwrite, UINT8 *write_data);
#endif  /* SMB_SLAVE_SUPPORT */

#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_EnableTimeout                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMBus module to initialize.                                               */
/*                  enable     - TRUE to enable SMBus Timeout Detection; FALSE to disable.                 */
/*                                                                                                         */
/* Returns:         none.                                                                                  */
/*                                                                                                         */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the SMBus Timeout Detection hardware.                          */
/*                  When enabled, it sets the SMB Bus Timeout Clock Divisor.                               */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_EnableTimeout (SMB_MODULE_T module_num, BOOLEAN enable);
#endif

#ifdef SMB_CAPABILITY_START_TO_STOP_TIMEOUT_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_EnableStartToStopTimeout                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMBus module to initialize.                                               */
/*                  timeout    - Any value between 35 to 1011.                                             */
/*                               If timeout < 35 or timeout > 1011 disable this timeout.                   */
/* Returns:         none.                                                                                  */
/*                                                                                                         */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the "Strat To Stop" timeout.                                   */
/*                  If during transaction the time elapsed from a Start condition without a Stop condition */
/*                  is longer than the defined timeout. The SMB HW will issue a timeout indication and the */
/*                  transaction will be aborted.                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_EnableStartToStopTimeout (SMB_MODULE_T module_num, UINT16 timeout);
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_SetStallAfterStartIdle                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number                                                         */
/*                  enable     -  boolean indicating whether to enable or disable the feature              */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets a stall-after-start-on-idle feature                                  */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_SetStallAfterStartIdle (SMB_MODULE_T module_num, BOOLEAN enable);

#ifdef SMB_STALL_TIMEOUT_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_ConfigStallThreshold                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number                                                         */
/*                  threshold  - number of ticks to wait before aborting/recovering a transaction          */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Set the stall threshold for the requested SMBus module                                 */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_ConfigStallThreshold (UINT module_num, UINT8 threshold);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_StallHandler                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Should be called periodically every X milliseconds . Used for recovering a stalled     */
/*                  Master transaction. For each active SMB modules, if there is no activity on the SMBus  */
/*                  lines for (X * stall_threshold) milliseconds the transaction for the specific module   */
/*                  will be aborted.                                                                       */
/*                  The default stall_threshold = 25, if needed change it with SMB_ConfigStallThreshold()  */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_StallHandler (void);
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_ReEnableModule                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number                                                         */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine re-enables the module after idle mode. According to the spec, all the     */
/*                  registers are retained in IDLE mode besides SMBCTL1                                    */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_ReEnableModule (SMB_MODULE_T module_num);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_InterruptIsPending                                                                 */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         BOOLEAN                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if we have pending interrupt                                       */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN SMB_InterruptIsPending (void);

#ifdef SMB_CAPABILITY_FORCE_SCL_SDA
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_WriteSCL                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number                                                         */
/*                  level      - SCL level                                                                 */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine updates SCL current level to module chosen                                */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_WriteSCL (SMB_MODULE_T module_num, SMB_LEVEL_T level);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_WriteSDA                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number                                                         */
/*                  level      - SDA level                                                                 */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine updates SDA current level to module chosen                                */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_WriteSDA (SMB_MODULE_T module_num, SMB_LEVEL_T level);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_GetSDA                                                                             */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/*                                                                                                         */
/* Returns:                                                                                                */
/*                  level - SDA level (SMB_LEVEL_LOW/SMB_LEVEL_HIGH)                                       */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine retrieves the current SDA level                                           */
/*---------------------------------------------------------------------------------------------------------*/
SMB_LEVEL_T SMB_GetSDA (SMB_MODULE_T module_num);
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintRegs (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_PrintModuleRegs                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - The module to be printed.                                                 */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module instance registers                                      */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintModuleRegs (SMB_MODULE_T module_num);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_PrintVars                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module variables                                               */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintVars (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_PrintModuleVars                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - The module to be printed.                                                 */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module instance variables                                      */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintModuleVars (SMB_MODULE_T module_num);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_PrintVersion                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintVersion (void);

#endif  /* _SMB_IF_H */

