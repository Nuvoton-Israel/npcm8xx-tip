/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2013-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   i2c_drv.c                                                                                             */
/*            This file contains I2C driver implementation over SMB module                                 */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_DRV()

#include "i2c_drv.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Module Dependencies                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#if defined (SMB_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_DRV(smb)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                  TYPES                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

typedef enum
{
    I2C_STATE_ERROR = 0,
    I2C_STATE_DISABLE,
    I2C_STATE_IDLE,
    I2C_STATE_MASTER_READ,
    I2C_STATE_MASTER_WRITE,
    I2C_STATE_SLAVE_READ,
    I2C_STATE_SLAVE_WRITE,
    I2C_STATE_SLAVE_RESTART
} I2C_STATE_T;

typedef struct I2C_TRANS_T {
    I2C_EVENT_FINISHED_T    event;          // callback event
    void*                   buffWrite;      // pointer to write buffer
    UINT16                  buffWriteSize;  // write buffer size
    void*                   buffRead;       // pointer to read buffer
    UINT16                  buffReadSize;   // read buffer size
} I2C_TRANS_T;

typedef struct I2C_STATUS_T {
#ifdef I2C_MASTER_SUPPORT
    I2C_TRANS_T             master;         // Master: callback related variables
#endif
#ifdef I2C_SLAVE_SUPPORT
    I2C_TRANS_T             slave;          // Slave:  callback related variables
#endif
    I2C_STATE_T             state;          // Main driver state
    UINT32                  freq;           // Channel frequency (in HZ)
    DEFS_STATUS             blockingStatus; // The status of the last Master blocking transaction.
#ifdef I2C_HANDLE_TIMEOUT
    BOOLEAN                 autoTimeout;    // Automaticaly handle (enable/disable/config) the smb driver HW timeout.
    UINT32                  microsecPerByte;// time to transmit one byte (in microseconds). Depend on the channel frequency.
#endif
} I2C_STATUS_T;


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                VARIABLES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

static volatile I2C_STATUS_T I2C_status[I2C_NUM_OF_CHANNELS];


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                  LOCAL FUNCTIONS FORWARD DECLARATIONS                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#ifdef HAL_PRINT_CAPABILITY
static const char* I2C_StateStr (I2C_STATE_T stateType);
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             COMMON FUNCTIONS                                            */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

//lint -e{715} Suppress 'address/actualSize/eventType' not referenced
#ifdef I2C_MASTER_SUPPORT
static void I2C_SMBBlockingCallback_L (
    I2C_CHANNEL_T       channel,
    UINT8               address,
    UINT16              actualSize,
    DEFS_STATUS         status,
    I2C_TYPE_OF_EVENT   eventType
)
{
    I2C_status[channel].blockingStatus = status;
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_Callback_L                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num    - Module number                                                          */
/*                  op_status     - Operation status                                                       */
/*                  info          - Operation info                                                         */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles SMB events for external callback                                  */
/*lint -e{550}      ret not used                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
static void I2C_SMBCallback_L (SMB_MODULE_T module_num, SMB_STATE_IND_T op_status, UINT16 info)
{
    I2C_CHANNEL_T               ch          = (I2C_CHANNEL_T)module_num;
    UINT8                       address     = 0;
    static volatile I2C_STATE_T i2c_event   = I2C_STATE_DISABLE;
#ifdef I2C_SLAVE_SUPPORT
    volatile BOOLEAN            ret         = FALSE;
    ret = ret;  /* this line exist in order to prevent warning in GCC. */

    switch (op_status)
    {
        case SMB_SLAVE_RCV_IND:
        case SMB_SLAVE_XMIT_IND:
        case SMB_SLAVE_RESTART_IND:
        case SMB_SLAVE_DONE_IND:
            if (SMB_GetCurrentSlaveAddress(module_num, &address) != DEFS_STATUS_OK)
            {
                return;
            }
            break;

        default:
            break;
    }
#endif

    i2c_event = I2C_status[ch].state;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Inore interrupts before channel is initialized                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (I2C_status[ch].state == I2C_STATE_DISABLE)
    {
        return;
    }

    switch (op_status)
    {
#ifdef I2C_MASTER_SUPPORT
        case SMB_MASTER_DONE_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* zero the counters. This is for blocking commands                                            */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].master.buffReadSize  = 0;
            I2C_status[ch].master.buffWriteSize = 0;

            /*---------------------------------------------------------------------------------------------*/
            /* Set state                                                                                   */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].state = I2C_STATE_IDLE;

#ifdef I2C_HANDLE_TIMEOUT
            if (I2C_status[ch].autoTimeout)
            {
                I2C_ConfigTimeout(ch, FALSE, 0);
            }
#endif
            /*---------------------------------------------------------------------------------------------*/
            /* Execute callback : master mode                                                              */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(I2C_status[ch].master.event, (ch, 0, info, DEFS_STATUS_OK, I2C_DONE));

            break;

        case SMB_NACK_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* zero the counters. This is for blocking commands                                            */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].master.buffReadSize  = 0;
            I2C_status[ch].master.buffWriteSize = 0;

#ifdef I2C_HANDLE_TIMEOUT
            if (I2C_status[ch].autoTimeout)
            {
                I2C_ConfigTimeout(ch, FALSE, 0);
            }
#endif
            /*---------------------------------------------------------------------------------------------*/
            /* Set state                                                                                   */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].state = I2C_STATE_IDLE;

            /*---------------------------------------------------------------------------------------------*/
            /* Execute callback                                                                            */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(I2C_status[ch].master.event, (ch, 0, 0, DEFS_STATUS_NO_CONNECTION, I2C_DONE));

            break;

        case SMB_MASTER_PEC_ERR_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* zero the counters. This is for blocking commands                                            */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].master.buffReadSize  = 0;
            I2C_status[ch].master.buffWriteSize = 0;

            /*---------------------------------------------------------------------------------------------*/
            /* Set state                                                                                   */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].state = I2C_STATE_IDLE;

#ifdef I2C_HANDLE_TIMEOUT
            if (I2C_status[ch].autoTimeout)
            {
                I2C_ConfigTimeout(ch, FALSE, 0);
            }
#endif
            /*---------------------------------------------------------------------------------------------*/
            /* Execute callback : master mode                                                              */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(I2C_status[ch].master.event, (ch, 0, info, DEFS_STATUS_BAD_CHECKSUM, I2C_DONE));

            break;

        case SMB_MASTER_BLOCK_BYTES_ERR_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* zero the counters. This is for blocking commands                                            */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].master.buffReadSize  = 0;
            I2C_status[ch].master.buffWriteSize = 0;

            /*---------------------------------------------------------------------------------------------*/
            /* Set state                                                                                   */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].state = I2C_STATE_IDLE;

#ifdef I2C_HANDLE_TIMEOUT
            if (I2C_status[ch].autoTimeout)
            {
                I2C_ConfigTimeout(ch, FALSE, 0);
            }
#endif
            /*---------------------------------------------------------------------------------------------*/
            /* Execute callback : master mode                                                              */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(I2C_status[ch].master.event, (ch, address, 0, DEFS_STATUS_COMMUNICATION_ERROR, I2C_DONE));

            break;
#endif

#ifdef I2C_SLAVE_SUPPORT
        case SMB_SLAVE_RCV_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* Start slave receive                                                                         */
            /*---------------------------------------------------------------------------------------------*/
            if (I2C_status[ch].slave.buffReadSize != 0)
            {
                I2C_status[ch].state = I2C_STATE_SLAVE_READ;

                ret = SMB_StartSlaveReceive(module_num, I2C_status[ch].slave.buffReadSize, (UINT8*)I2C_status[ch].slave.buffRead);
                ASSERT(ret);
            }
            else
            {
                /*-----------------------------------------------------------------------------------------*/
                /* i2c driver don't know where to put the data from host !                                 */
                /*-----------------------------------------------------------------------------------------*/
                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info,
                             DEFS_STATUS_RESOURCE_ERROR, I2C_RECEIVE_DONE)); //read buffer is missing.
            }

            break;

        /*-------------------------------------------------------------------------------------------------*/
        /* Restart is a read done + xmit interrupts together                                               */
        /*-------------------------------------------------------------------------------------------------*/
        case SMB_SLAVE_RESTART_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* Execute callback: notify end of read operation. User should  : process the read data,       */
            /* to select the write data  / RESTART = RECEIVE DONE + XMIT                                   */
            /*---------------------------------------------------------------------------------------------*/
            if (I2C_status[ch].state == I2C_STATE_SLAVE_READ)
            {
                I2C_status[ch].slave.buffReadSize = 0;

                I2C_status[ch].state = I2C_STATE_SLAVE_RESTART;

                /*-----------------------------------------------------------------------------------------*/
                /* In this callback the upper level should fill the write buffer and its size using        */
                /* I2C_Slave_Write                                                                         */
                /*-----------------------------------------------------------------------------------------*/
                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info, DEFS_STATUS_OK, I2C_READ_AFTER_WRITE));

                if (I2C_status[ch].slave.buffWriteSize != 0)
                {
                    I2C_status[ch].state = I2C_STATE_SLAVE_WRITE;

                    ret = SMB_StartSlaveTransmit(module_num, I2C_status[ch].slave.buffWriteSize, (UINT8*)I2C_status[ch].slave.buffWrite);

                    ASSERT(ret);
                }
                /*-----------------------------------------------------------------------------------------*/
                /*              Restart in the middle of read. Read some more.                             */
                /*-----------------------------------------------------------------------------------------*/
                else if (I2C_status[ch].slave.buffReadSize != 0)
                {
                    I2C_status[ch].state = I2C_STATE_SLAVE_READ;

                    ret = SMB_StartSlaveReceive(module_num, I2C_status[ch].slave.buffReadSize, (UINT8*)I2C_status[ch].slave.buffRead);

                    ASSERT(ret);
                }
                else
                {
                    EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info,
                                 DEFS_STATUS_RESOURCE_ERROR, I2C_READ_AFTER_WRITE));
                }
            }
            else // restart in the middle of slave write? Currently not supported.
            {
                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info,
                             DEFS_STATUS_COMMUNICATION_ERROR, I2C_READ_AFTER_WRITE));
            }
            break;

        case SMB_SLAVE_XMIT_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* Slave got an address match with direction bit set so it should transmit data the interrupt  */
            /* must call SMB_StartSlaveTransmit()                                                          */
            /*                                                                                             */
            /* info: the enum SMB_ADDR_T address match                                                     */
            /*---------------------------------------------------------------------------------------------*/
            if (I2C_status[ch].slave.buffWriteSize != 0)
            {
                I2C_status[ch].state = I2C_STATE_SLAVE_WRITE;

                ret = SMB_StartSlaveTransmit(module_num, I2C_status[ch].slave.buffWriteSize, (UINT8*)I2C_status[ch].slave.buffWrite);
                ASSERT(ret);
            }
            /*---------------------------------------------------------------------------------------------*/
            /* i2c driver don't know what to transmit !                                                    */
            /*---------------------------------------------------------------------------------------------*/
            else
            {
                I2C_status[ch].state = I2C_STATE_SLAVE_WRITE;

                /* Notice the zero: no data was sent */
                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, 0,
                             DEFS_STATUS_RESOURCE_ERROR, I2C_SENT_DONE)); //write buffer is missing

                if (I2C_status[ch].slave.buffWriteSize != 0)
                {
                    ret = SMB_StartSlaveTransmit(module_num, I2C_status[ch].slave.buffWriteSize, (UINT8*)I2C_status[ch].slave.buffWrite);
                    ASSERT(ret);
                }
                else
                {
                    EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info,
                                 DEFS_STATUS_COMMUNICATION_ERROR, I2C_SENT_DONE));
                }
            }

            break;

#ifdef SMB_WRAP_AROUND_BUFFER
         case SMB_SLAVE_XMIT_MISSING_DATA_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* Slave transmitted all the data in the buffer, and end device wants more. select additional  */
            /* buffer. must call SMB_StartSlaveTransmit()                                                  */
            /*                                                                                             */
            /* info: the enum SMB_ADDR_T address match                                                     */
            /*---------------------------------------------------------------------------------------------*/

            /*---------------------------------------------------------------------------------------------*/
            /* i2c driver don't know what to transmit !                                                    */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].state = I2C_STATE_IDLE;

            /* Notice the zero: no data was sent */
            EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info,
                         DEFS_STATUS_CUSTOM_ERROR_07, I2C_SENT_DONE)); //write buffer is missing

            if (I2C_status[ch].slave.buffWriteSize != 0)
            {
                I2C_status[ch].state = I2C_STATE_SLAVE_WRITE;
                ret = SMB_StartSlaveTransmit(module_num, I2C_status[ch].slave.buffWriteSize, (UINT8*)I2C_status[ch].slave.buffWrite);
                ASSERT(ret);
            }
            else
            {
                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info,
                             DEFS_STATUS_COMMUNICATION_ERROR, I2C_SENT_DONE));
            }

            break;
 #endif
        case SMB_SLAVE_DONE_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* Slave done transmitting or receiving                                                        */
            /* info:                                                                                       */
            /*      on receive: number of actual bytes received                                            */
            /*      on transmit: number of actual bytes transmitted,                                       */
            /*                                                                                             */
            /* when PEC is used 'info' should be (nwrite+1) which means that 'nwrite' bytes were sent +    */
            /* the PEC byte 'nwrite' is the second parameter SMB_StartSlaveTransmit()                      */
            /*---------------------------------------------------------------------------------------------*/

            I2C_status[ch].state = I2C_STATE_IDLE;

            if (i2c_event == I2C_STATE_SLAVE_READ)
            {
                I2C_status[ch].slave.buffReadSize = 0;
                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info, DEFS_STATUS_OK, I2C_RECEIVE_DONE));
            }
            else if (i2c_event == I2C_STATE_SLAVE_WRITE)
            {
                I2C_status[ch].slave.buffWriteSize = 0;
                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info, DEFS_STATUS_OK, I2C_SENT_DONE));
            }
            else
            {
                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info,
                             DEFS_STATUS_COMMUNICATION_ERROR, I2C_UNDEFINED_EVENT));
            }

            break;

        case SMB_SLAVE_PEC_ERR_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* zero the counters. This is for blocking commands                                            */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].slave.buffReadSize  = 0;
            I2C_status[ch].slave.buffWriteSize = 0;

            /*---------------------------------------------------------------------------------------------*/
            /* Set state                                                                                   */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].state = I2C_STATE_IDLE;

            /*---------------------------------------------------------------------------------------------*/
            /* Execute callback : slave mode                                                               */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info, DEFS_STATUS_BAD_CHECKSUM, I2C_RECEIVE_DONE));

            break;
#endif
        case SMB_BUS_ERR_IND:
            /*---------------------------------------------------------------------------------------------*/
            /* Execute callback                                                                            */
            /*---------------------------------------------------------------------------------------------*/
            I2C_status[ch].state = I2C_STATE_IDLE;

            if (i2c_event == I2C_STATE_SLAVE_READ)
            {
#ifdef I2C_SLAVE_SUPPORT
                I2C_status[ch].slave.buffReadSize = 0;

                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info,
                             DEFS_STATUS_COMMUNICATION_ERROR, I2C_RECEIVE_DONE));
#endif
            }
            else if (i2c_event == I2C_STATE_SLAVE_WRITE)
            {
#ifdef I2C_SLAVE_SUPPORT
                I2C_status[ch].slave.buffWriteSize = 0;

                EXECUTE_FUNC(I2C_status[ch].slave.event, (ch, address, info,
                             DEFS_STATUS_COMMUNICATION_ERROR, I2C_SENT_DONE));
#endif
            }
            else
            {
#ifdef I2C_MASTER_SUPPORT
#ifdef I2C_HANDLE_TIMEOUT
                if (I2C_status[ch].autoTimeout)
                {
                    I2C_ConfigTimeout(ch, FALSE, 0);
                }
#endif
                EXECUTE_FUNC(I2C_status[ch].master.event, (ch, address, info,
                             DEFS_STATUS_COMMUNICATION_ERROR, I2C_UNDEFINED_EVENT));
#endif
            }

            break;

        case SMB_NO_DATA_IND:
        case SMB_WAKE_UP_IND:
        default:

            break;
    }
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Init                                                                               */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes I2C module                                                    */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Init (void)
{
    I2C_CHANNEL_T ch;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Register SMB callback                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_Init(I2C_SMBCallback_L);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear local variables                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    for (ch = 0; ch < I2C_NUM_OF_CHANNELS; ++ch)
    {
        I2C_status[ch].state                = I2C_STATE_DISABLE;

#ifdef I2C_HANDLE_TIMEOUT
        I2C_status[ch].autoTimeout          = TRUE;
#endif

#ifdef I2C_MASTER_SUPPORT
        I2C_status[ch].master.event         = NULL;
        I2C_status[ch].master.buffWrite     = NULL;
        I2C_status[ch].master.buffRead      = NULL;
        I2C_status[ch].master.buffWriteSize = 0;
        I2C_status[ch].master.buffReadSize  = 0;
#endif

#ifdef I2C_SLAVE_SUPPORT
        I2C_status[ch].slave.event          = NULL;
        I2C_status[ch].slave.buffWrite      = NULL;
        I2C_status[ch].slave.buffRead       = NULL;
        I2C_status[ch].slave.buffWriteSize  = 0;
        I2C_status[ch].slave.buffReadSize   = 0;
#endif
    }

    return DEFS_STATUS_OK;
}


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
BOOLEAN I2C_StateIsBusy (I2C_CHANNEL_T ch)
{
    return !((I2C_status[ch].state == I2C_STATE_DISABLE) || (I2C_status[ch].state == I2C_STATE_IDLE));
}


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
BOOLEAN I2C_ModuleIsBusy (I2C_CHANNEL_T ch)
{
    return SMB_ModuleIsBusy((SMB_MODULE_T)ch);
}


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
BOOLEAN I2C_BusIsBusy (I2C_CHANNEL_T ch)
{
    return SMB_BusIsBusy((SMB_MODULE_T)ch);
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_InterruptIsPending                                                                 */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         BOOLEAN                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if we have pending interrupt                                       */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN I2C_InterruptIsPending (void)
{
    return SMB_InterruptIsPending();
}


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            MASTER FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


#ifdef I2C_MASTER_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_Init                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch      - Channel number                                                               */
/*                  freq    - Channel frequency (in HZ)                                                    */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes master channel                                                */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Master_Init (I2C_CHANNEL_T ch, UINT32 freq)
{
    UINT32 f;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK(((I2C_status[ch].state == I2C_STATE_DISABLE) ||
                            (I2C_status[ch].state == I2C_STATE_IDLE)), DEFS_STATUS_SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Frequency                                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    f = DIV_ROUND(freq , _1KHz_);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Initialize master module                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    if (!SMB_InitModule((SMB_MODULE_T)ch, (UINT16)f))
    {
        return DEFS_STATUS_FAIL;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Init state                                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    I2C_status[ch].master.event         = NULL;
    I2C_status[ch].master.buffWrite     = NULL;
    I2C_status[ch].master.buffWriteSize = 0;
    I2C_status[ch].master.buffRead      = NULL;
    I2C_status[ch].master.buffReadSize  = 0;

    I2C_status[ch].freq                 = freq;
    I2C_status[ch].state                = I2C_STATE_IDLE;

#ifdef I2C_HANDLE_TIMEOUT
    /*-------------------------------------------------------------------------------------------------*/
    /* The aproximate time for:                                                                        */
    /* 1 bytes is in 100KHz is ~ 90us, in 400KHz is ~ 23us , in 1MHz is ~ 9us                          */
    /* So the calculation is very close to 9,000,000 divided by the freq in Hz and we round it up.     */
    /* This is the calulation that we do below.                                                        */
    /*-------------------------------------------------------------------------------------------------*/
    I2C_status[ch].microsecPerByte = DIV_ROUND(9000000, I2C_status[ch].freq);
#endif

    return DEFS_STATUS_OK;
}


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
DEFS_STATUS I2C_Master_DeInit (I2C_CHANNEL_T ch)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Initialize master module                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_Disable((SMB_MODULE_T)ch);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Init state                                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    I2C_status[ch].master.event         = NULL;
    I2C_status[ch].master.buffWrite     = NULL;
    I2C_status[ch].master.buffWriteSize = 0;
    I2C_status[ch].master.buffRead      = NULL;
    I2C_status[ch].master.buffReadSize  = 0;

    I2C_status[ch].state                = I2C_STATE_DISABLE;

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS I2C_Master_ChangeFrequency (I2C_CHANNEL_T ch, UINT32 freq)
{
    if (I2C_status[ch].freq != freq)
    {
        (void)I2C_Master_DeInit(ch);
        return I2C_Master_Init(ch, freq);
    }

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS I2C_Master_WriteRead (I2C_CHANNEL_T         ch,
                                UINT8                   slaveAddr,
                                void*                   writeBuffer,
                                UINT16                  writeSize,
                                void*                   readBuffer,
                                UINT16                  readSize,
                                I2C_EVENT_FINISHED_T    readWriteFinished,
                                BOOLEAN                 use_PEC)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK((I2C_status[ch].state == I2C_STATE_IDLE), DEFS_STATUS_SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if SMBus is free                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK(SMB_ModuleIsBusy((SMB_MODULE_T)ch) == FALSE, DEFS_STATUS_SYSTEM_BUSY);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set state variables                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    I2C_status[ch].state    = (writeSize != 0 ) ? I2C_STATE_MASTER_WRITE : I2C_STATE_MASTER_READ;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Setting the event for the callback                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    I2C_status[ch].master.event    = readWriteFinished;

    /*-----------------------------------------------------------------------------------------------------*/
    /* save buffers                                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    I2C_status[ch].master.buffWrite        = writeBuffer;
    I2C_status[ch].master.buffWriteSize    = writeSize;
    I2C_status[ch].master.buffRead         = readBuffer;
    I2C_status[ch].master.buffReadSize     = readSize;

#ifdef I2C_HANDLE_TIMEOUT
    if (I2C_status[ch].autoTimeout)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Calulation transaction time + 25ms for SCL low timeout + 1m spare. Min time must be 35ms        */
        /*-------------------------------------------------------------------------------------------------*/
        UINT16 tr_time = (UINT16)(DIV_ROUND((writeSize + readSize) * I2C_status[ch].microsecPerByte, 1000) + 25 + 1 );
        tr_time = MAX(tr_time, 35);
        I2C_ConfigTimeout(ch, TRUE, tr_time);
    }
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Start transaction                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    if (SMB_StartMasterTransaction((SMB_MODULE_T)ch, slaveAddr, writeSize, readSize, writeBuffer, readBuffer, use_PEC))
    {
        return DEFS_STATUS_OK;
    }
    else
    {
        return DEFS_STATUS_FAIL;
    }
}


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
                                            BOOLEAN         use_PEC)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Start transaction                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_RET_CHECK(I2C_Master_WriteRead(ch, slaveAddr, writeBuffer, writeSize, readBuffer, readSize,
                                               I2C_SMBBlockingCallback_L, use_PEC));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait for transaction to finish, and for call back to run                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    while ( ((I2C_status[ch].state != I2C_STATE_IDLE) && (I2C_status[ch].state != I2C_STATE_DISABLE)) ) ;

    if (I2C_status[ch].state == I2C_STATE_DISABLE)
    {
        I2C_status[ch].blockingStatus = DEFS_STATUS_FAIL;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* in case of NACK : reset the state and return the proper error ind                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    return I2C_status[ch].blockingStatus;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Master_Recover                                                                     */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reinitialize the I2C module and recover SMBus.                            */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Master_Recover (I2C_CHANNEL_T ch)
{
    SMB_MasterAbort(ch);
    DEFS_STATUS_RET_CHECK(I2C_Master_DeInit(ch));
    DEFS_STATUS_RET_CHECK(I2C_Master_Init(ch, I2C_status[ch].freq));
    return DEFS_STATUS_OK;
}
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            SLAVE FUNCTIONS                                              */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

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
/*                  This routine initializes slave module                                                  */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_Init (I2C_CHANNEL_T ch, UINT32 freq)
{
    UINT32 f;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK(((I2C_status[ch].state == I2C_STATE_DISABLE) ||
                            (I2C_status[ch].state == I2C_STATE_IDLE)), DEFS_STATUS_SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Frequency                                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    f = DIV_ROUND(freq , _1KHz_);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Initialize Slave module                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (!SMB_InitModule((SMB_MODULE_T)ch, (UINT16)f))
    {
        return DEFS_STATUS_FAIL;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Init state                                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    I2C_status[ch].slave.event          = NULL;
    I2C_status[ch].slave.buffWrite      = NULL;
    I2C_status[ch].slave.buffWriteSize  = 0;
    I2C_status[ch].slave.buffRead       = NULL;
    I2C_status[ch].slave.buffReadSize   = 0;

    I2C_status[ch].freq                 = freq;
    I2C_status[ch].state                = I2C_STATE_IDLE;

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_DeInit                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ch    - Channel number                                                                 */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if succeeded                                                            */
/*                  DEFS_STATUS_SYSTEM_BUSY if the I2C is currently active                                 */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine disables the I2C in hardware as well as software                          */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_DeInit (I2C_CHANNEL_T ch)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Initialize slave module                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_Disable((SMB_MODULE_T)ch);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Init state                                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    I2C_status[ch].slave.event          = NULL;
    I2C_status[ch].slave.buffRead       = NULL;
    I2C_status[ch].slave.buffReadSize   = 0;
    I2C_status[ch].slave.buffWrite      = NULL;
    I2C_status[ch].slave.buffWriteSize  = 0;

    I2C_status[ch].state                = I2C_STATE_DISABLE;

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS I2C_Slave_Add (I2C_CHANNEL_T ch, UINT8 slaveAddr, BOOLEAN use_PEC)
{
    return SMB_AddSlaveAddress((SMB_MODULE_T)ch, slaveAddr, use_PEC);
}

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
DEFS_STATUS I2C_Slave_Remove (I2C_CHANNEL_T ch, UINT8 slaveAddr)
{
    return SMB_RemSlaveAddress((SMB_MODULE_T)ch, slaveAddr);
}

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
DEFS_STATUS I2C_Slave_Read (I2C_CHANNEL_T ch, void* buffer, UINT16 size, I2C_EVENT_FINISHED_T readFinished)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (size == 0)
    {
        return DEFS_STATUS_INVALID_PARAMETER;
    }

    if (buffer == NULL)
    {
        return DEFS_STATUS_INVALID_PARAMETER;
    }

    DEFS_STATUS_COND_CHECK((I2C_status[ch].state == I2C_STATE_IDLE), DEFS_STATUS_SYSTEM_BUSY);

    I2C_status[ch].slave.event          = readFinished;
    I2C_status[ch].slave.buffRead       = buffer;
    I2C_status[ch].slave.buffReadSize   = size;

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS I2C_Slave_ReadContinue (I2C_CHANNEL_T ch, void* buffer, UINT16 size, I2C_EVENT_FINISHED_T readFinished)
{
    if (readFinished != NULL)
    {
        I2C_status[ch].slave.event        = readFinished;
    }
    if (buffer != NULL)
    {
        I2C_status[ch].slave.buffRead     = buffer;
    }
    if (size != 0)
    {
        I2C_status[ch].slave.buffReadSize = size;
    }

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS I2C_Slave_Write (I2C_CHANNEL_T ch, void* buffer, UINT16 size, I2C_EVENT_FINISHED_T writeFinished)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (size == 0)
    {
        return DEFS_STATUS_INVALID_PARAMETER;
    }

    if (buffer == NULL)
    {
        return DEFS_STATUS_INVALID_PARAMETER;
    }

    DEFS_STATUS_COND_CHECK((I2C_status[ch].state != I2C_STATE_DISABLE), DEFS_STATUS_SYSTEM_IN_INCORRECT_STATE);

    I2C_status[ch].slave.event          = writeFinished;
    I2C_status[ch].slave.buffWrite      = buffer;
    I2C_status[ch].slave.buffWriteSize  = size;

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS I2C_Slave_ReadBlocking (I2C_CHANNEL_T ch, void* buffer, UINT16 size)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Start read                                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_RET_CHECK(I2C_Slave_Read(ch, buffer, size, NULL));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait till finished                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    while (I2C_status[ch].slave.buffReadSize != 0) ;

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS I2C_Slave_WriteBlocking (I2C_CHANNEL_T ch, void* buffer, UINT16 size)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Start write                                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_RET_CHECK(I2C_Slave_Write(ch, buffer, size, NULL));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait till finished                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    while (I2C_status[ch].slave.buffWriteSize != 0) ;

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_Slave_Recover                                                                      */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reinitialize the I2C module and recover SMBus.                            */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS I2C_Slave_Recover (I2C_CHANNEL_T ch)
{
    DEFS_STATUS_RET_CHECK(I2C_Slave_DeInit(ch));
    DEFS_STATUS_RET_CHECK(I2C_Slave_Init(ch, I2C_status[ch].freq));
    return DEFS_STATUS_OK;
}
#endif

#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_ConfigTimeout                                                                      */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/*                  ch              - Channel number.                                                      */
/*                  scl_low_timeout - TRUE: Enable 25ms SCL suck low timeout; FALSE: Disable this timeout  */
/*                  frame_timeout   - configures the "Strat To Stop" timeout. Can be any value between     */
/*                                    35 to 1011. Other values will disable this timeout.                  */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module variables                                               */
/*lint -e{715}      Suppress 'frame_timeout' not referenced                                                */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_ConfigTimeout (I2C_CHANNEL_T ch, BOOLEAN scl_low_timeout , UINT16 frame_timeout)
{
    SMB_EnableTimeout(ch, scl_low_timeout);
#ifdef SMB_CAPABILITY_START_TO_STOP_TIMEOUT_SUPPORT
    SMB_EnableStartToStopTimeout(ch, frame_timeout);
#endif
}
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
void I2C_EnableTimeout (I2C_CHANNEL_T ch, BOOLEAN enable)
{
    I2C_status[ch].autoTimeout = enable;
}
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
void I2C_PrintVars (void)
{
    UINT i;

    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     I2C      */\n");
    HAL_PRINT("/*--------------*/\n\n");

    for (i = 0; i < I2C_NUM_OF_CHANNELS; i++)
    {
        I2C_PrintModuleVars((I2C_CHANNEL_T)i);
    }
}

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
/*lint -e{715}      Suppress 'channel_num' not referenced                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_PrintModuleVars (I2C_CHANNEL_T channel_num)
{
    ASSERT(channel_num < I2C_NUM_OF_CHANNELS);

    HAL_PRINT("I2C%d:\n", channel_num);
    HAL_PRINT("-----\n");

#ifdef I2C_MASTER_SUPPORT
    HAL_PRINT("master.buffWriteSize    = %d\n",       I2C_status[channel_num].master.buffWriteSize);
    HAL_PRINT("master.buffReadSize     = %d\n",       I2C_status[channel_num].master.buffReadSize);
#endif
#ifdef I2C_SLAVE_SUPPORT
    HAL_PRINT("slave.buffWriteSize     = %d\n",       I2C_status[channel_num].slave.buffWriteSize);
    HAL_PRINT("slave.buffReadSize      = %d\n",       I2C_status[channel_num].slave.buffReadSize);
#endif
    HAL_PRINT("state                   = %s\n",       I2C_StateStr(I2C_status[channel_num].state));
    HAL_PRINT("freq                    = %d Hz\n",    I2C_status[channel_num].freq);
    HAL_PRINT("blockingStatus          = 0x%02X\n",   I2C_status[channel_num].blockingStatus);

    HAL_PRINT("\n");
}


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#ifdef HAL_PRINT_CAPABILITY
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        I2C_StateStr                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  stateType - The enumerator type to be converted to string                              */
/*                                                                                                         */
/* Returns:         A string of stateType                                                                  */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine retrieves the string which represents the given I2C_STATE_T enumerator    */
/*---------------------------------------------------------------------------------------------------------*/
static const char* I2C_StateStr (I2C_STATE_T stateType)
{
    switch (stateType)
    {
    case    I2C_STATE_ERROR:            return "I2C_STATE_ERROR";
    case    I2C_STATE_DISABLE:          return "I2C_STATE_DISABLE";
    case    I2C_STATE_IDLE:             return "I2C_STATE_IDLE";
    case    I2C_STATE_MASTER_READ:      return "I2C_STATE_MASTER_READ";
    case    I2C_STATE_MASTER_WRITE:     return "I2C_STATE_MASTER_WRITE";
    case    I2C_STATE_SLAVE_READ:       return "I2C_STATE_SLAVE_READ";
    case    I2C_STATE_SLAVE_WRITE:      return "I2C_STATE_SLAVE_WRITE";
    case    I2C_STATE_SLAVE_RESTART:    return "I2C_STATE_SLAVE_RESTART";
    default:                            return "";
    }
}
#endif

