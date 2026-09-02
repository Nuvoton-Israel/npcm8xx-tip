/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2010-2019 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   smb_drv.c                                                                                             */
/*            This file contains System Management Bus (SMB) driver implementation                         */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_DRV()

#include "smb_drv.h"
#include "smb_regs.h"
#include <string.h>

/*---------------------------------------------------------------------------------------------------------*/
/* Module Dependencies                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#if defined (MIWU_MODULE_TYPE) && defined (SMB_CAPABILITY_WAKEUP_SUPPORT)
#include __MODULE_IF_HEADER_FROM_DRV(miwu)
#endif

#ifndef SMB_SW_BYPASS_HW_ISSUE_SMB_STOP
#define SMB_SW_BYPASS_HW_ISSUE_SMB_STOP
#define SMB_DELAY(microSecDelay, t0Time)    DELAY_USEC(microSecDelay, HFCG_CORE_CLK)
#define SMB_GET_TIME(time)
#else
#if defined (CLK_MODULE_TYPE)
#include __MODULE_IF_HEADER_FROM_DRV(clk)
#else
extern void CLK_Delay_Since  (UINT32 microSecDelay, UINT32 t0_time[2]);
extern void CLK_GetTimeStamp (UINT32 time[2]);
#endif
#define SMB_DELAY(microSecDelay, t0Time)    CLK_Delay_Since(microSecDelay, t0Time)
#define SMB_GET_TIME(time)                  CLK_GetTimeStamp(time)
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#ifdef SMB_SLAVE_ONLY
    /*-----------------------------------------------------------------------------------------------------*/
    /* Stall Timeout feature is only relevant in Master mode.                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    #undef SMB_STALL_TIMEOUT_SUPPORT
#endif

#ifdef SMB_STALL_TIMEOUT_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* stall/stuck timeout                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define DEFAULT_STALL_COUNT         25
#define SMB_RESET_STALL_COUNTER(n)  (SMB_status[n].stall_counter = 0)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* SMBus PEC                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef SMB_CAPABILITY_HW_PEC_SUPPORT
#define SMBUS_PEC_BYTE(n)           REG_READ(SMBPEC(n))
#else
#define SMBUS_PEC_BYTE(n)           SMB_status[n].crc_data
#endif
#define SMBUS_PEC_IS_CORRECT(n)     (SMB_status[n].PEC_use == FALSE || SMBUS_PEC_BYTE(n) == 0)
#define SMBUS_SEND_PEC_BYTE(n)      (REG_WRITE(SMBSDA(n), SMBUS_PEC_BYTE(n)))

/*---------------------------------------------------------------------------------------------------------*/
/* Data abort timeout                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define ABORT_TIMEOUT       1000

/*---------------------------------------------------------------------------------------------------------*/
/* SMBus spec. values in KHz                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define SMBUS_FREQ_MIN      10

#if defined SMB_CAPABILITY_FAST_MODE_PLUS_SUPPORT
#define SMBUS_FREQ_MAX      1000
#elif defined SMB_CAPABILITY_FAST_MODE_SUPPORT
#define SMBUS_FREQ_MAX      400
#else
#define SMBUS_FREQ_MAX      100
#endif

#define SMBUS_FREQ_100KHz   100
#define SMBUS_FREQ_400KHz   400
#define SMBUS_FREQ_1MHz     1000

/*---------------------------------------------------------------------------------------------------------*/
/* SMBus FIFO SIZE (when FIFO hardware exist)                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef SMB_CAPABILITY_32B_FIFO
#define SMBUS_FIFO_SIZE     32
#else
#define SMBUS_FIFO_SIZE     16
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* SCLFRQ min/max field values                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define SCLFRQ_MIN          8
#define SCLFRQ_MAX          511

/*---------------------------------------------------------------------------------------------------------*/
/* SCLFRQ field position                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define SCLFRQ_0_TO_6       0,  7
#define SCLFRQ_7_TO_8       7,  2

/*---------------------------------------------------------------------------------------------------------*/
/* SMB Maximum Retry Trials (on Bus Arbitration Loss)                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define SMB_RETRY_MAX_COUNT 3

/*---------------------------------------------------------------------------------------------------------*/
/* SMBus Operation type values                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    SMB_NO_OPER     = 0,
    SMB_WRITE_OPER  = 1,
    SMB_READ_OPER   = 2
} SMB_OPERATION_T;

/*---------------------------------------------------------------------------------------------------------*/
/* SMBus Bank (FIFO mode)                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    SMB_BANK_0  = 0,
    SMB_BANK_1  = 1
} SMB_BANK_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Internal SMBus Interface driver states values, which reflect events which occurred on the bus           */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    SMB_DISABLE,
    SMB_IDLE,
    SMB_MASTER_START,
    SMB_SLAVE_MATCH,
    SMB_OPER_STARTED,
    SMB_REPEATED_START,
    SMB_STOP_PENDING
} SMB_OPERATION_STATE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Status of one SMBus module                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct
{
    /* Current state of SMBus */
    volatile SMB_OPERATION_STATE_T  operation_state;

    /* Type of the last SMBus operation */
    SMB_OPERATION_T        operation;

    /* Mode of operation on SMBus */
    SMB_MODE_T             master_or_slave;
#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
    /* The indication to the hi level after Master Stop */
    SMB_STATE_IND_T        stop_indication;
#endif
    /* SMBus slave device's Slave Address in 8-bit format - for master transactions */
    UINT8                  dest_addr;

    /* Buffer where read data should be placed */
    UINT8*                 read_data_buf;

    /* Number of bytes to be read */
    UINT16                 read_size;

    /* Number of bytes already read */
    UINT16                 read_index;

    /* Buffer with data to be written */
    UINT8*                 write_data_buf;

    /* Number of bytes to write */
    UINT16                 write_size;

    /* Number of bytes already written */
    UINT16                 write_index;

    /* use fifo hardware or not */
    BOOLEAN                fifo_use;

    /* fifo threshold size */
    UINT8                  threshold_fifo;

    /* PEC bit mask per slave address.
       1: use PEC for this address,
       0: do not use PEC for this address */
    UINT16                PEC_mask;

    /* Use PEC CRC  */
    BOOLEAN                PEC_use;

    /* PEC CRC data */
    UINT8                  crc_data;

    /* Use read block */
    BOOLEAN                read_block_use;

    /* Number of retries remaining */
    UINT8                  retry_count;

#ifdef SMB_STALL_TIMEOUT_SUPPORT
    UINT8                  stall_counter;
    UINT8                  stall_threshold;
#endif

#ifdef SMB_SW_BYPASS_HW_ISSUE_SMB_STOP
    /*-----------------------------------------------------------------------------------------------------*/
    /* override issue #614: CP_FW: SMBus may fail to supply stop condition in Master Write operation.      */
    /* If needed : define it at hal_cfg.h                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    /* The indication to the hi level after Master Stop */
    UINT32                clk_period_us;
    UINT32                interrupt_time_stamp[2];
#endif
} SMB_STATUS_T;

/*---------------------------------------------------------------------------------------------------------*/
/* SMBus Timing                                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct
{
    UINT32  core_clk;
    UINT8   hldt;
    UINT8   dbcnt;
    UINT16  sclfrq;
    UINT8   scllt;
    UINT8   sclht;
    BOOLEAN fastMode;
} SMB_TIMING_T;


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            GLOBAL VARIABLES                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Current status of the SMBus interface driver                                                            */
/*---------------------------------------------------------------------------------------------------------*/
static SMB_STATUS_T SMB_status[SMB_NUM_OF_MODULES];

#ifdef SMB_SLAVE_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Current address of the SMBus interface driver                                                           */
/*---------------------------------------------------------------------------------------------------------*/
static UINT8 SMB_CurSlaveAddr[SMB_NUM_OF_MODULES];
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Callback function provided by next-higher level driver or application, implementing operation handling  */
/* state-machine                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
static SMB_CALLBACK_T SMB_callback;

#ifndef SMB_CAPABILITY_HW_PEC_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* CRC8 Table                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
static const UINT8 SMB_crc8_table[256] =
{
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
    0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
    0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* SMBus Timing                                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
static const SMB_TIMING_T SMB_TIMING_100KHZ[] =
{
    {.core_clk = 100000000, .hldt = 0x11, .dbcnt = 0x4, .sclfrq = 0xFB, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 62500000,  .hldt = 0x11, .dbcnt = 0x1, .sclfrq = 0x9D, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 60000000,  .hldt = 0x11, .dbcnt = 0x1, .sclfrq = 0x98, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 50000000,  .hldt = 0x11, .dbcnt = 0x1, .sclfrq = 0x7E, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 48000000,  .hldt = 0x11, .dbcnt = 0x1, .sclfrq = 0x79, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 40000000,  .hldt = 0x11, .dbcnt = 0x1, .sclfrq = 0x65, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 30000000,  .hldt = 0x11, .dbcnt = 0x1, .sclfrq = 0x4C, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 29000000,  .hldt = 0x11, .dbcnt = 0x1, .sclfrq = 0x49, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 26000000,  .hldt = 0x11, .dbcnt = 0x1, .sclfrq = 0x42, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 25000000,  .hldt = 0xF,  .dbcnt = 0x1, .sclfrq = 0x3F, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 24000000,  .hldt = 0xF,  .dbcnt = 0x1, .sclfrq = 0x3D, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 20000000,  .hldt = 0xF,  .dbcnt = 0x1, .sclfrq = 0x33, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 16180000,  .hldt = 0xF,  .dbcnt = 0x1, .sclfrq = 0x29, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 15000000,  .hldt = 0xF,  .dbcnt = 0x1, .sclfrq = 0x26, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 13000000,  .hldt = 0xF,  .dbcnt = 0x1, .sclfrq = 0x21, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 12000000,  .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x1F, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 10000000,  .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x1A, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 9000000,   .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x17, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 8090000,   .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x15, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 7500000,   .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x13, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 6500000,   .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x11, .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE },
    {.core_clk = 4000000,   .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0xB,  .scllt = 0x0, .sclht  = 0x0, .fastMode = FALSE }
};

#if defined (SMB_CAPABILITY_FAST_MODE_SUPPORT) || defined (SMB_CAPABILITY_FAST_MODE_PLUS_SUPPORT)
static const SMB_TIMING_T SMB_TIMING_400KHZ[] =
{
    {.core_clk = 100000000, .hldt = 0x2A, .dbcnt = 0x3, .sclfrq = 0x0, .scllt = 0x47, .sclht  = 0x35, .fastMode = TRUE },
    {.core_clk = 62500000,  .hldt = 0x2A, .dbcnt = 0x2, .sclfrq = 0x0, .scllt = 0x2C, .sclht  = 0x22, .fastMode = TRUE },
    {.core_clk = 60000000,  .hldt = 0x26, .dbcnt = 0x2, .sclfrq = 0x0, .scllt = 0x28, .sclht  = 0x24, .fastMode = TRUE },
    {.core_clk = 50000000,  .hldt = 0x21, .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x24, .sclht  = 0x1B, .fastMode = TRUE },
    {.core_clk = 48000000,  .hldt = 0x1E, .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x24, .sclht  = 0x19, .fastMode = TRUE },
    {.core_clk = 40000000,  .hldt = 0x1B, .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x1E, .sclht  = 0x14, .fastMode = TRUE },
    {.core_clk = 33000000,  .hldt = 0x15, .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x19, .sclht  = 0x11, .fastMode = TRUE },
    {.core_clk = 30000000,  .hldt = 0x15, .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x19, .sclht  = 0xD,  .fastMode = TRUE },
    {.core_clk = 29000000,  .hldt = 0x11, .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x15, .sclht  = 0x10, .fastMode = TRUE },
    {.core_clk = 26000000,  .hldt = 0x10, .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x13, .sclht  = 0xE,  .fastMode = TRUE },
    {.core_clk = 25000000,  .hldt = 0xF,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x13, .sclht  = 0xD,  .fastMode = TRUE },
    {.core_clk = 24000000,  .hldt = 0xD,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x12, .sclht  = 0xD,  .fastMode = TRUE },
    {.core_clk = 20000000,  .hldt = 0xB,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0xF,  .sclht  = 0xA,  .fastMode = TRUE },
    {.core_clk = 16180000,  .hldt = 0xA,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0xC,  .sclht  = 0x9,  .fastMode = TRUE },
    {.core_clk = 15000000,  .hldt = 0x9,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0xB,  .sclht  = 0x8,  .fastMode = TRUE },
    {.core_clk = 13000000,  .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0xA,  .sclht  = 0x7,  .fastMode = TRUE },
    {.core_clk = 12000000,  .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0xA,  .sclht  = 0x6,  .fastMode = TRUE },
    {.core_clk = 10000000,  .hldt = 0x6,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x8,  .sclht  = 0x5,  .fastMode = TRUE },
};
#endif

#if defined (SMB_CAPABILITY_FAST_MODE_PLUS_SUPPORT)
static const SMB_TIMING_T SMB_TIMING_1000KHZ[] =
{
    {.core_clk = 100000000, .hldt = 0x15, .dbcnt = 0x4, .sclfrq = 0x0, .scllt = 0x1C, .sclht  = 0x15, .fastMode = TRUE },
    {.core_clk = 62500000,  .hldt = 0xF,  .dbcnt = 0x3, .sclfrq = 0x0, .scllt = 0x11, .sclht  = 0xE,  .fastMode = TRUE },
    {.core_clk = 60000000,  .hldt = 0x12, .dbcnt = 0x2, .sclfrq = 0x0, .scllt = 0xF,  .sclht  = 0xE,  .fastMode = TRUE },
    {.core_clk = 50000000,  .hldt = 0xA,  .dbcnt = 0x2, .sclfrq = 0x0, .scllt = 0xE,  .sclht  = 0xB,  .fastMode = TRUE },
    {.core_clk = 48000000,  .hldt = 0x9,  .dbcnt = 0x2, .sclfrq = 0x0, .scllt = 0xD,  .sclht  = 0xB,  .fastMode = TRUE },
    {.core_clk = 41000000,  .hldt = 0x9,  .dbcnt = 0x2, .sclfrq = 0x0, .scllt = 0xC,  .sclht  = 0x9,  .fastMode = TRUE },
    {.core_clk = 40000000,  .hldt = 0x8,  .dbcnt = 0x2, .sclfrq = 0x0, .scllt = 0xB,  .sclht  = 0x9,  .fastMode = TRUE },
    {.core_clk = 33000000,  .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0xA,  .sclht  = 0x7,  .fastMode = TRUE },
    {.core_clk = 30000000,  .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0xA,  .sclht  = 0x7,  .fastMode = TRUE },
    {.core_clk = 25000000,  .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x7,  .sclht  = 0x6,  .fastMode = TRUE },
    {.core_clk = 24000000,  .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x8,  .sclht  = 0x5,  .fastMode = TRUE },
    {.core_clk = 20000000,  .hldt = 0x7,  .dbcnt = 0x1, .sclfrq = 0x0, .scllt = 0x6,  .sclht  = 0x4,  .fastMode = TRUE },
};
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                  LOCAL FUNCTIONS FORWARD DECLARATIONS                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ void        SMB_WriteByte               (SMB_MODULE_T module_num, UINT8 data);
static _INLINE_ BOOLEAN     SMB_ReadByte                (SMB_MODULE_T module_num, UINT8 *data);
static _INLINE_ void        SMB_SelectBank              (SMB_MODULE_T module_num, SMB_BANK_T bank);
static _INLINE_ UINT16      SMB_GetIndex                (SMB_MODULE_T module_num);
#ifdef SMB_MASTER_SUPPORT
static _INLINE_ void        SMB_Start                   (SMB_MODULE_T module_num);
static _INLINE_ void        SMB_Stop                    (SMB_MODULE_T module_num);
static _INLINE_ void        SMB_AbortData               (SMB_MODULE_T module_num);
static _INLINE_ void        SMB_StallAfterStart         (SMB_MODULE_T module_num, BOOLEAN stall);
static _INLINE_ void        SMB_Nack                    (SMB_MODULE_T module_num);
static          void        SMB_MasterFifoRead          (SMB_MODULE_T module_num);
#endif
#ifdef SMB_SLAVE_SUPPORT
static          DEFS_STATUS SMB_SlaveEnable_l           (SMB_MODULE_T module_num, SMB_ADDR_T addr_type, UINT8 addr, BOOLEAN enable);
static          UINT8       SMB_GetSlaveAddress_l       (SMB_MODULE_T module_num, SMB_ADDR_T addrEnum);
#endif
static          BOOLEAN     SMB_Enable                  (SMB_MODULE_T module_num);
#ifndef SMB_TEST_MODE
static
#endif
                void        SMB_Reset                   (SMB_MODULE_T module_num);
#ifndef SMB_TEST_MODE
static
#endif
                void        SMB_InterruptEnable         (SMB_MODULE_T module_num, BOOLEAN enable);
#if defined (MIWU_MODULE_TYPE) && defined (SMB_CAPABILITY_WAKEUP_SUPPORT)
static          void        SMB_WakeupHandler           (MIWU_SRC_T source);
#endif
static          BOOLEAN     SMB_InitClock               (SMB_MODULE_T module_num, UINT16 bus_freq);
static          void        SMB_ReadFromFifo            (SMB_MODULE_T module_num, UINT8 bytes_in_fifo);
static          void        SMB_WriteToFifo             (SMB_MODULE_T module_num, UINT16 max_bytes_to_send);
#ifndef SMB_CAPABILITY_HW_PEC_SUPPORT
static          UINT8       SMB_CalculateCRC8           (UINT8 crc_data, UINT8 data);
#endif
static          void        SMB_CalcPEC                 (SMB_MODULE_T module_num, UINT8 data);
static          void        SMBF_PrintModuleRegs        (SMB_MODULE_T module_num, SMB_MODULE_T smbf_module);
#ifdef HAL_PRINT_CAPABILITY
static const    char*       SMB_OperationStateStr       (SMB_OPERATION_STATE_T operatioStateType);
static const    char*       SMB_OperationStr            (SMB_OPERATION_T operationType);
static const    char*       SMB_ModeStr                 (SMB_MODE_T modeType);
#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
static const    char*       SMB_StateIndStr             (SMB_STATE_IND_T stateIndType);
#endif
#endif
/*---------------------------------------------------------------------------------------------------------*/
/* SMB Interrupt Handlers                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
static          void        SMB_InterruptHandler        (SMB_MODULE_T module_num);
static          void        SMB_HandleNegativeAck       (SMB_MODULE_T module_num);
static          void        SMB_HandleBusError          (SMB_MODULE_T module_num);
#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
static          void        SMB_HandleTimeout           (SMB_MODULE_T module_num);
#endif
#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
static          void        SMB_HandleEndOfBusy         (SMB_MODULE_T module_num);
#endif
#ifdef SMB_SLAVE_SUPPORT
#ifdef SMB_RECOVERY_SUPPORT
static          void        SMB_SlaveAbort              (SMB_MODULE_T module_num);
#endif
static          void        SMB_HandleSlaveStop         (SMB_MODULE_T module_num);
static          void        SMB_HandleSlaveRestart      (SMB_MODULE_T module_num);
static          BOOLEAN     SMB_HandleSlaveAddressMatch (SMB_MODULE_T module_num);
static          void        SMB_HandleSlaveRead         (SMB_MODULE_T module_num);
static          void        SMB_HandleSlaveWrite        (SMB_MODULE_T module_num);
#endif
#ifdef SMB_MASTER_SUPPORT
static          void        SMB_HandleStallAfterStart   (SMB_MODULE_T module_num);
static          void        SMB_HandleMasterStart       (SMB_MODULE_T module_num);
static          void        SMB_HandleMasterRead        (SMB_MODULE_T module_num);
static          void        SMB_HandleMasterWrite       (SMB_MODULE_T module_num);
#endif


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
void SMB_Init (SMB_CALLBACK_T operation_done)
{
    SMB_MODULE_T module_num;

    ASSERT(operation_done != NULL);
    SMB_callback = operation_done;

    for (module_num = 0; module_num < SMB_NUM_OF_MODULES; module_num++)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Initialize the internal data structures                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_status[module_num].operation_state  = SMB_DISABLE;
        SMB_status[module_num].master_or_slave  = SMB_SLAVE;
#ifdef SMB_STALL_TIMEOUT_SUPPORT
        SMB_status[module_num].stall_counter    = 0;
        SMB_status[module_num].stall_threshold  = DEFAULT_STALL_COUNT;
#endif
#ifdef SMB_SLAVE_SUPPORT
        SMB_CurSlaveAddr[module_num]            = 0;
#endif

        /*-------------------------------------------------------------------------------------------------*/
        /* Interrupt configuration                                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        INTERRUPT_REGISTER_AND_ENABLE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), SMB_IntHandler,
                                      SMB_INTERRUPT_POLARITY, SMB_INTERRUPT_PRIORITY);

		NVIC_InstallSwHandler (SMB_INTERRUPT(module_num), SMB_IntHandler);
    }
}

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
BOOLEAN SMB_InitModule (
    SMB_MODULE_T    module_num,
    UINT16          bus_freq
)
{
    BOOLEAN intr;
    UINT    i;

    ASSERT(module_num < SMB_NUM_OF_MODULES);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check whether module already enabled or frequency is out of bounds                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    if (((SMB_status[module_num].operation_state != SMB_DISABLE) &&
         (SMB_status[module_num].operation_state != SMB_IDLE)) ||
        (bus_freq < SMBUS_FREQ_MIN) || (bus_freq > SMBUS_FREQ_MAX))
    {
        return FALSE;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Mux SMB module pins                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_MUX(module_num);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure FIFO mode                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    //lint -e{774, 506} suppress PC-Lint warning on 'Boolean within 'left side of if' always evaluates to True'
    if (SMB_FIFO(module_num) && READ_REG_FIELD(SMB_VER(module_num), SMB_VER_FIFO_EN))
    {
        SMB_status[module_num].fifo_use = TRUE;
        SMB_status[module_num].threshold_fifo = SMBUS_FIFO_SIZE;
        SET_REG_FIELD(SMBFIF_CTL(module_num), SMBFIF_CTL_FIFO_EN, 1);
    }
    else
    {
        SMB_status[module_num].fifo_use = FALSE;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure SMB module clock frequency                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    if (!SMB_InitClock(module_num, bus_freq))
    {
        return FALSE;
    }

    INTERRUPT_SAVE_DISABLE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);
    SMB_SelectBank(module_num, SMB_BANK_0); // select bank 0 for SMB addresses

    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure slave addresses (by default they are disabled)                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    for (i = 0; i < SMB_NUM_OF_ADDR; i++)
    {
        REG_WRITE(SMBADDR(module_num, i), 0);
    }

    SMB_SelectBank(module_num, SMB_BANK_1); // by default most access is in bank 1
    INTERRUPT_RESTORE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable module - before configuring CTL1 !                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    if (!SMB_Enable(module_num))
    {
        return FALSE;
    }
    else
    {
        SMB_status[module_num].operation_state = SMB_IDLE;
    }

#ifdef SMB_SW_BYPASS_HW_ISSUE_SMB_STOP
    /*-----------------------------------------------------------------------------------------------------*/
    /* Keep the period time for SMB_Stop half cycle delay                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_status[module_num].clk_period_us = 1000 / bus_freq; //  bus_freq is in kHz
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable SMB interrupt and New Address Match interrupt source                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_NMINTE, ENABLE);
    SMB_InterruptEnable(module_num, TRUE);

    return TRUE;
}

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
void SMB_Disable (SMB_MODULE_T module_num)
{
    UINT i;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Slave Addresses Removal                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    for (i = SMB_SLAVE_ADDR1; i < SMB_NUM_OF_ADDR; i++)
    {
        REG_WRITE(SMBADDR(module_num,i),0);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable module.                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL2(module_num), SMBCTL2_ENABLE, DISABLE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set module disable                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_status[module_num].operation_state = SMB_DISABLE;
}

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
BOOLEAN SMB_ModuleIsBusy (SMB_MODULE_T module_num)
{
    return (READ_REG_FIELD(SMBCST(module_num), SMBCST_BUSY) ||
            READ_REG_FIELD(SMBST(module_num), SMBST_SLVSTP));
}

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
BOOLEAN SMB_BusIsBusy (SMB_MODULE_T module_num)
{
    return (READ_REG_FIELD(SMBCST(module_num), SMBCST_BB));
}

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
void SMB_IntHandler (UINT16 int_num)
{
    int_num = SCS_GetActiveVectorNumber () - NVIC_TRAP_NUM;

    SMB_MODULE_T module = SMB_INTERRUPT_MODULE(int_num);

#ifdef SMB_SW_BYPASS_HW_ISSUE_SMB_STOP
    SMB_GET_TIME(SMB_status[module].interrupt_time_stamp);
#endif
    SMB_InterruptHandler(module);
}

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
void SMB_WakeupEnable (
    SMB_MODULE_T    module_num,
    BOOLEAN         enable
)
{
    const MIWU_SRC_T SMbusToMiwu[]  = SMB_WAKEUP_SRC;
    MIWU_SRC_T       miwu_src       = SMbusToMiwu[module_num];

    if (enable)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Configure MIWU module to generate an interrupt to the ICU following SMBus wake-up conditions    */
        /*-------------------------------------------------------------------------------------------------*/
        MIWU_Config(miwu_src, MIWU_RISING_EDGE, SMB_WakeupHandler);

        /*-------------------------------------------------------------------------------------------------*/
        /* Configure SMBus Wake-up (in System Glue Function)                                               */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(SMB_SBD, MASK_BIT(module_num));                   /* Clear Start condition detection     */
        SET_REG_BIT(SMB_EEN, module_num);                           /* Enable Event assertion              */
        SET_REG_FIELD(SMBCTL3(module_num), SMBCTL3_IDL_START, 1);   /* Enable start detect in IDLE         */
    }
    else
    {
        /*-------------------------------------------------------------------------------------------------*/
        /*  Disable SMB Wake-Up indication via MIWU                                                        */
        /*-------------------------------------------------------------------------------------------------*/
        MIWU_EnableChannel(miwu_src, FALSE);

        /*-------------------------------------------------------------------------------------------------*/
        /* Disable SMBus Wake-up (in System Glue Function)                                                 */
        /*-------------------------------------------------------------------------------------------------*/
        CLEAR_REG_BIT(SMB_EEN, module_num);                         /* Disable Event assertion             */
        SET_REG_FIELD(SMBCTL3(module_num), SMBCTL3_IDL_START, 0);   /* Disable start detect in IDLE        */
    }
}
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
BOOLEAN SMB_StartMasterTransaction (
    SMB_MODULE_T    module_num,
    UINT8           slave_addr,
    UINT16          nwrite,
    UINT16          nread,
    UINT8*          write_data,
    UINT8*          read_data,
    BOOLEAN         use_PEC
)
{
    UINT volatile intr = 0;

    ASSERT(module_num < SMB_NUM_OF_MODULES);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Allow only if bus is not busy                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((SMB_status[module_num].operation_state != SMB_IDLE)
#ifdef SMBUS_SIZE_CHECK
        ||
        ((nwrite >= _32KB_) && (nwrite != SMB_BYTES_QUICK_PROT)) ||
        ((nread >= _32KB_) && (nread != SMB_BYTES_BLOCK_PROT) && (nread != SMB_BYTES_QUICK_PROT))
#endif
        )
    {
        return FALSE;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable all interrupts to prevent task switching in the middle of staring a transaction.            */
    /* If a task switch happens between setting operation_state and calling SMB_Start(), the transaction   */
    /* will not start, however the 25ms countdown in SMB_StallHandler() will start counting and might      */
    /* abort the transaction before it even started (assuming the current task will not get back the CPU   */
    /* for more than 25ms)                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPTS_SAVE_DISABLE(intr);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Update driver state                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_status[module_num].master_or_slave = SMB_MASTER;
    SMB_status[module_num].operation_state = SMB_MASTER_START;
    if (nwrite > 0)
    {
        SMB_status[module_num].operation = SMB_WRITE_OPER;
    }
    else
    {
        SMB_status[module_num].operation = SMB_READ_OPER;
    }

    SMB_status[module_num].dest_addr      = (UINT8)(slave_addr << 1);  /* Translate 7-bit to 8-bit format  */
    SMB_status[module_num].write_data_buf = write_data;
    SMB_status[module_num].write_size     = ((nwrite == 0 && nread == 0) ? SMB_BYTES_QUICK_PROT : nwrite);
    SMB_status[module_num].write_index    = 0;
    SMB_status[module_num].read_data_buf  = read_data;
    SMB_status[module_num].read_size      = ((nwrite == 0 && nread == 0) ? SMB_BYTES_QUICK_PROT : nread);
    SMB_status[module_num].read_index     = 0;
    SMB_status[module_num].PEC_use        = use_PEC;
    SMB_status[module_num].read_block_use = FALSE;
    SMB_status[module_num].retry_count    = SMB_RETRY_MAX_COUNT;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if transaction uses Block read protocol                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((SMB_status[module_num].read_size == SMB_BYTES_BLOCK_PROT) ||
        (SMB_status[module_num].read_size == SMB_BYTES_EXCLUDE_BLOCK_SIZE_FROM_BUFFER))
    {
        SMB_status[module_num].read_block_use = TRUE;

        /*-------------------------------------------------------------------------------------------------*/
        /* Change nread in order to configure receive threshold to 1                                       */
        /*-------------------------------------------------------------------------------------------------*/
        nread = 1;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* clear BER just in case it is set due to a previous transaction                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_BER));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Initiate SMBus master transaction                                                                   */
    /* Generate a Start condition on the SMBus                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (SMB_status[module_num].fifo_use == TRUE)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* select bank 1 for FIFO registers                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_SelectBank(module_num, SMB_BANK_1);

        /*-------------------------------------------------------------------------------------------------*/
        /* clear FIFO and relevant status bits.                                                            */
        /*-------------------------------------------------------------------------------------------------*/
        SET_REG_MASK(SMBFIF_CTS(module_num), MASK_FIELD(SMBFIF_CTS_SLVRSTR)  |
                                             MASK_FIELD(SMBFIF_CTS_CLR_FIFO) |
                                             MASK_FIELD(SMBFIF_CTS_RXF_TXE));
        if (nwrite == 0)
        {
            if ((SMB_status[module_num].PEC_use == TRUE) &&
                (SMB_status[module_num].read_size < SMB_BYTES_EXCLUDE_BLOCK_SIZE_FROM_BUFFER))
            {
                /*-----------------------------------------------------------------------------------------*/
                /* PEC is used but the protocol isn't block read protocol - add extra byte for PEC support */
                /*-----------------------------------------------------------------------------------------*/
                SMB_status[module_num].read_size += 1;
                nread += 1;
            }

            /*---------------------------------------------------------------------------------------------*/
            /* This is a read only operation. Configure the FIFO                                           */
            /* threshold according to the needed number of bytes to read.                                  */
            /*---------------------------------------------------------------------------------------------*/
            if (nread > SMBUS_FIFO_SIZE)
            {
                SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR, SMBUS_FIFO_SIZE);
            }
            else
            {
                SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR, (UINT8)nread);

                if ((SMB_status[module_num].read_size != SMB_BYTES_BLOCK_PROT) &&
                    (SMB_status[module_num].read_size != SMB_BYTES_EXCLUDE_BLOCK_SIZE_FROM_BUFFER))
                {
                    SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_LAST_PEC, 1);
                }
            }
        }
    }
    SMB_Start(module_num);

    INTERRUPTS_RESTORE(intr);

    return TRUE;
}

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
void SMB_MasterAbort (SMB_MODULE_T module_num)
{
    SMB_AbortData(module_num);
    SMB_Reset(module_num);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_MasterAbortWithoutStop                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine cleans up the SMB interface and registers                                 */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_MasterAbortWithoutStop (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear NEGACK, STASTR and BER bits                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBST(module_num), (MASK_FIELD(SMBST_STASTR) |
                                  MASK_FIELD(SMBST_NEGACK) |
                                  MASK_FIELD(SMBST_BER)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear BB (BUS BUSY) bit                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBCST(module_num), MASK_FIELD(SMBCST_BB));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear EO_BUSY pending bit                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCST3(module_num), SMBCST3_EO_BUSY, 1);

    SMB_Reset(module_num);
}

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
void SMB_Recovery (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable interrupt                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_InterruptEnable(module_num, FALSE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check If the SDA line is active (low)                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    if (SMB_GetSDA(module_num) == SMB_LEVEL_LOW)
    {
        UINT8   iter = 9;   // Allow one byte to be sent by the Slave
        UINT16  timeout;
        BOOLEAN done = FALSE;

        /*-------------------------------------------------------------------------------------------------*/
        /* Repeat the following sequence until SDA becomes inactive (high)                                 */
        /*-------------------------------------------------------------------------------------------------*/
        do
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Issue a single SCL cycle                                                                    */
            /*---------------------------------------------------------------------------------------------*/
            REG_WRITE(SMBCST(module_num), MASK_FIELD(SMBCST_TGSCL));
            timeout = ABORT_TIMEOUT;
            while (READ_REG_FIELD(SMBCST(module_num), SMBCST_TGSCL) && (--timeout != 0)) ;

            /*---------------------------------------------------------------------------------------------*/
            /* If SDA line is inactive (high), stop                                                        */
            /*---------------------------------------------------------------------------------------------*/
            if (SMB_GetSDA(module_num) == SMB_LEVEL_HIGH)
            {
                done = TRUE;
            }
        }
        while ((done == FALSE) && (--iter != 0));

        /*-------------------------------------------------------------------------------------------------*/
        /* If SDA line is released (high)                                                                  */
        /*-------------------------------------------------------------------------------------------------*/
        if (done)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Clear BB (BUS BUSY) bit                                                                     */
            /*---------------------------------------------------------------------------------------------*/
            REG_WRITE(SMBCST(module_num), MASK_FIELD(SMBCST_BB));

            /*---------------------------------------------------------------------------------------------*/
            /* Generate a START condition, to synchronize Master and Slave                                 */
            /*---------------------------------------------------------------------------------------------*/
            SMB_Start(module_num);

            /*---------------------------------------------------------------------------------------------*/
            /* Wait until START condition is sent, or timeout                                              */
            /*---------------------------------------------------------------------------------------------*/
            timeout = ABORT_TIMEOUT;
            while (!READ_REG_FIELD(SMBST(module_num), SMBST_MASTER) && (--timeout != 0)) ;

            /*---------------------------------------------------------------------------------------------*/
            /* If START condition was sent                                                                 */
            /*---------------------------------------------------------------------------------------------*/
            if (timeout > 0)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Send an address byte                                                                    */
                /*-----------------------------------------------------------------------------------------*/
                SMB_WriteByte(module_num, SMB_status[module_num].dest_addr);

                /*-----------------------------------------------------------------------------------------*/
                /* Wait for the byte transfer to complete                                                  */
                /*-----------------------------------------------------------------------------------------*/
                timeout = ABORT_TIMEOUT;
                while (!READ_REG_FIELD(SMBST(module_num), SMBST_SDAST) && (--timeout != 0)) ;

                /*-----------------------------------------------------------------------------------------*/
                /* Generate a STOP condition & clear status bits (NEGACK, STASTR, BER, BB, and EO_BUSY)    */
                /*-----------------------------------------------------------------------------------------*/
                SMB_AbortData(module_num);
            }
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable interrupt                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_InterruptEnable(module_num, TRUE);
}

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
void SMB_StartStop (SMB_MODULE_T module_num)
{
    UINT16 timeout = ABORT_TIMEOUT;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable interrupt                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_InterruptEnable(module_num, FALSE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Generate a START condition, to synchronize Master and Slave                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_Start(module_num);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait until START condition is sent, or timeout                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    while (!READ_REG_FIELD(SMBST(module_num), SMBST_MASTER) && (--timeout != 0)) ;

    /*-----------------------------------------------------------------------------------------------------*/
    /* If START condition was sent                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    if (timeout > 0)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Generate a STOP condition and clear status                                                      */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_AbortData(module_num);
    }
    else
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Bus is stuck or busy, the only way to clear the Start bit is to reset the module                */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_Reset(module_num);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable interrupt                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_InterruptEnable(module_num, TRUE);
}
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
BOOLEAN SMB_IsSlaveAddressExist (SMB_MODULE_T module_num, UINT8 addr)
{
    UINT i;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the enable bit                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    addr |= 0x80;

    for (i = SMB_SLAVE_ADDR1; i < SMB_NUM_OF_ADDR; i++)
    {
        if (addr == SMB_GetSlaveAddress_l(module_num, (SMB_ADDR_T)i))
        {
            return TRUE;
        }
    }

    return FALSE;
}

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
DEFS_STATUS SMB_AddSlaveAddress (SMB_MODULE_T module_num, UINT8 slaveAddrToAssign, BOOLEAN use_PEC)
{
    UINT        i;
    DEFS_STATUS ret = DEFS_STATUS_FAIL;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the enable bit                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    slaveAddrToAssign |= 0x80;

    for (i = SMB_SLAVE_ADDR1; i < SMB_NUM_OF_ADDR; i++)
    {
        UINT8 currentSlaveAddr = SMB_GetSlaveAddress_l(module_num, (SMB_ADDR_T)i);
        if (currentSlaveAddr == slaveAddrToAssign)
        {
            ret = DEFS_STATUS_OK;
            break;
        }
        else if ((currentSlaveAddr & 0x7F) == 0)
        {
            ret = SMB_SlaveEnable_l(module_num, (SMB_ADDR_T)i, slaveAddrToAssign, TRUE);
            break;
        }
    }

    if (ret == DEFS_STATUS_OK)
    {
        if (use_PEC)
        {
            SET_VAR_BIT(SMB_status[module_num].PEC_mask, i);
        }
        else
        {
            CLEAR_VAR_BIT(SMB_status[module_num].PEC_mask, i);
        }
    }
    return ret;
}

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
DEFS_STATUS SMB_RemSlaveAddress (SMB_MODULE_T module_num, UINT8 slaveAddrToRemove)
{
    UINT    i;
    BOOLEAN intr;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the enable bit                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    slaveAddrToRemove |= 0x80;

    /*-----------------------------------------------------------------------------------------------------*/
    /* disable interrupts and select bank 0 for address 3 to ...                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_SAVE_DISABLE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);
    SMB_SelectBank(module_num, SMB_BANK_0);

    for (i = SMB_SLAVE_ADDR1; i < SMB_NUM_OF_ADDR; i++)
    {
        if (REG_READ(SMBADDR(module_num, i)) == slaveAddrToRemove)
        {
            REG_WRITE(SMBADDR(module_num,i), 0);
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* return to bank 1 and enable interrupts (if needed)                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_SelectBank(module_num, SMB_BANK_1);
    INTERRUPT_RESTORE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);

    return DEFS_STATUS_OK;
}

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
DEFS_STATUS SMB_GetCurrentSlaveAddress (SMB_MODULE_T module_num, UINT8* currSlaveAddr)
{
    if (currSlaveAddr != NULL)
    {
        *currSlaveAddr = SMB_CurSlaveAddr[module_num];
        return DEFS_STATUS_OK;
    }
    return DEFS_STATUS_INVALID_PARAMETER;
}

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
DEFS_STATUS SMB_SlaveGlobalCallEnable (SMB_MODULE_T module_num, BOOLEAN enable)
{
    return SMB_SlaveEnable_l(module_num, SMB_GC_ADDR, 0, enable);
}

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
DEFS_STATUS SMB_SlaveARPEnable (SMB_MODULE_T module_num, BOOLEAN enable)
{
    return SMB_SlaveEnable_l(module_num, SMB_ARP_ADDR, 0, enable);
}

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
BOOLEAN SMB_StartSlaveReceive (
    SMB_MODULE_T    module_num,
    UINT16          nread,
    UINT8*          read_data
)
{
    ASSERT(module_num < SMB_NUM_OF_MODULES);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Allow only if bus is not busy                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((SMB_status[module_num].operation_state != SMB_SLAVE_MATCH)
#ifdef SMBUS_SIZE_CHECK
        ||
        ((nread >= _32KB_) && (nread != SMB_BYTES_BLOCK_PROT) && (nread != SMB_BYTES_QUICK_PROT))
#endif
        )
    {
        return FALSE;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Update driver state                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_status[module_num].operation_state = SMB_OPER_STARTED;
    SMB_status[module_num].operation       = SMB_READ_OPER;
    SMB_status[module_num].read_data_buf   = read_data;
    SMB_status[module_num].read_size       = nread;
    SMB_status[module_num].read_index      = 0;
    SMB_status[module_num].write_size      = 0;
    SMB_status[module_num].write_index     = 0;

    if (SMB_status[module_num].fifo_use == TRUE)
    {
        if (nread > 0)
        {
            UINT8 smbrxf_ctl;

            if (nread <= SMBUS_FIFO_SIZE)
            {
                smbrxf_ctl = BUILD_FIELD_VAL(SMBRXF_CTL_THR_RXIE, 0);
                smbrxf_ctl |= BUILD_FIELD_VAL(SMBRXF_CTL_RX_THR, nread);
            }
            else
            {
                /*-----------------------------------------------------------------------------------------*/
                /* if threshold_fifo != SMBUS_FIFO_SIZE set SMBRXF_CTL.THR_RXIE to 1 otherwise to 0        */
                /*-----------------------------------------------------------------------------------------*/
                smbrxf_ctl = BUILD_FIELD_VAL(SMBRXF_CTL_RX_THR, SMB_status[module_num].threshold_fifo) |
                             BUILD_FIELD_VAL(SMBRXF_CTL_THR_RXIE, (BOOLEAN)(SMB_status[module_num].threshold_fifo != SMBUS_FIFO_SIZE));
            }
            REG_WRITE(SMBRXF_CTL(module_num), smbrxf_ctl);
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* triggers new data reception                                                                     */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_NMATCH));
    }

    return TRUE;
}

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
BOOLEAN SMB_StartSlaveTransmit (
    SMB_MODULE_T    module_num,
    UINT16          nwrite,
    UINT8*          write_data
)
{
    ASSERT(module_num < SMB_NUM_OF_MODULES);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Allow only if bus is not busy                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((SMB_status[module_num].operation_state != SMB_SLAVE_MATCH) ||
        (nwrite == 0))
    {
        return FALSE;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Update driver state                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    if (SMB_status[module_num].PEC_use)
    {
        nwrite++;
    }
    SMB_status[module_num].operation_state = SMB_OPER_STARTED;
    SMB_status[module_num].operation       = SMB_WRITE_OPER;
    SMB_status[module_num].write_data_buf  = write_data;
    SMB_status[module_num].write_size      = nwrite;
    SMB_status[module_num].write_index     = 0;

    if (SMB_status[module_num].fifo_use == TRUE)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* triggers new data reception                                                                     */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_NMATCH));

        if (nwrite > 0)
        {
            UINT8 smbtxf_ctl;

            if (nwrite <= SMBUS_FIFO_SIZE)
            {
                smbtxf_ctl = BUILD_FIELD_VAL(SMBTXF_CTL_THR_TXIE, 0) |
                             BUILD_FIELD_VAL(SMBTXF_CTL_TX_THR, 0);
            }
            else
            {
                /*-----------------------------------------------------------------------------------------*/
                /* if threshold_fifo != SMBUS_FIFO_SIZE set SMBTXF_CTL.THR_TXIE to 1 otherwise to 0        */
                /*-----------------------------------------------------------------------------------------*/
                smbtxf_ctl = BUILD_FIELD_VAL(SMBTXF_CTL_THR_TXIE, (BOOLEAN)(SMB_status[module_num].threshold_fifo != SMBUS_FIFO_SIZE)) |
                             BUILD_FIELD_VAL(SMBTXF_CTL_TX_THR, SMBUS_FIFO_SIZE - SMB_status[module_num].threshold_fifo);
            }
            REG_WRITE(SMBTXF_CTL(module_num), smbtxf_ctl);

#ifdef SMB_CAPABILITY_HW_PEC_SUPPORT
            /*---------------------------------------------------------------------------------------------*/
            /* Put the PEC byte last after the FIFO becomes empty.                                         */
            /*---------------------------------------------------------------------------------------------*/
            if ((SMB_status[module_num].PEC_use) && (nwrite > 1))
            {
                nwrite--;
            }
#endif

            /*---------------------------------------------------------------------------------------------*/
            /* Fill the FIFO with data                                                                     */
            /*---------------------------------------------------------------------------------------------*/
            SMB_WriteToFifo(module_num, MIN(SMBUS_FIFO_SIZE, nwrite));
        }
    }

    return TRUE;
}
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
/*---------------------------------------------------------------------------------------------------------*/
void SMB_EnableTimeout (SMB_MODULE_T module_num, BOOLEAN enable)
{
#ifdef SMB_DISABLE_TO_CLEAR_T_OUTST
    UINT8 smbEnabled;
    UINT8 smbctl1 = 0;
#endif
    if (enable)
    {
#ifdef SMB_DISABLE_TO_CLEAR_T_OUTST
        /*-------------------------------------------------------------------------------------------------*/
        /* TO_CKDIV may be changed only when the SMB is disabled                                           */
        /*-------------------------------------------------------------------------------------------------*/
        smbEnabled = READ_REG_FIELD(SMBCTL2(module_num), SMBCTL2_ENABLE);

        /*-------------------------------------------------------------------------------------------------*/
        /* If SMB is enabled - disable the SMB module                                                      */
        /*-------------------------------------------------------------------------------------------------*/
        if (smbEnabled)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Save smbctl1 relevant bits. It is being cleared when the module is disabled                 */
            /*---------------------------------------------------------------------------------------------*/
            smbctl1 = REG_READ(SMBCTL1(module_num)) & (MASK_FIELD(SMBCTL1_GCMEN) | MASK_FIELD(SMBCTL1_INTEN) | MASK_FIELD(SMBCTL1_NMINTE)) ;

            /*---------------------------------------------------------------------------------------------*/
            /* Disable the SMB module                                                                      */
            /*---------------------------------------------------------------------------------------------*/
            SET_REG_FIELD(SMBCTL2(module_num), SMBCTL2_ENABLE, DISABLE);
        }
#endif
        /*-------------------------------------------------------------------------------------------------*/
        /* Clear T_OUTST pending bit                                                                       */
        /*-------------------------------------------------------------------------------------------------*/
        SET_REG_FIELD(SMBT_OUT(module_num), SMBT_OUT_T_OUTST, 1);

#ifdef SMB_DISABLE_TO_CLEAR_T_OUTST
        /*-------------------------------------------------------------------------------------------------*/
        /* If SMB was enabled - re-enable the SMB module                                                   */
        /*-------------------------------------------------------------------------------------------------*/
        if (smbEnabled)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Enable the SMB module                                                                       */
            /*---------------------------------------------------------------------------------------------*/
            (void)SMB_Enable(module_num);

            /*---------------------------------------------------------------------------------------------*/
            /* Restore smbctl1 status                                                                      */
            /*---------------------------------------------------------------------------------------------*/
            REG_WRITE(SMBCTL1(module_num),  smbctl1);
        }
#endif
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable/Disable the bus timeout interrupt                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBT_OUT(module_num), SMBT_OUT_T_OUTIE, enable);
}
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
/* Side effects:    Mast be call after SMB_InitModule() (Since it uses BANK_1 registers)                   */
/* Description:                                                                                            */
/*                  This routine configures the "Strat To Stop" timeout.                                   */
/*                  If during transaction the time elapsed from a Start condition without a Stop condition */
/*                  is longer than the defined timeout. The SMB HW will issue a timeout indication and the */
/*                  transaction will be aborted.                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_EnableStartToStopTimeout (SMB_MODULE_T module_num, UINT16 timeout)
{
    if ((timeout < 35) || (timeout > 1011))
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Disable Start To Stop (Frame) timeout mechanism                                                 */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(SMB_FRTO(module_num), 0);
    }
    else
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Clear FRTOST pending bit                                                                       */
        /*-------------------------------------------------------------------------------------------------*/
        SET_REG_FIELD(SMB_FRTO(module_num), SMB_FRTO_FRTOST, 1);

        /*-------------------------------------------------------------------------------------------------*/
        /* calculate FR_LEN_TO value                                                                       */
        /* the actual HW timeout = FR_LEN_TO * 16 + 3                                                      */
        /* Therefore we cannot support any value between 35 to 1011 ms. Need to make sure that the         */
        /* calculation will not give a timeout lower than the requested timeout.                           */
        /*-------------------------------------------------------------------------------------------------*/
        timeout = timeout - 3;
        if ((timeout & 0xF) != 0)  /* timeout % 16 is not 0 */
        {
           timeout = (timeout / 16) + 1;  /* make sure we are not creating a smaller timeout */
        }
        else
        {
            timeout = (timeout / 16);
        }
        /*-------------------------------------------------------------------------------------------------*/
        /* Progrand the timeout and enable it                                                              */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(SMB_FRTO(module_num), MASK_FIELD(SMB_FRTO_FRTOIE) | timeout);
    }
}
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
void SMB_SetStallAfterStartIdle (SMB_MODULE_T module_num, BOOLEAN enable)
{
    SET_REG_FIELD(SMBCTL3(module_num), SMBCTL3_IDL_START, enable);
}

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
void SMB_ConfigStallThreshold (UINT module_num, UINT8 threshold)
{
    SMB_status[module_num].stall_threshold = threshold;
}

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
void SMB_StallHandler (void)
{
    UINT module_num;

    for (module_num = 0; module_num < SMB_NUM_OF_MODULES; module_num++)
    {
        if ((SMB_status[module_num].operation_state != SMB_IDLE) &&
            (SMB_status[module_num].operation_state != SMB_DISABLE) &&
            (SMB_status[module_num].master_or_slave != SMB_SLAVE))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* increase timeout counter                                                                    */
            /*---------------------------------------------------------------------------------------------*/
            SMB_status[module_num].stall_counter++;

            /*---------------------------------------------------------------------------------------------*/
            /* time expired, execute recovery                                                              */
            /*---------------------------------------------------------------------------------------------*/
            if ((SMB_status[module_num].stall_counter) >= SMB_status[module_num].stall_threshold)
            {
                SMB_MasterAbort(module_num);
                EXECUTE_FUNC(SMB_callback, (module_num, SMB_BUS_ERR_IND, SMB_GetIndex(module_num)));
                return;
            }
        }
    }
}
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
void SMB_ReEnableModule (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable SMB interrupt and New Address Match interrupt source                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_NMINTE, ENABLE);
    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_INTEN,  ENABLE);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_InterruptIsPending                                                                 */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         BOOLEAN                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks if we have pending interrupt                                       */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN SMB_InterruptIsPending (void)
{
    SMB_MODULE_T    module_num;
    BOOLEAN         InterruptIsPending = FALSE;

    for (module_num = 0; module_num < SMB_NUM_OF_MODULES; module_num++)
    {
        InterruptIsPending |= INTERRUPT_PENDING(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num));
    }

    return InterruptIsPending;
}

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
void SMB_WriteSCL (SMB_MODULE_T module_num, SMB_LEVEL_T level)
{
    BOOLEAN intr;
    UINT8   smbctl3;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Select Bank 0 to access SMBCTL4                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_SAVE_DISABLE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);
    SMB_SelectBank(module_num, SMB_BANK_0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read current SMBCTL3 and set SCL level (set SDA level to high)                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    smbctl3 = REG_READ(SMBCTL3(module_num));
    SET_VAR_FIELD(smbctl3, SMBCTL3_SCL_LVL, level);
    SET_VAR_FIELD(smbctl3, SMBCTL3_SDA_LVL, SMB_LEVEL_HIGH);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set SCL_LVL, SDA_LVL bits as Read/Write (R/W)                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL4(module_num), SMBCTL4_LVL_WE, 1);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Write SMBCTL3 to set the SCL level                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBCTL3(module_num), smbctl3);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set SCL_LVL, SDA_LVL bits as Read Only (RO)                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL4(module_num), SMBCTL4_LVL_WE, 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Return to Bank 1                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_SelectBank(module_num, SMB_BANK_1);
    INTERRUPT_RESTORE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);
}

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
void SMB_WriteSDA (SMB_MODULE_T module_num, SMB_LEVEL_T level)
{
    BOOLEAN intr;
    UINT8   smbctl3;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Select Bank 0 to access SMBCTL4                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_SAVE_DISABLE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);
    SMB_SelectBank(module_num, SMB_BANK_0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read current SMBCTL3 and set SDA level (set SCL level to high)                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    smbctl3 = REG_READ(SMBCTL3(module_num));
    SET_VAR_FIELD(smbctl3, SMBCTL3_SDA_LVL, level);
    SET_VAR_FIELD(smbctl3, SMBCTL3_SCL_LVL, SMB_LEVEL_HIGH);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set SCL_LVL, SDA_LVL bits as Read/Write (R/W)                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL4(module_num), SMBCTL4_LVL_WE, 1);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Write SMBCTL3 to set the SDA level                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBCTL3(module_num), smbctl3);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set SCL_LVL, SDA_LVL bits as Read Only (RO)                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL4(module_num), SMBCTL4_LVL_WE, 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Return to Bank 1                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_SelectBank(module_num, SMB_BANK_1);
    INTERRUPT_RESTORE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);
}

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
SMB_LEVEL_T SMB_GetSDA (SMB_MODULE_T module_num)
{
    return (SMB_LEVEL_T)READ_REG_FIELD(SMBCST(module_num), SMBCST_TSDA);
}

#endif // SMB_CAPABILITY_FORCE_SCL_SDA

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintRegs (void)
{
    UINT i;
    UINT smbfCount = 0;

    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     SMB      */\n");
    HAL_PRINT("/*--------------*/\n\n");

    for (i = 0; i < SMB_NUM_OF_MODULES; i++)
    {
        SMB_PrintModuleRegs(i);

        //lint -e{774, 506} suppress PC-Lint warning on ''if' always evaluates to True'
        if (SMB_FIFO(i))
        {
            smbfCount++;
        }
    }

    if (smbfCount > 0)
    {
        HAL_PRINT("/*--------------*/\n");
        HAL_PRINT("/*     SMBF     */\n");
        HAL_PRINT("/*--------------*/\n\n");

        for (i = 0; i < SMB_NUM_OF_MODULES; i++)
        {
            //lint -e{774, 506} suppress PC-Lint warning on ''if' always evaluates to True'
            if (SMB_FIFO(i))
            {
                if (smbfCount == 1)
                {
                    SMBF_PrintModuleRegs(i, 0x0F);
                }
                else
                {
                    SMBF_PrintModuleRegs(i, i);
                }
            }
        }
    }
}

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
/*lint -e{715}      Suppress 'module_num' not referenced                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintModuleRegs (SMB_MODULE_T module_num)
{
    ASSERT(module_num < SMB_NUM_OF_MODULES);

    HAL_PRINT("SMB%d:\n", module_num);
    HAL_PRINT("------\n");
    HAL_PRINT("SMB%dSDA             = 0x%02X\n", module_num, REG_READ(SMBSDA(module_num)));
    HAL_PRINT("SMB%dST              = 0x%02X\n", module_num, REG_READ(SMBST(module_num)));
    HAL_PRINT("SMB%dCST             = 0x%02X\n", module_num, REG_READ(SMBCST(module_num)));
    HAL_PRINT("SMB%dCTL1            = 0x%02X\n", module_num, REG_READ(SMBCTL1(module_num)));
    HAL_PRINT("SMB%dADDR1           = 0x%02X\n", module_num, REG_READ(SMBADDR1(module_num)));
    HAL_PRINT("SMB%dCTL2            = 0x%02X\n", module_num, REG_READ(SMBCTL2(module_num)));
    HAL_PRINT("SMB%dADDR2           = 0x%02X\n", module_num, REG_READ(SMBADDR2(module_num)));
    HAL_PRINT("SMB%dCTL3            = 0x%02X\n", module_num, REG_READ(SMBCTL3(module_num)));
#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
    HAL_PRINT("SMB%dT_OUT           = 0x%02X\n", module_num, REG_READ(SMBT_OUT(module_num)));
#endif
    HAL_PRINT("SMB%dADDR3           = 0x%02X\n", module_num, REG_READ(SMBADDR3(module_num)));
    HAL_PRINT("SMB%dADDR7           = 0x%02X\n", module_num, REG_READ(SMBADDR7(module_num)));
    HAL_PRINT("SMB%dADDR4           = 0x%02X\n", module_num, REG_READ(SMBADDR4(module_num)));
    HAL_PRINT("SMB%dADDR8           = 0x%02X\n", module_num, REG_READ(SMBADDR8(module_num)));
    HAL_PRINT("SMB%dADDR5           = 0x%02X\n", module_num, REG_READ(SMBADDR5(module_num)));
    HAL_PRINT("SMB%dADDR9           = 0x%02X\n", module_num, REG_READ(SMBADDR9(module_num)));
    HAL_PRINT("SMB%dADDR6           = 0x%02X\n", module_num, REG_READ(SMBADDR6(module_num)));
    HAL_PRINT("SMB%dADDR10          = 0x%02X\n", module_num, REG_READ(SMBADDR10(module_num)));
    HAL_PRINT("SMB%dCST2            = 0x%02X\n", module_num, REG_READ(SMBCST2(module_num)));
    HAL_PRINT("SMB%dCST3            = 0x%02X\n", module_num, REG_READ(SMBCST3(module_num)));
    HAL_PRINT("SMB%dCTL4            = 0x%02X\n", module_num, REG_READ(SMBCTL4(module_num)));
    HAL_PRINT("SMB%dCTL5            = 0x%02X\n", module_num, REG_READ(SMBCTL5(module_num)));
    HAL_PRINT("SMB%dSCLLT           = 0x%02X\n", module_num, REG_READ(SMBSCLLT(module_num)));
    HAL_PRINT("SMB%dSCLHT           = 0x%02X\n", module_num, REG_READ(SMBSCLHT(module_num)));
    HAL_PRINT("SMB%dVER             = 0x%02X\n", module_num, REG_READ(SMB_VER(module_num)));

    HAL_PRINT("\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_PrintVars                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module variables                                               */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintVars (void)
{
    UINT i;

    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     SMB      */\n");
    HAL_PRINT("/*--------------*/\n\n");

    for (i = 0; i < SMB_NUM_OF_MODULES; i++)
    {
        SMB_PrintModuleVars((SMB_MODULE_T)i);
    }
}

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
/*lint -e{715}      Suppress 'module_num' not referenced                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintModuleVars (SMB_MODULE_T module_num)
{
    ASSERT(module_num < SMB_NUM_OF_MODULES);

    HAL_PRINT("SMB%d:\n", module_num);
    HAL_PRINT("-----\n");

    HAL_PRINT("operation_state         = %s\n",     SMB_OperationStateStr(SMB_status[module_num].operation_state));
    HAL_PRINT("operation               = %s\n",     SMB_OperationStr(SMB_status[module_num].operation));
    HAL_PRINT("master_or_slave         = %s\n",     SMB_ModeStr(SMB_status[module_num].master_or_slave));
#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
    HAL_PRINT("stop_indication         = %s\n",     SMB_StateIndStr(SMB_status[module_num].stop_indication));
#endif
    HAL_PRINT("dest_addr               = 0x%02X\n", SMB_status[module_num].dest_addr);
    HAL_PRINT("read_size               = %d\n",     SMB_status[module_num].read_size);
    HAL_PRINT("read_index              = %d\n",     SMB_status[module_num].read_index);
    HAL_PRINT("write_size              = %d\n",     SMB_status[module_num].write_size);
    HAL_PRINT("write_index             = %d\n",     SMB_status[module_num].write_index);
    HAL_PRINT("fifo_use                = %s\n",     SMB_status[module_num].fifo_use ? "TRUE" : "FALSE");
    HAL_PRINT("threshold_fifo          = %d\n",     SMB_status[module_num].threshold_fifo);
    HAL_PRINT("PEC_mask                = 0x%04X\n", SMB_status[module_num].PEC_mask);
    HAL_PRINT("PEC_use                 = %s\n",     SMB_status[module_num].PEC_use ? "TRUE" : "FALSE");
    HAL_PRINT("crc_data                = 0x%02X\n", SMB_status[module_num].crc_data);
    HAL_PRINT("read_block_use          = %s\n",     SMB_status[module_num].read_block_use ? "TRUE" : "FALSE");
    HAL_PRINT("retry_count             = %d\n",     SMB_status[module_num].retry_count);
#ifdef SMB_STALL_TIMEOUT_SUPPORT
    HAL_PRINT("stall_counter           = %d\n",     SMB_status[module_num].stall_counter);
    HAL_PRINT("stall_threshold         = %d\n",     SMB_status[module_num].stall_threshold);
#endif

    HAL_PRINT("\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_PrintVersion                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void SMB_PrintVersion (void)
{
    HAL_PRINT("SMB         = %X\n", MODULE_VERSION(SMB_MODULE_TYPE));
}


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_WriteByte                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                  data       - Data to write.                                                            */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine writes a byte to the SMBus.                                               */
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ void SMB_WriteByte (
    SMB_MODULE_T module_num,
    UINT8        data
)
{
    REG_WRITE(SMBSDA(module_num), data);
    SMB_CalcPEC(module_num, data);
#ifdef SMB_STALL_TIMEOUT_SUPPORT
    SMB_RESET_STALL_COUNTER(module_num);
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_ReadByte                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                  data       - Data pointer to read into.                                                */
/*                                                                                                         */
/* Returns:         TRUE is successful read; FALSE otherwise.                                              */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reads a byte from the SMBus.                                              */
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ BOOLEAN SMB_ReadByte (
    SMB_MODULE_T module_num,
    UINT8 *      data
)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Read data                                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    *data = REG_READ(SMBSDA(module_num));
    SMB_CalcPEC(module_num, *data);
#ifdef SMB_STALL_TIMEOUT_SUPPORT
    SMB_RESET_STALL_COUNTER(module_num);
#endif

    return (TRUE);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_SelectBank                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                  bank       - SMB bank number.                                                          */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine selects the active bank of registers, in FIFO mode.                       */
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ void SMB_SelectBank (SMB_MODULE_T module_num, SMB_BANK_T bank)
{
    if (SMB_status[module_num].fifo_use == TRUE)
    {
        SET_REG_FIELD(SMBCTL3(module_num), SMBCTL3_BNK_SEL, bank);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_GetIndex                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine retrieves the current read/write index.                                   */
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ UINT16 SMB_GetIndex (SMB_MODULE_T module_num)
{
    UINT16 index = 0;

    if (SMB_status[module_num].operation == SMB_READ_OPER)
    {
        index = SMB_status[module_num].read_index;
    }
    else if (SMB_status[module_num].operation == SMB_WRITE_OPER)
    {
        index = SMB_status[module_num].write_index;
    }

    return index;
}

#ifdef SMB_MASTER_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_Start                                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine generates a Start condition on the SMBus.                                 */
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ void SMB_Start (SMB_MODULE_T module_num)
{
    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_START, TRUE);
#ifdef SMB_STALL_TIMEOUT_SUPPORT
    SMB_RESET_STALL_COUNTER(module_num);
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_Stop                                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine generates a Stop condition on the SMBus if it is the Master.              */
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ void SMB_Stop (SMB_MODULE_T module_num)
{
#ifdef SMB_SW_BYPASS_HW_ISSUE_SMB_STOP
    /*-----------------------------------------------------------------------------------------------------*/
    /* override issue #614: CP_FW: SMBus may fail to supply stop condition in Master Write operation.      */
    /* If needed : define it at hal_cfg.h                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_DELAY((SMB_status[module_num].clk_period_us >> 1), SMB_status[module_num].interrupt_time_stamp);
#endif

    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_STOP, TRUE);

    if (SMB_status[module_num].fifo_use)
    {
        SET_REG_FIELD(SMBRXF_STS(module_num), SMBRXF_STS_RX_THST, 1);
        SET_REG_MASK(SMBFIF_CTS(module_num), MASK_FIELD(SMBFIF_CTS_SLVRSTR) |
                                             MASK_FIELD(SMBFIF_CTS_RXF_TXE));

        REG_WRITE(SMBTXF_CTL(module_num), 0);
    }

#ifdef SMB_STALL_TIMEOUT_SUPPORT
    SMB_RESET_STALL_COUNTER(module_num);
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_AbortData                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs partial recovery to the bus:                                     */
/*                  - Issue STOP event                                                                     */
/*                  - Clear NEGACK, STASTR and BER bits                                                    */
/*                  - Clear BB (BUS BUSY) bit                                                              */
/*                  - Clear EO_BUSY (END OF BUSY) bit                                                      */
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ void SMB_AbortData (SMB_MODULE_T module_num)
{
    UINT timeout = ABORT_TIMEOUT;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Generate a STOP condition                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_Stop(module_num);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear NEGACK, STASTR and BER bits                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBST(module_num), (MASK_FIELD(SMBST_STASTR) |
                                  MASK_FIELD(SMBST_NEGACK) |
                                  MASK_FIELD(SMBST_BER)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait till STOP condition is generated                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    while (--timeout)
    {
        if (! READ_REG_FIELD(SMBCTL1(module_num), SMBCTL1_STOP))
        {
            break;
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear BB (BUS BUSY) bit                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBCST(module_num), MASK_FIELD(SMBCST_BB));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear EO_BUSY pending bit                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCST3(module_num), SMBCST3_EO_BUSY, 1);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_StallAfterStart                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                  stall      - TRUE to stall any further SMB operation.                                  */
/*                               FALSE to cancel the Stall After Start mechanism.                          */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine controls the Stall After Start mechanism.                                 */
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ void SMB_StallAfterStart (
    SMB_MODULE_T module_num,
    BOOLEAN      stall
)
{
    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_STASTRE, stall);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_Nack                                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine issues a negative acknowledge onto the SMBus.                             */
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ void SMB_Nack (SMB_MODULE_T module_num)
{
    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_ACK, TRUE);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_MasterFifoRead                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs...                                                               */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_MasterFifoRead (SMB_MODULE_T module_num)
{
    UINT16          rcount;
    UINT8           fifo_bytes;
    SMB_STATE_IND_T ind = SMB_MASTER_DONE_IND;

    rcount = SMB_status[module_num].read_size - SMB_status[module_num].read_index;

    fifo_bytes = READ_REG_FIELD(SMBRXF_STS(module_num), SMBRXF_STS_RX_BYTES);

    if ((fifo_bytes == 0) && (READ_REG_FIELD(SMBST(module_num), SMBST_XMIT)))
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Found that a glitch on SDA during re-start (on a read after write transaction) might            */
        /* get the HW/Driver into a wrong state. It looks like the Driver switch to a "Read" mode          */
        /* but the HW is still in stack in transmit mode, and therefore RX FIFO is empty and the           */
        /* XMT bit is set. If we detect this case we abort the transaction.                                */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_MasterAbort(module_num);
        EXECUTE_FUNC(SMB_callback, (module_num, SMB_BUS_ERR_IND, SMB_GetIndex(module_num)));
        return; /* Exit before calling SMB_RESET_STALL_COUNTER */
     }
    /*-----------------------------------------------------------------------------------------------------*/
    /* In order not to change the RX_TRH during transaction (we found that this might                      */
    /* be problematic if it takes too much time to read the FIFO) we read the data in the                  */
    /* following way. If the number of bytes to read == FIFO Size + C (where C < FIFO Size)                */
    /* then first read C bytes and in the next interrupt we read rest of the data.                         */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((rcount < (2 * SMBUS_FIFO_SIZE)) && (rcount > SMBUS_FIFO_SIZE))
    {
        fifo_bytes = (UINT8)(rcount - SMBUS_FIFO_SIZE);
    }

    if (rcount - fifo_bytes == 0)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* last byte is about to be read - end of transaction.                                             */
        /* Stop should be set before reading last byte.                                                    */
        /*-------------------------------------------------------------------------------------------------*/
#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
        /*-------------------------------------------------------------------------------------------------*/
        /* Enable "End of Busy" interrupt.                                                                 */
        /*-------------------------------------------------------------------------------------------------*/
        SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_EOBINTE, 1);
#endif
        SMB_Stop(module_num);

        SMB_ReadFromFifo(module_num, fifo_bytes);

        /*-------------------------------------------------------------------------------------------------*/
        /* PEC value is not correct                                                                        */
        /*-------------------------------------------------------------------------------------------------*/
        if (!SMBUS_PEC_IS_CORRECT(module_num))
        {
            ind = SMB_MASTER_PEC_ERR_IND;
        }

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
        SMB_status[module_num].operation_state = SMB_STOP_PENDING;
        SMB_status[module_num].stop_indication = ind;
#else
        /*-------------------------------------------------------------------------------------------------*/
        /* Reset state for new transaction                                                                 */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_status[module_num].operation_state = SMB_IDLE;

        /*-------------------------------------------------------------------------------------------------*/
        /* Notify upper layer of transaction completion                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        EXECUTE_FUNC(SMB_callback, (module_num, ind, SMB_status[module_num].read_index));
#endif
    }
    else
    {
        SMB_ReadFromFifo(module_num, fifo_bytes);
        rcount = SMB_status[module_num].read_size - SMB_status[module_num].read_index;

        if (rcount > 0)
        {
            SET_REG_FIELD(SMBRXF_STS(module_num), SMBRXF_STS_RX_THST, 1);
            SET_REG_FIELD(SMBFIF_CTS(module_num), SMBFIF_CTS_RXF_TXE, 1);
            if (rcount > SMBUS_FIFO_SIZE)
            {
                SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR, SMBUS_FIFO_SIZE);
            }
            else
            {
                SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR, (UINT8)rcount);
                SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_LAST_PEC, 1);
            }
        }
    }
#ifdef SMB_STALL_TIMEOUT_SUPPORT
    SMB_RESET_STALL_COUNTER(module_num);
#endif
}
#endif  /* SMB_MASTER_SUPPORT */

#ifdef SMB_SLAVE_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_SlaveEnable_l                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB Module number to be configured.                                       */
/*                  addr_type  - Slave address type to configure.                                          */
/*                  addr       - The address to configure.                                                 */
/*                               if addr_type is SMB_GC_ADDR or SMB_ARP_ADDR addr is ignored.              */
/*                  enable     - When TRUE,  module will respond as slave to transactions on slave_addr.   */
/*                               When FALSE, module will ignore transactions directed to slave_addr.       */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables or disables operation of the specified SMB module as a slave      */
/*                  SMBus device.                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
static DEFS_STATUS SMB_SlaveEnable_l (
    SMB_MODULE_T module_num,
    SMB_ADDR_T   addr_type,
    UINT8        addr,
    BOOLEAN      enable
)
{
    BOOLEAN intr;
    UINT8   SmbAddrX_Addr = BUILD_FIELD_VAL(SMBADDRx_ADDR, addr) | BUILD_FIELD_VAL(SMBADDRx_SAEN, enable);

    if (addr_type == SMB_GC_ADDR)
    {
        SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_GCMEN, enable);
        return DEFS_STATUS_OK;
    }
    if (addr_type == SMB_ARP_ADDR)
    {
        SET_REG_FIELD(SMBCTL3(module_num), SMBCTL3_ARPMEN, enable);
        return DEFS_STATUS_OK;
    }
    if (addr_type >= SMB_NUM_OF_ADDR)
    {
        return DEFS_STATUS_FAIL;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable interrupts and select bank 0 for address 3 to ...                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_SAVE_DISABLE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);
    SMB_SelectBank(module_num, SMB_BANK_0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set and enable the address                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBADDR(module_num,addr_type), SmbAddrX_Addr);

    /*-----------------------------------------------------------------------------------------------------*/
    /* return to bank 1 and enable interrupts (if needed)                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_SelectBank(module_num, SMB_BANK_1);
    INTERRUPT_RESTORE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_GetSlaveAddress_l                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - The SMB module number                                                     */
/*                  addrEnum   - Enumeration of the slave address required                                 */
/*                                                                                                         */
/* Returns:         Byte: Slave address that correlate to the enumeration including bit7 which indicate    */
/*                  if the address is enabled or not: 1 - Enabled, 0 - Disabled                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine given an enumeration of a slave address gets the correlating              */
/*                  slave address (including the enable bit).                                              */
/*---------------------------------------------------------------------------------------------------------*/
static UINT8 SMB_GetSlaveAddress_l (SMB_MODULE_T module_num, SMB_ADDR_T addrEnum)
{
    BOOLEAN intr = 0;
    UINT8   slaveAddress;

    /*-----------------------------------------------------------------------------------------------------*/
    /* disable interrupts and select bank 0 for address 3 to ...                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_SAVE_DISABLE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);
    SMB_SelectBank(module_num, SMB_BANK_0);

    slaveAddress = REG_READ(SMBADDR(module_num, addrEnum));

    /*-----------------------------------------------------------------------------------------------------*/
    /* return to bank 1 and enable interrupts (if needed)                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_SelectBank(module_num, SMB_BANK_1);
    INTERRUPT_RESTORE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);

    return slaveAddress;
}
#endif  /* SMB_SLAVE_SUPPORT */

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_Enable                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMBus module to initialize.                                               */
/*                                                                                                         */
/* Returns:         FALSE if a module was not enabled; TRUE otherwise.                                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine is a bypass to a errata issue that was found in NPCE2mnx                  */
/*                  If the SMBus module is enabled while a low-to-high transaction on SCL or SDA ,         */
/*                  unpredictable SMB module behaviour will happen.                                        */
/*---------------------------------------------------------------------------------------------------------*/
static BOOLEAN SMB_Enable (SMB_MODULE_T module_num)
{
    SET_REG_FIELD(SMBCTL2(module_num), SMBCTL2_ENABLE, ENABLE);
    return TRUE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_Reset                                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:    Called as part of SMB_MasterAbort.                                                     */
/* Description:                                                                                            */
/*                  This routine performs the following:                                                   */
/*                  - Disable SMB                                                                          */
/*                  - Disable global call address                                                          */
/*                  - Disable slave addresses                                                              */
/*                  - Enable SMB                                                                           */
/*                  - Issue STOP condition                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef SMB_TEST_MODE
static
#endif
void SMB_Reset (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Save smbctl1 relevant bits. It is being cleared when the module is disabled                         */
    /*-----------------------------------------------------------------------------------------------------*/
    UINT8 smbctl1 = REG_READ(SMBCTL1(module_num)) & (MASK_FIELD(SMBCTL1_GCMEN) |
                                                     MASK_FIELD(SMBCTL1_INTEN) |
                                                     MASK_FIELD(SMBCTL1_NMINTE)) ;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable the SMB module                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL2(module_num), SMBCTL2_ENABLE, DISABLE);

#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear T_OUTST pending bit                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBT_OUT(module_num), SMBT_OUT_T_OUTST, 1);
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable the SMB module                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    (void)SMB_Enable(module_num);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Restore smbctl1 status                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBCTL1(module_num),  smbctl1);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Reset driver status                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_status[module_num].operation_state = SMB_IDLE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_InterruptEnable                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                  enable     - TRUE to enable the SMBus interrupt; FALSE otherwise.                      */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine disables or enables interrupt generation for the SMBus module.            */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef SMB_TEST_MODE
static
#endif
void SMB_InterruptEnable (
    SMB_MODULE_T module_num,
    BOOLEAN      enable
)
{
    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_INTEN, enable);
}

#if defined (MIWU_MODULE_TYPE) && defined (SMB_CAPABILITY_WAKEUP_SUPPORT)
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_WakeupHandler                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  miwu_source - MIWU input source.                                                       */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles SMB wakeup conditions identified by the MIWU module.              */
/*                  Called from the MIWU interrupt handler which is generated by Start Conditions on one   */
/*                  of the SMBuses in Idle mode.                                                           */
/*                  The upper layer is notified that event occurred through the callback function.         */
/*lint -e{715}      Suppress 'source' not referenced                                                       */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_WakeupHandler (MIWU_SRC_T source)
{
    SMB_MODULE_T module_num;    /* Module whose bus generated the wake-up */

    /*-----------------------------------------------------------------------------------------------------*/
    /* SMB module is enabled already (otherwise a wakeup signal isn't generated) -                         */
    /* so no need to enable the SMB. SMB HW responds with a negative acknowledge to the Start Condition -  */
    /* so either the Master Device will re-issue a Start Condition, or the upper layer will initiate a     */
    /* transaction as a master                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check wake-up source                                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    for (module_num = 0; module_num < SMB_NUM_OF_MODULES; module_num++)
    {
        if (READ_REG_BIT(SMB_SBD, module_num) && READ_REG_BIT(SMB_EEN, module_num))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Clear start-bit-detected status                                                             */
            /*---------------------------------------------------------------------------------------------*/
            REG_WRITE(SMB_SBD, MASK_BIT(module_num));

            /*---------------------------------------------------------------------------------------------*/
            /* Restore SMBnCTL1 register values, because the register is being reset when the core         */
            /* switches to Idle or Deep-Idle mode.                                                         */
            /*---------------------------------------------------------------------------------------------*/
            SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_NMINTE, ENABLE);
            SMB_InterruptEnable(module_num, TRUE);

            /*---------------------------------------------------------------------------------------------*/
            /* Notify upper layer of wake-up                                                               */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(SMB_callback, (module_num, SMB_WAKE_UP_IND, 0));
        }
    }
}
#endif  /* (MIWU_MODULE_TYPE) && (SMB_CAPABILITY_WAKEUP_SUPPORT) */

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_InitClock                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMBus module to initialize.                                               */
/*                  bus_freq   - Frequency, in KHz, to set for all the buses of the module.                */
/*                                                                                                         */
/* Returns:         FALSE if a module has already been initialized; TRUE otherwise.                        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the specified SMB module's clock.                             */
/*---------------------------------------------------------------------------------------------------------*/
static BOOLEAN SMB_InitClock (
    SMB_MODULE_T    module_num,
    UINT16          bus_freq
)
{
    BOOLEAN         intr;
    UINT32          src_clk_freq;
    UINT            scl_table_cnt = 0;
    SMB_TIMING_T    smb_timing;

    src_clk_freq = SMB_SOURCE_CLOCK(module_num);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Frequency is less or equal to 100 KHz                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    if (bus_freq <= SMBUS_FREQ_100KHz)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Scan the table to find where the core clk is less or equal to the requested core clk:           */
        /*-------------------------------------------------------------------------------------------------*/
        for (scl_table_cnt = 0; scl_table_cnt < (ARRAY_SIZE(SMB_TIMING_100KHZ) - 1); scl_table_cnt++)
        {
            if (src_clk_freq >= SMB_TIMING_100KHZ[scl_table_cnt].core_clk)
            {
                break;
            }
        }

        memcpy((void*)&smb_timing, (void*)&SMB_TIMING_100KHZ[scl_table_cnt], sizeof(SMB_TIMING_T));
    }

#ifdef SMB_CAPABILITY_FAST_MODE_SUPPORT
    /*-----------------------------------------------------------------------------------------------------*/
    /* Frequency equal to 400 KHz                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    else if (bus_freq <= SMBUS_FREQ_400KHz)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Scan the table to find where the core clk is less or equal to the requested core clk:           */
        /*-------------------------------------------------------------------------------------------------*/
        for (scl_table_cnt = 0; scl_table_cnt < (ARRAY_SIZE(SMB_TIMING_400KHZ) - 1); scl_table_cnt++)
        {
            if (src_clk_freq >= SMB_TIMING_400KHZ[scl_table_cnt].core_clk)
            {
                break;
            }
        }

        memcpy((void*)&smb_timing, (void*)&SMB_TIMING_400KHZ[scl_table_cnt], sizeof(SMB_TIMING_T));
    }
#endif

#ifdef SMB_CAPABILITY_FAST_MODE_PLUS_SUPPORT
    /*-----------------------------------------------------------------------------------------------------*/
    /* Frequency equal to 1 MHz                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    else if (bus_freq <= SMBUS_FREQ_1MHz)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Scan the table to find where the core clk is less or equal to the requested core clk:           */
        /*-------------------------------------------------------------------------------------------------*/
        for (scl_table_cnt = 0; scl_table_cnt < (ARRAY_SIZE(SMB_TIMING_1000KHZ) - 1); scl_table_cnt++)
        {
            if (src_clk_freq >= SMB_TIMING_1000KHZ[scl_table_cnt].core_clk)
            {
                break;
            }
        }

        memcpy((void*)&smb_timing, (void*)&SMB_TIMING_1000KHZ[scl_table_cnt], sizeof(SMB_TIMING_T));
    }
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Frequency larger than 1 MHz                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    else
    {
        return FALSE;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Select Bank 0 to access SMBCTL4/SMBCTL5                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_SAVE_DISABLE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);
    SMB_SelectBank(module_num, SMB_BANK_0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable module before any clock change:                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL2(module_num), SMBCTL2_ENABLE, 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* After clock parameters calculation update reg (ENABLE should be 0):                                 */
    /* force to bank 0, set SCL and fast mode                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL2(module_num) , SMBCTL2_SCLFRQ6_0, READ_VAR_FIELD(smb_timing.sclfrq, SCLFRQ_0_TO_6));
    SET_REG_FIELD(SMBCTL3(module_num) , SMBCTL3_SCLFRQ8_7, READ_VAR_FIELD(smb_timing.sclfrq, SCLFRQ_7_TO_8));
    SET_REG_FIELD(SMBCTL3(module_num) , SMBCTL3_400K_MODE, smb_timing.fastMode);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set SCL Low/High Time:                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBSCLLT(module_num), smb_timing.scllt);
    REG_WRITE(SMBSCLHT(module_num), smb_timing.sclht);

    /*-----------------------------------------------------------------------------------------------------*/
    /* DBNCT: determine the filter on SCL                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL5(module_num), SMBCTL5_DBNCT, smb_timing.dbcnt);

    /*-----------------------------------------------------------------------------------------------------*/
    /* HLDT (SDA Hold Time):                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBCTL4(module_num), smb_timing.hldt);

#if 0
    HAL_PRINT("i2c%d, core_freq = %d Hz, bus_freq=%d KHz, hldt = 0x%02X, dbcnt = 0x%02X, sclfrq = 0x%04X, scllt = 0x%02X, sclht =0x%02X, scl_table_cnt = %d\n",
              module_num, src_clk_freq, bus_freq, smb_timing.hldt, smb_timing.dbcnt, smb_timing.sclfrq, smb_timing.scllt, smb_timing.sclht, scl_table_cnt);
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Return to Bank 1 (and stay there by default)                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_SelectBank(module_num, SMB_BANK_1);
    INTERRUPT_RESTORE(SMB_INTERRUPT_PROVIDER, SMB_INTERRUPT(module_num), intr);

#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure the division of the SMB Module Basic clock (BCLK) to generate the 1 KHz clock of the      */
    /* timeout detector.                                                                                   */
    /* The timeout detector has an "n+1" divider, controlled by TO_CKDIV and a fixed divider by 1000.      */
    /* Together they generate the 1 ms clock cycle                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBT_OUT(module_num), MASK_FIELD(SMBT_OUT_T_OUTST) |                  /* Clear status bit    */
                                    (UINT8)(((src_clk_freq / _1KHz_) / 1000) - 1)); /* Set divider         */
#endif

    return TRUE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_ReadFromFifo                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num    - SMB module number                                                      */
/*                  bytes_in_fifo - Number of bytes to read from FIFO                                      */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reads from FIFO. Called from the interrupt handler                        */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_ReadFromFifo (SMB_MODULE_T module_num, UINT8 bytes_in_fifo)
{
#ifdef SMB_DDR_OPTIMIZATION
    _ALIGN_(4, UINT8    fifo_buf[SMBUS_FIFO_SIZE]) = {0};
    UINT16              read_index = SMB_status[module_num].read_index;
    UINT                fifo_index = 0;
#endif
    while (bytes_in_fifo--)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Keep read data                                                                                  */
        /*-------------------------------------------------------------------------------------------------*/
        UINT8 data = REG_READ(SMBSDA(module_num));
        SMB_CalcPEC(module_num, data);
#ifdef SMB_DDR_OPTIMIZATION
        if (read_index < SMB_status[module_num].read_size)
        {
            fifo_buf[fifo_index++] = data;
            read_index++;
            if ((read_index == 1) && SMB_status[module_num].read_size == SMB_BYTES_BLOCK_PROT)
#else
        if (SMB_status[module_num].read_index < SMB_status[module_num].read_size)
        {
            SMB_status[module_num].read_data_buf[SMB_status[module_num].read_index++] = data;
            if ((SMB_status[module_num].read_index == 1) && SMB_status[module_num].read_size == SMB_BYTES_BLOCK_PROT)
#endif
            {
                /*-----------------------------------------------------------------------------------------*/
                /* First byte indicates length in block protocol                                           */
                /*-----------------------------------------------------------------------------------------*/
                SMB_status[module_num].read_size = data;
            }
        }
    }
#ifdef SMB_DDR_OPTIMIZATION
    memcpy((void*)&SMB_status[module_num].read_data_buf[SMB_status[module_num].read_index],
           (void*)fifo_buf,
           read_index - SMB_status[module_num].read_index);

    SMB_status[module_num].read_index = read_index;
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_WriteToFifo                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num        - SMB module number                                                  */
/*                  max_bytes_to_send - Maximum bytes to write to FIFO                                     */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine writes to FIFO. Called from the interrupt handler                         */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_WriteToFifo (SMB_MODULE_T module_num, UINT16 max_bytes_to_send)
{
#ifdef SMB_DDR_OPTIMIZATION
    _ALIGN_(4, UINT8    fifo_buf[SMBUS_FIFO_SIZE]);
    UINT                fifo_index      = 0;
    UINT16              bytes_in_fifo_l = MIN(max_bytes_to_send, SMBUS_FIFO_SIZE);

    memcpy(fifo_buf,
           (void*)&SMB_status[module_num].write_data_buf[SMB_status[module_num].write_index],
           bytes_in_fifo_l);
#endif
    /*-----------------------------------------------------------------------------------------------------*/
    /* Fill the FIFO , while the FIFO is not full and there are more bytes to write                        */
    /*-----------------------------------------------------------------------------------------------------*/
    while ((max_bytes_to_send--) && (SMBUS_FIFO_SIZE - READ_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_BYTES)))
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* write the data                                                                                  */
        /*-------------------------------------------------------------------------------------------------*/
        if (SMB_status[module_num].write_index < SMB_status[module_num].write_size)
        {
            if ((SMB_status[module_num].PEC_use == TRUE) &&
                ((SMB_status[module_num].write_index + 1) == SMB_status[module_num].write_size) &&
                ((SMB_status[module_num].read_size == 0) || (SMB_status[module_num].master_or_slave == SMB_SLAVE)))
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Master send PEC in write protocol, Slave send PEC in read protocol.                     */
                /*-----------------------------------------------------------------------------------------*/
                SMBUS_SEND_PEC_BYTE(module_num);

                SMB_status[module_num].write_index++;
            }
            else
            {
#ifdef SMB_DDR_OPTIMIZATION
                SMB_WriteByte(module_num, fifo_buf[fifo_index++]);
                SMB_status[module_num].write_index++;
                if (fifo_index >= sizeof (fifo_buf))
                {
                    return;
                }
#else
                SMB_WriteByte(module_num, SMB_status[module_num].write_data_buf[SMB_status[module_num].write_index++]);
#endif
            }
        }
        else
        {
#ifdef SMB_WRAP_AROUND_BUFFER
            /*---------------------------------------------------------------------------------------------*/
            /* We're out of bytes. Ask the higher level for more bytes. Let it know that driver used all   */
            /* its bytes                                                                                   */
            /*---------------------------------------------------------------------------------------------*/

            /*---------------------------------------------------------------------------------------------*/
            /* clear the status bits                                                                       */
            /*---------------------------------------------------------------------------------------------*/
            SET_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_THST, 1);

            /*---------------------------------------------------------------------------------------------*/
            /* Reset state for the remaining bytes transaction                                             */
            /*---------------------------------------------------------------------------------------------*/
            SMB_status[module_num].operation_state = SMB_SLAVE_MATCH;

            /*---------------------------------------------------------------------------------------------*/
            /* Notify upper layer of transaction completion                                                */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(SMB_callback, (module_num, SMB_SLAVE_XMIT_MISSING_DATA_IND, SMB_status[module_num].write_index));

            REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_SDAST));
#else
            SMB_WriteByte(module_num, 0xFF);
#endif
        }
    }
}

#ifndef SMB_CAPABILITY_HW_PEC_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_CalculateCRC8                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  crc_data - existing CRC8 data                                                          */
/*                  data     - SMBus transaction data                                                      */
/*                                                                                                         */
/* Returns:         Updated CRC8 data                                                                      */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Handling CRC8 calculation.                                                             */
/*---------------------------------------------------------------------------------------------------------*/
static UINT8 SMB_CalculateCRC8 (UINT8 crc_data, UINT8 data)
{
    UINT8 tmp = crc_data^data;

    crc_data = SMB_crc8_table[tmp];

    return crc_data;
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_CalcPEC                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number                                                         */
/*                  data       - SMBus transaction data                                                    */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Calculate and update SMBus PEC data.                                                   */
/*lint -e{715}      Suppress 'module_num'/'data' not referenced                                            */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_CalcPEC (SMB_MODULE_T module_num, UINT8 data)
{
#ifndef SMB_CAPABILITY_HW_PEC_SUPPORT
    if (SMB_status[module_num].PEC_use)
    {
        SMB_status[module_num].crc_data = SMB_CalculateCRC8(SMB_status[module_num].crc_data, data);
    }
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMBF_PrintModuleRegs                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - The module to be printed.                                                 */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module instance registers                                      */
/*lint -e{715}      Suppress 'module_num'/'smbf_module' not referenced                                     */
/*---------------------------------------------------------------------------------------------------------*/
static void SMBF_PrintModuleRegs (SMB_MODULE_T module_num, SMB_MODULE_T smbf_module)
{
    ASSERT(module_num < SMB_NUM_OF_MODULES);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Common Registers                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    HAL_PRINT("SMBF%1X:\n", smbf_module);
    HAL_PRINT("------\n");
    HAL_PRINT("SMB%1XSDA             = 0x%02X\n", smbf_module, REG_READ(SMBSDA(module_num)));
    HAL_PRINT("SMB%1XST              = 0x%02X\n", smbf_module, REG_READ(SMBST(module_num)));
    HAL_PRINT("SMB%1XCST             = 0x%02X\n", smbf_module, REG_READ(SMBCST(module_num)));
    HAL_PRINT("SMB%1XCTL1            = 0x%02X\n", smbf_module, REG_READ(SMBCTL1(module_num)));
    HAL_PRINT("SMB%1XADDR1           = 0x%02X\n", smbf_module, REG_READ(SMBADDR1(module_num)));
    HAL_PRINT("SMB%1XCTL2            = 0x%02X\n", smbf_module, REG_READ(SMBCTL2(module_num)));
    HAL_PRINT("SMB%1XADDR2           = 0x%02X\n", smbf_module, REG_READ(SMBADDR2(module_num)));
    HAL_PRINT("SMB%1XCTL3            = 0x%02X\n", smbf_module, REG_READ(SMBCTL3(module_num)));
#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
    HAL_PRINT("SMB%1XT_OUT           = 0x%02X\n", smbf_module, REG_READ(SMBT_OUT(module_num)));
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Bank 0 Registers                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_SelectBank(module_num, SMB_BANK_0);

    HAL_PRINT("SMB%1XADDR3           = 0x%02X\n", smbf_module, REG_READ(SMBADDR3(module_num)));
    HAL_PRINT("SMB%1XADDR7           = 0x%02X\n", smbf_module, REG_READ(SMBADDR7(module_num)));
    HAL_PRINT("SMB%1XADDR4           = 0x%02X\n", smbf_module, REG_READ(SMBADDR4(module_num)));
    HAL_PRINT("SMB%1XADDR8           = 0x%02X\n", smbf_module, REG_READ(SMBADDR8(module_num)));
    HAL_PRINT("SMB%1XADDR5           = 0x%02X\n", smbf_module, REG_READ(SMBADDR5(module_num)));
    HAL_PRINT("SMB%1XADDR9           = 0x%02X\n", smbf_module, REG_READ(SMBADDR9(module_num)));
    HAL_PRINT("SMB%1XADDR6           = 0x%02X\n", smbf_module, REG_READ(SMBADDR6(module_num)));
    HAL_PRINT("SMB%1XADDR10          = 0x%02X\n", smbf_module, REG_READ(SMBADDR10(module_num)));
    HAL_PRINT("SMB%1XCST2            = 0x%02X\n", smbf_module, REG_READ(SMBCST2(module_num)));
    HAL_PRINT("SMB%1XCST3            = 0x%02X\n", smbf_module, REG_READ(SMBCST3(module_num)));
    HAL_PRINT("SMB%1XCTL4            = 0x%02X\n", smbf_module, REG_READ(SMBCTL4(module_num)));
    HAL_PRINT("SMB%1XCTL5            = 0x%02X\n", smbf_module, REG_READ(SMBCTL5(module_num)));
    HAL_PRINT("SMB%1XSCLLT           = 0x%02X\n", smbf_module, REG_READ(SMBSCLLT(module_num)));
    HAL_PRINT("SMB%1XFIF_CTL         = 0x%02X\n", smbf_module, REG_READ(SMBFIF_CTL(module_num)));
    HAL_PRINT("SMB%1XSCLHT           = 0x%02X\n", smbf_module, REG_READ(SMBSCLHT(module_num)));
    HAL_PRINT("SMB%1X_VER            = 0x%02X\n", smbf_module, REG_READ(SMB_VER(module_num)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Bank 1 Registers                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_SelectBank(module_num, SMB_BANK_1);

    HAL_PRINT("SMB%1XFIF_CTS         = 0x%02X\n", smbf_module, REG_READ(SMBFIF_CTS(module_num)));
    HAL_PRINT("SMB%1XTXF_CTL         = 0x%02X\n", smbf_module, REG_READ(SMBTXF_CTL(module_num)));
#ifdef SMB_CAPABILITY_START_TO_STOP_TIMEOUT_SUPPORT
    HAL_PRINT("SMB%1X_FRTO           = 0x%02X\n", smbf_module, REG_READ(SMB_FRTO(module_num)));
#endif
#ifdef SMB_CAPABILITY_HW_PEC_SUPPORT
    HAL_PRINT("SMB%1XPEC             = 0x%02X\n", smbf_module, REG_READ(SMBPEC(module_num)));
#endif
    HAL_PRINT("SMB%1XTXF_STS         = 0x%02X\n", smbf_module, REG_READ(SMBTXF_STS(module_num)));
    HAL_PRINT("SMB%1XRXF_STS         = 0x%02X\n", smbf_module, REG_READ(SMBRXF_STS(module_num)));
    HAL_PRINT("SMB%1XRXF_CTL         = 0x%02X\n", smbf_module, REG_READ(SMBRXF_CTL(module_num)));

    HAL_PRINT("\n");
}

#ifdef HAL_PRINT_CAPABILITY
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_OperationStateStr                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  operatioStateType - The enumerator type to be converted to string                      */
/*                                                                                                         */
/* Returns:         A string of operatioStateType                                                          */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine retrieves the string which represents the given SMB_OPERATION_STATE_T     */
/*                  enumerator                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
static const char* SMB_OperationStateStr (SMB_OPERATION_STATE_T operatioStateType)
{
    switch (operatioStateType)
    {
    case    SMB_DISABLE:        return "SMB_DISABLE";
    case    SMB_IDLE:           return "SMB_IDLE";
    case    SMB_MASTER_START:   return "SMB_MASTER_START";
    case    SMB_SLAVE_MATCH:    return "SMB_SLAVE_MATCH";
    case    SMB_OPER_STARTED:   return "SMB_OPER_STARTED";
    default:                    return "";
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_OperationStr                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  operationType - The enumerator type to be converted to string                          */
/*                                                                                                         */
/* Returns:         A string of operationType                                                              */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine retrieves the string which represents the given SMB_OPERATION_T           */
/*                  enumerator                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
static const char* SMB_OperationStr (SMB_OPERATION_T operationType)
{
    switch (operationType)
    {
    case    SMB_NO_OPER:        return "SMB_NO_OPER";
    case    SMB_WRITE_OPER:     return "SMB_WRITE_OPER";
    case    SMB_READ_OPER:      return "SMB_READ_OPER";
    default:                    return "";
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_ModeStr                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  modeType - The enumerator type to be converted to string                               */
/*                                                                                                         */
/* Returns:         A string of modeType                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine retrieves the string which represents the given SMB_MODE_T                */
/*                  enumerator                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
static const char* SMB_ModeStr (SMB_MODE_T modeType)
{
    switch (modeType)
    {
    case    SMB_SLAVE:          return "SMB_SLAVE";
    case    SMB_MASTER:         return "SMB_MASTER";
    default:                    return "";
    }
}

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_StateIndStr                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  stateIndType - The enumerator type to be converted to string                           */
/*                                                                                                         */
/* Returns:         A string of stateIndType                                                               */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine retrieves the string which represents the given SMB_STATE_IND_T           */
/*                  enumerator                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
static const char* SMB_StateIndStr (SMB_STATE_IND_T stateIndType)
{
    switch (stateIndType)
    {
    case    SMB_NO_STATUS_IND:                  return "SMB_NO_STATUS_IND";
    case    SMB_SLAVE_RCV_IND:                  return "SMB_SLAVE_RCV_IND";
    case    SMB_SLAVE_XMIT_IND:                 return "SMB_SLAVE_XMIT_IND";
#ifdef SMB_WRAP_AROUND_BUFFER
    case    SMB_SLAVE_XMIT_MISSING_DATA_IND:    return "SMB_SLAVE_XMIT_MISSING_DATA_IND";
#endif
    case    SMB_SLAVE_RESTART_IND:              return "SMB_SLAVE_RESTART_IND";
    case    SMB_SLAVE_DONE_IND:                 return "SMB_SLAVE_DONE_IND";
    case    SMB_MASTER_DONE_IND:                return "SMB_MASTER_DONE_IND";
    case    SMB_NO_DATA_IND:                    return "SMB_NO_DATA_IND";
    case    SMB_NACK_IND:                       return "SMB_NACK_IND";
    case    SMB_BUS_ERR_IND:                    return "SMB_BUS_ERR_IND";
    case    SMB_WAKE_UP_IND:                    return "SMB_WAKE_UP_IND";
    case    SMB_MASTER_PEC_ERR_IND:             return "SMB_MASTER_PEC_ERR_IND";
    case    SMB_MASTER_BLOCK_BYTES_ERR_IND:     return "SMB_MASTER_BLOCK_BYTES_ERR_IND";
    case    SMB_SLAVE_PEC_ERR_IND:              return "SMB_SLAVE_PEC_ERR_IND";
    default:                                    return "";
    }
}
#endif
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_InterruptHandler                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine responds to SMB interrupts.                                               */
/*                  Checks the interrupt source and updates the SMB state accordingly.                     */
/*                  When necessary, invokes a callback function.                                           */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_InterruptHandler (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* A Negative Acknowledge has occurred                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    if (READ_REG_FIELD(SMBST(module_num), SMBST_NEGACK))
    {
        SMB_HandleNegativeAck(module_num);
        return;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* A Bus Error has been identified                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    if (READ_REG_FIELD(SMBST(module_num), SMBST_BER))
    {
        SMB_HandleBusError(module_num);
        return;
    }

#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
    /*-----------------------------------------------------------------------------------------------------*/
    /* A Bus Timeout has been identified (bus timeout interrupt is on and bus timeout status is set)       */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((READ_REG_FIELD(SMBT_OUT(module_num), SMBT_OUT_T_OUTIE) &&
        READ_REG_FIELD(SMBT_OUT(module_num), SMBT_OUT_T_OUTST))
#ifdef SMB_CAPABILITY_START_TO_STOP_TIMEOUT_SUPPORT
        || ((READ_REG_FIELD(SMB_FRTO(module_num), SMB_FRTO_FRTOIE) &&
        READ_REG_FIELD(SMB_FRTO(module_num), SMB_FRTO_FRTOST)))
#endif
        )
    {
        SMB_HandleTimeout(module_num);
        return;
    }
#endif

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
    /*-----------------------------------------------------------------------------------------------------*/
    /* A Master End of Busy (meaning Stop Condition happened - End of Busy interrupt is on and End of Busy */
    /* is set)                                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (READ_REG_FIELD(SMBCTL1(module_num), SMBCTL1_EOBINTE) &&
        READ_REG_FIELD(SMBCST3(module_num), SMBCST3_EO_BUSY))
    {
        SMB_HandleEndOfBusy(module_num);
        return;
    }
#endif

#ifdef SMB_SLAVE_SUPPORT
    /*-----------------------------------------------------------------------------------------------------*/
    /* A Slave Stop Condition has been identified                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    if (READ_REG_FIELD(SMBST(module_num), SMBST_SLVSTP))
    {
        SMB_HandleSlaveStop(module_num);
        return;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* A Slave Restart Condition has been identified                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    if (SMB_status[module_num].fifo_use && READ_REG_FIELD(SMBFIF_CTS(module_num), SMBFIF_CTS_SLVRSTR))
    {
        SMB_HandleSlaveRestart(module_num);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* A Slave Address Match has been identified                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    if (READ_REG_FIELD(SMBST(module_num), SMBST_NMATCH))
    {
        if (!SMB_HandleSlaveAddressMatch(module_num))
        {
            return;
        }
    }
#endif  /* SMB_SLAVE_SUPPORT */

#ifdef SMB_MASTER_SUPPORT
    /*-----------------------------------------------------------------------------------------------------*/
    /* Address sent and requested stall occurred (Master mode)                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (READ_REG_FIELD(SMBST(module_num), SMBST_STASTR))
    {
        SMB_HandleStallAfterStart(module_num);
        return;
    }
#endif  /* SMB_MASTER_SUPPORT */

    /*-----------------------------------------------------------------------------------------------------*/
    /* SDA status is set - transmit or receive, master or slave                                            */
    /* Status Bit is cleared by writing to or reading from SDA (depending on current direction)            */
    /*-----------------------------------------------------------------------------------------------------*/
    if (READ_REG_FIELD(SMBST(module_num), SMBST_SDAST) ||
        (SMB_status[module_num].fifo_use &&
        (READ_REG_FIELD(SMBRXF_STS(module_num), SMBRXF_STS_RX_THST) ||
         READ_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_THST))))
    {
#ifdef SMB_MASTER_SUPPORT
        /*-------------------------------------------------------------------------------------------------*/
        /* Handle successful bus mastership                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        if (SMB_status[module_num].master_or_slave == SMB_MASTER)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* SMB state is IDLE                                                                           */
            /*---------------------------------------------------------------------------------------------*/
            if (SMB_status[module_num].operation_state == SMB_IDLE)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Perform SMB recovery in Master mode, where state is IDLE, which is an illegal state     */
                /*-----------------------------------------------------------------------------------------*/
                SMB_MasterAbort(module_num);
                EXECUTE_FUNC(SMB_callback, (module_num, SMB_BUS_ERR_IND, SMB_GetIndex(module_num)));
                return;
            }
            /*---------------------------------------------------------------------------------------------*/
            /* A Master Start has been identified                                                          */
            /*---------------------------------------------------------------------------------------------*/
            else if (SMB_status[module_num].operation_state == SMB_MASTER_START)
            {
                SMB_HandleMasterStart(module_num);
            }
            /*---------------------------------------------------------------------------------------------*/
            /* SDA status is set - transmit or receive: Handle master mode                                 */
            /*---------------------------------------------------------------------------------------------*/
            else if (SMB_status[module_num].operation_state == SMB_OPER_STARTED)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Perform master write                                                                    */
                /*-----------------------------------------------------------------------------------------*/
                if (SMB_status[module_num].operation == SMB_WRITE_OPER)
                {
                    SMB_HandleMasterWrite(module_num);
                }
                /*-----------------------------------------------------------------------------------------*/
                /* Perform master read                                                                     */
                /*-----------------------------------------------------------------------------------------*/
                else if (SMB_status[module_num].operation == SMB_READ_OPER)
                {
                    SMB_HandleMasterRead(module_num);
                }
            }
        }
#ifdef SMB_SLAVE_SUPPORT
        else
#endif
#endif  /* SMB_MASTER_SUPPORT */

#ifdef SMB_SLAVE_SUPPORT
        /*-------------------------------------------------------------------------------------------------*/
        /* SDA status is set - transmit or receive: Handle slave mode                                      */
        /*-------------------------------------------------------------------------------------------------*/
        if (SMB_status[module_num].master_or_slave == SMB_SLAVE)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Perform slave read                                                                          */
            /*---------------------------------------------------------------------------------------------*/
            if ((SMB_status[module_num].operation == SMB_READ_OPER))
            {
                SMB_HandleSlaveRead(module_num);
            }
            /*---------------------------------------------------------------------------------------------*/
            /* Perform slave write                                                                         */
            /*---------------------------------------------------------------------------------------------*/
            else
            {
                SMB_HandleSlaveWrite(module_num);
            }
        }
#endif  /* SMB_SLAVE_SUPPORT */
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleNegativeAck                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles negative acknowledge.                                             */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleNegativeAck (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    if (SMB_status[module_num].fifo_use)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* if there are still untransmitted bytes in TX FIFO reduce them from write_index                  */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_status[module_num].write_index -= READ_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_BYTES);

        /*-------------------------------------------------------------------------------------------------*/
        /* clear the FIFO                                                                                  */
        /*-------------------------------------------------------------------------------------------------*/
        REG_WRITE(SMBFIF_CTS(module_num), MASK_FIELD(SMBFIF_CTS_CLR_FIFO));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* In slave write operation, NACK is OK, otherwise it is a problem.                                    */
    /* So if not slave, or number of bytes sent to master less than required - notify upper layer.         */
    /* If we are slave - the upper layer should still wait for a Slave Stop.                               */
    /*-----------------------------------------------------------------------------------------------------*/
    if (!((SMB_status[module_num].master_or_slave == SMB_SLAVE) &&
          (SMB_status[module_num].write_index != 0) &&
          (SMB_status[module_num].write_index == SMB_status[module_num].write_size)))
    {
#ifdef SMB_MASTER_SUPPORT
        if ((SMB_status[module_num].master_or_slave == SMB_MASTER) &&
            READ_REG_FIELD(SMBST(module_num), SMBST_MASTER))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Only current master is allowed to issue Stop Condition                                      */
            /*---------------------------------------------------------------------------------------------*/
            SMB_MasterAbort(module_num);
        }
#endif  /* SMB_MASTER_SUPPORT */

        SMB_status[module_num].operation_state = SMB_IDLE;
        EXECUTE_FUNC(SMB_callback, (module_num, SMB_NACK_IND, SMB_status[module_num].write_index));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* else:                                                                                               */
    /* Slave has to wait for SMB_STOP to decide this is the end of the transaction.                        */
    /* Therefore transaction is not yet considered as done                                                 */
    /*                                                                                                     */
    /* In Master mode, NEGACK should be cleared only after generating STOP.                                */
    /* In such case, the bus is released from stall only after the software clears NEGACK bit.             */
    /* Then a Stop condition is sent.                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_NEGACK));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleBusError                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Bus Error.                                                        */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleBusError (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check whether bus arbitration or Start or Stop during data transfer                                 */
    /*-----------------------------------------------------------------------------------------------------*/
#ifdef SMB_MASTER_SUPPORT
    if ((SMB_status[module_num].master_or_slave == SMB_MASTER))
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Bus arbitration problem should not result in recovery                                           */
        /*-------------------------------------------------------------------------------------------------*/
        if (!READ_REG_FIELD(SMBST(module_num), SMBST_MASTER) &&
            (SMB_status[module_num].write_index == 0) &&
            (SMB_status[module_num].read_index == 0))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Bus arbitration loss                                                                        */
            /*---------------------------------------------------------------------------------------------*/
            if (--SMB_status[module_num].retry_count > 0)
            {
                if (SMB_status[module_num].operation_state == SMB_MASTER_START)
                {
                    /*-------------------------------------------------------------------------------------*/
                    /* Only if the arbitration loss is after master start transaction and before sending   */
                    /* any data, Perform a retry (generate a Start condition as soon as the SMBus is free) */
                    /*-------------------------------------------------------------------------------------*/
                    REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_BER));
                    SMB_Start(module_num);
                }
                else
                {
                    /*-------------------------------------------------------------------------------------*/
                    /* In all other cases, it is an illegal bus arbitration loss (other master initiate a  */
                    /* transaction while we already started a transaction), thus abort the transaction.    */
                    /*-------------------------------------------------------------------------------------*/
                    SMB_MasterAbortWithoutStop(module_num);
                    EXECUTE_FUNC(SMB_callback, (module_num, SMB_NACK_IND, SMB_GetIndex(module_num)));
                }
                return;
            }
        }
        else
        {
            SMB_MasterAbort(module_num);
        }
    }
#ifdef SMB_SLAVE_SUPPORT
    else
#endif
#endif  /* SMB_MASTER_SUPPORT */

#ifdef SMB_SLAVE_SUPPORT
    if (SMB_status[module_num].master_or_slave == SMB_SLAVE)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Reset the module                                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_Reset(module_num);
    }
#endif  /* SMB_SLAVE_SUPPORT */

    REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_BER));
    SMB_status[module_num].operation_state = SMB_IDLE;
    EXECUTE_FUNC(SMB_callback, (module_num, SMB_BUS_ERR_IND, SMB_GetIndex(module_num)));
}

#ifdef SMB_CAPABILITY_TIMEOUT_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleTimeout                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Bus Timeout.                                                      */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleTimeout (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

#ifdef SMB_MASTER_SUPPORT
    if (SMB_status[module_num].master_or_slave == SMB_MASTER)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Only current master is allowed to issue Stop Condition                                          */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_MasterAbort(module_num);
    }
#ifdef SMB_SLAVE_SUPPORT
    else
#endif
#endif  /* SMB_MASTER_SUPPORT */

#ifdef SMB_SLAVE_SUPPORT
    if (SMB_status[module_num].master_or_slave == SMB_SLAVE)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Reset the module                                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_Reset(module_num);
    }
#endif  /* SMB_SLAVE_SUPPORT */

    SET_REG_FIELD(SMBT_OUT(module_num), SMBT_OUT_T_OUTST, 1);
#ifdef SMB_CAPABILITY_START_TO_STOP_TIMEOUT_SUPPORT
    SET_REG_FIELD(SMB_FRTO(module_num), SMB_FRTO_FRTOST, 1);
#endif
    SMB_status[module_num].operation_state = SMB_IDLE;
    EXECUTE_FUNC(SMB_callback, (module_num, SMB_BUS_ERR_IND, SMB_GetIndex(module_num)));
}
#endif

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleEndOfBusy                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Master End of Busy.                                               */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleEndOfBusy (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable "End of Busy" interrupt                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_EOBINTE, 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear EO_BUSY pending bit                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCST3(module_num), SMBCST3_EO_BUSY, 1);

    SMB_status[module_num].operation_state = SMB_IDLE;

    if ((SMB_status[module_num].write_size == SMB_BYTES_QUICK_PROT) ||
        (SMB_status[module_num].read_size  == SMB_BYTES_QUICK_PROT) ||
        (SMB_status[module_num].read_size  == 0))
    {
        EXECUTE_FUNC(SMB_callback, (module_num, SMB_status[module_num].stop_indication, 0));
    }
    else
    {
        EXECUTE_FUNC(SMB_callback, (module_num, SMB_status[module_num].stop_indication, SMB_status[module_num].read_index));
    }
}
#endif

#ifdef SMB_SLAVE_SUPPORT
#ifdef SMB_RECOVERY_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_SlaveAbort                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine cleans up the SMB interface and registers                                 */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_SlaveAbort (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable interrupt.                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_InterruptEnable(module_num, FALSE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Dummy read to clear interface.                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    (void)REG_READ(SMBSDA(module_num));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear NMATCH and BER bits by writing 1s to them.                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBST(module_num), SMBST_BER, TRUE);
    SET_REG_FIELD(SMBST(module_num), SMBST_NMATCH, TRUE);

#ifdef SMB_STALL_TIMEOUT_SUPPORT
    SMB_RESET_STALL_COUNTER(module_num);
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Reset driver status                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_status[module_num].operation_state = SMB_IDLE;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Disable SMB Module                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(SMBCTL2(module_num), SMBCTL2_ENABLE, DISABLE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Delay 100 us                                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    DELAY_LOOP(500);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable SMB Module                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    (void)SMB_Enable(module_num);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable interrupt.                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_InterruptEnable(module_num, TRUE);
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleSlaveStop                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Slave Stop Condition.                                             */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleSlaveStop (SMB_MODULE_T module_num)
{
    SMB_STATE_IND_T ind;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    ASSERT(SMB_status[module_num].master_or_slave == SMB_SLAVE);
    REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_SLVSTP));

    SMB_status[module_num].operation_state = SMB_IDLE;

    if (SMB_status[module_num].fifo_use)
    {
        if (SMB_status[module_num].operation == SMB_READ_OPER)
        {
            SMB_ReadFromFifo(module_num, READ_REG_FIELD(SMBRXF_STS(module_num), SMBRXF_STS_RX_BYTES));

            /*---------------------------------------------------------------------------------------------*/
            /* if PEC is not used or PEC is used and PEC is correct                                        */
            /*---------------------------------------------------------------------------------------------*/
            if (SMBUS_PEC_IS_CORRECT(module_num))
            {
                ind = SMB_SLAVE_DONE_IND;
            }
            /*---------------------------------------------------------------------------------------------*/
            /* PEC value is not correct                                                                    */
            /*---------------------------------------------------------------------------------------------*/
            else
            {
                ind = SMB_SLAVE_PEC_ERR_IND;
            }

            /*---------------------------------------------------------------------------------------------*/
            /* Notify upper layer that illegal data received                                               */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(SMB_callback, (module_num, ind, SMB_status[module_num].read_index));
        }

        if (SMB_status[module_num].operation == SMB_WRITE_OPER)
        {
            EXECUTE_FUNC(SMB_callback, (module_num, SMB_SLAVE_DONE_IND, SMB_status[module_num].write_index));
        }

        SET_REG_MASK(SMBFIF_CTS(module_num), MASK_FIELD(SMBFIF_CTS_SLVRSTR)  |
                                             MASK_FIELD(SMBFIF_CTS_CLR_FIFO) |
                                             MASK_FIELD(SMBFIF_CTS_RXF_TXE));
    }
    /*-----------------------------------------------------------------------------------------------------*/
    /* FIFO is not used                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    else
    {
        if (SMB_status[module_num].operation == SMB_READ_OPER)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* if PEC is not used or PEC is used and PEC is correct                                        */
            /*---------------------------------------------------------------------------------------------*/
            if (SMBUS_PEC_IS_CORRECT(module_num))
            {
                ind = SMB_SLAVE_DONE_IND;
            }
            /*---------------------------------------------------------------------------------------------*/
            /* PEC value is not correct                                                                    */
            /*---------------------------------------------------------------------------------------------*/
            else
            {
                ind = SMB_SLAVE_PEC_ERR_IND;
            }

            /*---------------------------------------------------------------------------------------------*/
            /* Notify upper layer of missing data or all data received                                     */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(SMB_callback, (module_num, ind, SMB_status[module_num].read_index));
        }
        else
        {
            EXECUTE_FUNC(SMB_callback, (module_num, SMB_SLAVE_DONE_IND, SMB_status[module_num].write_index));
        }
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleSlaveRestart                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Slave Restart Condition.                                          */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleSlaveRestart (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    ASSERT(SMB_status[module_num].master_or_slave == SMB_SLAVE);

    if (SMB_status[module_num].operation == SMB_READ_OPER)
    {
        SMB_ReadFromFifo(module_num, READ_REG_FIELD(SMBRXF_STS(module_num), SMBRXF_STS_RX_BYTES));
    }
    REG_WRITE(SMBFIF_CTS(module_num), MASK_FIELD(SMBFIF_CTS_SLVRSTR));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleSlaveAddressMatch                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         TRUE if SMB_InterruptHandler should continue; FALSE otherwise                          */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Slave Address Match.                                              */
/*---------------------------------------------------------------------------------------------------------*/
static BOOLEAN SMB_HandleSlaveAddressMatch (SMB_MODULE_T module_num)
{
    BOOLEAN         slave_tx;
    SMB_STATE_IND_T ind     = SMB_NO_STATUS_IND;
    UINT8           info    = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    if (SMB_status[module_num].fifo_use == FALSE)
    {
        REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_NMATCH));
    }

    if (READ_REG_FIELD(SMBST(module_num), SMBST_XMIT))
    {
        slave_tx = TRUE;
    }
    else
    {
        slave_tx = FALSE;
    }

    if (SMB_status[module_num].operation_state == SMB_IDLE)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Indicate Slave Mode                                                                             */
        /*-------------------------------------------------------------------------------------------------*/
        if (slave_tx)
        {
            ind = SMB_SLAVE_XMIT_IND;
        }
        else
        {
            ind = SMB_SLAVE_RCV_IND;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Check which type of address match                                                               */
        /*-------------------------------------------------------------------------------------------------*/
        if (READ_REG_FIELD(SMBCST(module_num), SMBCST_MATCH))
        {
            UINT16 address_match = ((REG_READ(SMBCST3(module_num)) & 0x07) << 7) |
                                    (REG_READ(SMBCST2(module_num)) & 0x7F);
            info = 0;
            ASSERT(address_match);
            while (address_match)
            {
                if (address_match & 1)
                {
                    break;
                }
                info++;
                address_match = address_match >> 1;
            }

            SMB_CurSlaveAddr[module_num] = READ_VAR_FIELD(SMB_GetSlaveAddress_l(module_num, (SMB_ADDR_T)info), SMBADDRx_ADDR);

            if (READ_VAR_BIT(SMB_status[module_num].PEC_mask, info) == 1)
            {
                SMB_status[module_num].PEC_use = TRUE;
                SMB_status[module_num].crc_data = 0;
                if (slave_tx)
                {
                    SMB_CalcPEC(module_num, (SMB_CurSlaveAddr[module_num] & 0x7F) << 1 | 1);
                }
                else
                {
                    SMB_CalcPEC(module_num, (SMB_CurSlaveAddr[module_num] & 0x7F) << 1);
                }
            }
            else
            {
                SMB_status[module_num].PEC_use = FALSE;
            }
        }
        else
        {
            if (READ_REG_FIELD(SMBCST(module_num), SMBCST_GCMATCH))
            {
                info = (UINT8)SMB_GC_ADDR;
                SMB_CurSlaveAddr[module_num] = 0;
            }
            else
            {
                if (READ_REG_FIELD(SMBCST(module_num), SMBCST_ARPMATCH))
                {
                    info = (UINT8)SMB_ARP_ADDR;
                    SMB_CurSlaveAddr[module_num] = 0x61;
                }
            }
        }
    }
    else
    {
        /*-------------------------------------------------------------------------------------------------*/
        /*  Slave match can happen in two options:                                                         */
        /*  1. Start, SA, read    (slave read without further ado).                                        */
        /*  2. Start, SA, read,  data, restart, SA, read, ... (salve read in fragmented mode)              */
        /*  3. Start, SA, write, data, restart, SA, read, ... (regular write-read mode)                    */
        /*-------------------------------------------------------------------------------------------------*/
        if (((SMB_status[module_num].operation_state == SMB_OPER_STARTED) &&
             (SMB_status[module_num].operation == SMB_READ_OPER) &&
             (SMB_status[module_num].master_or_slave == SMB_SLAVE) &&
             slave_tx) ||
            ((SMB_status[module_num].master_or_slave == SMB_SLAVE) &&
             !slave_tx))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* slave transmit after slave receive w/o Slave Stop implies repeated start                    */
            /*---------------------------------------------------------------------------------------------*/
            ind = SMB_SLAVE_RESTART_IND;
            info = (UINT8)(SMB_status[module_num].read_index);
            SMB_CalcPEC(module_num,(SMB_CurSlaveAddr[module_num] & 0x7F) << 1 | 1);
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Address match automatically implies slave mode                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    ASSERT(! READ_REG_FIELD(SMBST(module_num), SMBST_MASTER));
    SMB_status[module_num].master_or_slave = SMB_SLAVE;
    SMB_status[module_num].operation_state = SMB_SLAVE_MATCH;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Notify upper layer                                                                                  */
    /* Upper layer must at this stage call the driver routine for slave tx or rx,                          */
    /* to eliminate a condition of slave being notified but not yet starting                               */
    /* transaction - and thus an endless interrupt from SDAST for the slave RCV or TX !                    */
    /*-----------------------------------------------------------------------------------------------------*/
    EXECUTE_FUNC(SMB_callback, (module_num, ind, info));

#ifdef SMB_RECOVERY_SUPPORT
    /*-----------------------------------------------------------------------------------------------------*/
    /* By now, SMB operation state should have been changed from MATCH to SMB_OPER_STARTED.                */
    /* If state hasn't been changed already, this may suggest that the SMB slave is not ready to           */
    /* transmit or receive data.                                                                           */
    /*                                                                                                     */
    /* In addition, when using FIFO, NMATCH bit is cleared only when moving to SMB_OPER_STARTED state.     */
    /* If NMATCH is not cleared, we would get an endless SMB interrupt.                                    */
    /* Therefore, Abort the slave, such that SMB HW and state machine return to default, functional state. */
    /*-----------------------------------------------------------------------------------------------------*/
    if (SMB_status[module_num].operation_state == SMB_SLAVE_MATCH)
    {
        SMB_SlaveAbort(module_num);
        EXECUTE_FUNC(SMB_callback, (module_num, SMB_BUS_ERR_IND, SMB_GetIndex(module_num)));
        return FALSE;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Slave abort data                                                                                    */
    /* if the SMBus's status is not match current status register of XMIT                                  */
    /* the Slave device will enter dead-lock and stall bus forever                                         */
    /* Add this check rule to avoid this condition                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((SMB_status[module_num].operation == SMB_READ_OPER  && ind == SMB_SLAVE_XMIT_IND) ||
        (SMB_status[module_num].operation == SMB_WRITE_OPER && ind == SMB_SLAVE_RCV_IND))
    {
        SMB_SlaveAbort(module_num);
        EXECUTE_FUNC(SMB_callback, (module_num, SMB_BUS_ERR_IND, SMB_GetIndex(module_num)));
        return FALSE;
    }
#endif
    /*-----------------------------------------------------------------------------------------------------*/
    /* If none of the above - BER should occur                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    return TRUE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleSlaveRead                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Slave Read operation.                                             */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleSlaveRead (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform slave read. No need to distinguish between last byte and the rest of the bytes.             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (SMB_status[module_num].fifo_use == FALSE)
    {
        UINT8 data;

        (void)SMB_ReadByte(module_num, &data);
        if (SMB_status[module_num].read_index < SMB_status[module_num].read_size)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Keep read data                                                                              */
            /*---------------------------------------------------------------------------------------------*/
            SMB_status[module_num].read_data_buf[SMB_status[module_num].read_index++] = data;
            if ((SMB_status[module_num].read_index == 1) && SMB_status[module_num].read_size == SMB_BYTES_BLOCK_PROT)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* First byte indicates length in block protocol                                           */
                /*-----------------------------------------------------------------------------------------*/
                SMB_status[module_num].read_size = data;
            }
        }
    }
    /*-----------------------------------------------------------------------------------------------------*/
    /* FIFO is used                                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    else
    {
        if (READ_REG_FIELD(SMBRXF_STS(module_num), SMBRXF_STS_RX_THST))
        {
            SMB_ReadFromFifo(module_num, READ_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR));

            /*---------------------------------------------------------------------------------------------*/
            /* clear the status bits                                                                       */
            /*---------------------------------------------------------------------------------------------*/
            SET_REG_FIELD(SMBRXF_STS(module_num), SMBRXF_STS_RX_THST, 1);
        }
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleSlaveWrite                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Slave Write operation.                                            */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleSlaveWrite (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* More bytes to write                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((SMB_status[module_num].operation == SMB_WRITE_OPER) &&
        (SMB_status[module_num].write_index < SMB_status[module_num].write_size))
    {
        if (SMB_status[module_num].fifo_use == FALSE)
        {
            if ((SMB_status[module_num].write_index + 1 == SMB_status[module_num].write_size) &&
                SMB_status[module_num].PEC_use == TRUE)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Send PEC byte                                                                           */
                /*-----------------------------------------------------------------------------------------*/
                SMBUS_SEND_PEC_BYTE(module_num);
            }
            else if (SMB_status[module_num].write_index < SMB_status[module_num].write_size)
            {
                SMB_WriteByte(module_num, SMB_status[module_num].write_data_buf[SMB_status[module_num].write_index]);
            }
            SMB_status[module_num].write_index++;
        }
        /*-------------------------------------------------------------------------------------------------*/
        /* FIFO is used                                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        else
        {
            UINT16 wcount;
            wcount =  (SMB_status[module_num].write_size - SMB_status[module_num].write_index);

#ifdef SMB_CAPABILITY_HW_PEC_SUPPORT
            /*---------------------------------------------------------------------------------------------*/
            /* Put the PEC byte last after the FIFO becomes empty.                                         */
            /*---------------------------------------------------------------------------------------------*/
            if ((SMB_status[module_num].PEC_use) && (wcount > 1))
            {
                wcount--;
            }
#endif

            if (wcount > SMBUS_FIFO_SIZE)
            {
                wcount = SMBUS_FIFO_SIZE;
            }
            REG_WRITE(SMBTXF_CTL(module_num), (UINT8)wcount);
            SMB_WriteToFifo(module_num, wcount);

            /*---------------------------------------------------------------------------------------------*/
            /* clear the status bits                                                                       */
            /*---------------------------------------------------------------------------------------------*/
            SET_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_THST, 1);
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* If all bytes were written, ignore further master read requests.                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    else
    {
#if !defined(SMB_WRAP_AROUND_BUFFER)
        ASSERT(FALSE);
#endif
        if (SMB_status[module_num].fifo_use == FALSE)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Clear SDA Status bit                                                                        */
            /*---------------------------------------------------------------------------------------------*/
            if (SMB_status[module_num].write_index != 0)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Was writing                                                                             */
                /*-----------------------------------------------------------------------------------------*/
                SMB_WriteByte(module_num, 0xFF);
            }
            else
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Was reading                                                                             */
                /*-----------------------------------------------------------------------------------------*/
                UINT8 data;
                (void)SMB_ReadByte(module_num, &data);
            }
        }
        /*-------------------------------------------------------------------------------------------------*/
        /* write/read redundant bytes with FIFO (if there are any bytes to write)                          */
        /*-------------------------------------------------------------------------------------------------*/
        else
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Set threshold size                                                                          */
            /*---------------------------------------------------------------------------------------------*/
            REG_WRITE(SMBTXF_CTL(module_num), SMBUS_FIFO_SIZE);

            SMB_WriteToFifo(module_num, SMBUS_FIFO_SIZE);

            /*---------------------------------------------------------------------------------------------*/
            /* Clear the status bits                                                                       */
            /*---------------------------------------------------------------------------------------------*/
            SET_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_THST, 1);
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Notify upper layer of transaction completion                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        EXECUTE_FUNC(SMB_callback, (module_num, SMB_NO_DATA_IND, SMB_status[module_num].read_index));
    }
    /*-----------------------------------------------------------------------------------------------------*/
    /* All bytes sent/received                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
}
#endif  /* SMB_SLAVE_SUPPORT */

#ifdef SMB_MASTER_SUPPORT
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleStallAfterStart                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Master Stall After Start (successful completion of the sending    */
/*                  of an address).                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleStallAfterStart (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    ASSERT(READ_REG_FIELD(SMBST(module_num), SMBST_MASTER));
    ASSERT(SMB_status[module_num].master_or_slave == SMB_MASTER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check for Quick Command SMBus protocol                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((SMB_status[module_num].write_size == SMB_BYTES_QUICK_PROT) ||
        (SMB_status[module_num].read_size  == SMB_BYTES_QUICK_PROT))
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* No need to write any data bytes - reached here only in Quick Command                            */
        /*-------------------------------------------------------------------------------------------------*/

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
        /*-------------------------------------------------------------------------------------------------*/
        /* Enable "End of Busy" interrupt before issuing a STOP condition.                                 */
        /*-------------------------------------------------------------------------------------------------*/
        SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_EOBINTE, 1);
#endif
        SMB_Stop(module_num);

        /*-------------------------------------------------------------------------------------------------*/
        /* Update status                                                                                   */
        /*-------------------------------------------------------------------------------------------------*/
#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
        SMB_status[module_num].operation_state = SMB_STOP_PENDING;
        SMB_status[module_num].stop_indication = SMB_MASTER_DONE_IND;
#else
        SMB_status[module_num].operation_state = SMB_IDLE;

        /*-------------------------------------------------------------------------------------------------*/
        /* Notify upper layer                                                                              */
        /*-------------------------------------------------------------------------------------------------*/
        EXECUTE_FUNC(SMB_callback, (module_num, SMB_MASTER_DONE_IND, 0));
#endif
    }
    else if (SMB_status[module_num].read_size == 1)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Receiving one byte only - set NACK after ensuring slave ACKed the address byte                  */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_Nack(module_num);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Reset stall-after-address-byte                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    SMB_StallAfterStart(module_num, FALSE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear stall only after setting STOP                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(SMBST(module_num), MASK_FIELD(SMBST_STASTR));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleMasterStart                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Master Start.                                                     */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleMasterStart (SMB_MODULE_T module_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    if (READ_REG_FIELD(SMBST(module_num), SMBST_MASTER))
    {
        UINT8 addr_byte = SMB_status[module_num].dest_addr;
        SMB_status[module_num].crc_data = 0;

        /*-------------------------------------------------------------------------------------------------*/
        /* Check for Quick Command SMBus protocol                                                          */
        /*-------------------------------------------------------------------------------------------------*/
        if ((SMB_status[module_num].write_size == SMB_BYTES_QUICK_PROT) ||
            (SMB_status[module_num].read_size  == SMB_BYTES_QUICK_PROT))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Need to stall after successful completion of sending address byte                           */
            /*---------------------------------------------------------------------------------------------*/
            SMB_StallAfterStart(module_num, TRUE);
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Prepare address byte                                                                            */
        /*-------------------------------------------------------------------------------------------------*/
        if (SMB_status[module_num].write_size == 0)
        {
            if (SMB_status[module_num].read_size == 1)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Receiving one byte only - stall after successful completion of sending address byte.    */
                /* If we NACK here, and slave doesn't ACK the address, we might unintentionally NACK the   */
                /* next multi-byte read                                                                    */
                /*-----------------------------------------------------------------------------------------*/
                SMB_StallAfterStart(module_num, TRUE);
            }

            /*---------------------------------------------------------------------------------------------*/
            /* Set direction to Read                                                                       */
            /*---------------------------------------------------------------------------------------------*/
            addr_byte |= (UINT8)0x1;
            SMB_status[module_num].operation = SMB_READ_OPER;
        }
        else
        {
            SMB_status[module_num].operation = SMB_WRITE_OPER;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Write the address to the bus                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_WriteByte(module_num, addr_byte);
        SMB_status[module_num].operation_state = SMB_OPER_STARTED;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleMasterRead                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Master Read operation.                                            */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleMasterRead (SMB_MODULE_T module_num)
{
    UINT16 block_zero_bytes;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Master read operation (pure read or following a write operation).                                   */
    /*-----------------------------------------------------------------------------------------------------*/

    /*-----------------------------------------------------------------------------------------------------*/
    /* Initialize number of bytes to include only the first byte (presents a case where                    */
    /* number of bytes to read is zero); add PEC if applicable                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    block_zero_bytes = 1;
    if (SMB_status[module_num].PEC_use == TRUE)
    {
        block_zero_bytes++;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform master read, distinguishing between last byte and the rest of the bytes.                    */
    /* The last byte should be read when the clock is stopped                                              */
    /*-----------------------------------------------------------------------------------------------------*/

    if ((SMB_status[module_num].read_index < (SMB_status[module_num].read_size - 1)) ||
         SMB_status[module_num].fifo_use == TRUE)
    {
        UINT8 data;

        /*-------------------------------------------------------------------------------------------------*/
        /* Byte to be read is not the last one                                                             */
        /* Check if byte-before-last is about to be read                                                   */
        /*-------------------------------------------------------------------------------------------------*/
        if ((SMB_status[module_num].read_index == (SMB_status[module_num].read_size - 2)) &&
             SMB_status[module_num].fifo_use == FALSE)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Set nack before reading byte-before-last, so that nack will be generated after receive      */
            /* of last byte                                                                                */
            /*---------------------------------------------------------------------------------------------*/
            SMB_Nack(module_num);
        }

        if (! READ_REG_FIELD(SMBST(module_num), SMBST_SDAST))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* No data available - reset state for new transaction                                         */
            /*---------------------------------------------------------------------------------------------*/
            SMB_status[module_num].operation_state = SMB_IDLE;

            /*---------------------------------------------------------------------------------------------*/
            /* Notify upper layer of transaction completion                                                */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(SMB_callback, (module_num, SMB_NO_DATA_IND, SMB_status[module_num].read_index));
        }
        else if (SMB_status[module_num].read_index == 0)
        {
            if (SMB_status[module_num].read_size == SMB_BYTES_EXCLUDE_BLOCK_SIZE_FROM_BUFFER ||
                SMB_status[module_num].read_size == SMB_BYTES_BLOCK_PROT)
            {
                (void)SMB_ReadByte(module_num, &data);

                /*-----------------------------------------------------------------------------------------*/
                /* First byte indicates length in block protocol                                           */
                /*-----------------------------------------------------------------------------------------*/
                if (SMB_status[module_num].read_size == SMB_BYTES_EXCLUDE_BLOCK_SIZE_FROM_BUFFER)
                {
                    SMB_status[module_num].read_size = data;
                }
                else
                {
                    SMB_status[module_num].read_data_buf[SMB_status[module_num].read_index++] = data;
                    SMB_status[module_num].read_size = data + 1;
                }

                if (SMB_status[module_num].PEC_use == TRUE)
                {
                    SMB_status[module_num].read_size += 1;
                    data += 1;
                }

                if (SMB_status[module_num].fifo_use == TRUE)
                {
                    SET_REG_FIELD(SMBRXF_STS(module_num), SMBRXF_STS_RX_THST, 1);
                    SET_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_THST, 1);
                    SET_REG_FIELD(SMBFIF_CTS(module_num), SMBFIF_CTS_RXF_TXE, 1);

                    if (data > SMBUS_FIFO_SIZE)
                    {
                        SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR, SMBUS_FIFO_SIZE);
                    }
                    else
                    {
                        if (data == 0)
                        {
                            data = 1;
                        }

                        /*---------------------------------------------------------------------------------*/
                        /* Clear the status bits                                                           */
                        /*---------------------------------------------------------------------------------*/
                        SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR, data);
                        SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_LAST_PEC, 1);
                    }
                }
            }
            else
            {
                if (SMB_status[module_num].fifo_use == FALSE)
                {
                    (void)SMB_ReadByte(module_num, &data);
                    SMB_status[module_num].read_data_buf[SMB_status[module_num].read_index++] = data;
                }
                else
                {
                    SET_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_THST, 1);
                    SMB_MasterFifoRead(module_num);
                }
            }

        }
        else
        {
            if (SMB_status[module_num].fifo_use == TRUE)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* FIFO in used.                                                                           */
                /*-----------------------------------------------------------------------------------------*/
                if ((SMB_status[module_num].read_size == block_zero_bytes) && (SMB_status[module_num].read_block_use == TRUE))
                {
#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
                    /*-------------------------------------------------------------------------------------*/
                    /* Enable "End of Busy" interrupt                                                      */
                    /*-------------------------------------------------------------------------------------*/
                    SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_EOBINTE, 1);
#endif
                    SMB_Stop(module_num);

                    SMB_ReadFromFifo(module_num, READ_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR));

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
                    SMB_status[module_num].operation_state = SMB_STOP_PENDING;
                    SMB_status[module_num].stop_indication = SMB_MASTER_BLOCK_BYTES_ERR_IND;
#else
                    /*-------------------------------------------------------------------------------------*/
                    /* Reset state for new transaction                                                     */
                    /*-------------------------------------------------------------------------------------*/
                    SMB_status[module_num].operation_state = SMB_IDLE;

                    /*-------------------------------------------------------------------------------------*/
                    /* Notify upper layer of transaction completion                                        */
                    /*-------------------------------------------------------------------------------------*/
                    EXECUTE_FUNC(SMB_callback, (module_num, SMB_MASTER_BLOCK_BYTES_ERR_IND, SMB_status[module_num].read_index));
#endif
                }
                else
                {
                    SMB_MasterFifoRead(module_num);
                }
            }
            else
            {
                (void)SMB_ReadByte(module_num, &data);
                SMB_status[module_num].read_data_buf[SMB_status[module_num].read_index++] = data;
            }
        }
    }
    else
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Last byte is about to be read - end of transaction.                                             */
        /* Stop should be set before reading last byte.                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        UINT8           data;
        SMB_STATE_IND_T ind = SMB_MASTER_DONE_IND;

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
        /*-------------------------------------------------------------------------------------------------*/
        /* Enable "End of Busy" interrupt.                                                                 */
        /*-------------------------------------------------------------------------------------------------*/
        SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_EOBINTE, 1);
#endif
        SMB_Stop(module_num);

        (void)SMB_ReadByte(module_num, &data);

        if ((SMB_status[module_num].read_size == block_zero_bytes) && (SMB_status[module_num].read_block_use == TRUE))
        {
            ind = SMB_MASTER_BLOCK_BYTES_ERR_IND;
        }
        else
        {
            SMB_status[module_num].read_data_buf[SMB_status[module_num].read_index++] = data;

            /*---------------------------------------------------------------------------------------------*/
            /* PEC value is not correct                                                                    */
            /*---------------------------------------------------------------------------------------------*/
            if (!SMBUS_PEC_IS_CORRECT(module_num))
            {
                ind = SMB_MASTER_PEC_ERR_IND;
            }
        }

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
        SMB_status[module_num].operation_state = SMB_STOP_PENDING;
        SMB_status[module_num].stop_indication = ind;
#else
        /*-------------------------------------------------------------------------------------------------*/
        /* Reset state for new transaction                                                                 */
        /*-------------------------------------------------------------------------------------------------*/
        SMB_status[module_num].operation_state = SMB_IDLE;

        /*-------------------------------------------------------------------------------------------------*/
        /* Notify upper layer of transaction completion                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        EXECUTE_FUNC(SMB_callback, (module_num, ind, SMB_status[module_num].read_index));
#endif
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        SMB_HandleMasterWrite                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - SMB module number.                                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles Master Write operation.                                           */
/*---------------------------------------------------------------------------------------------------------*/
static void SMB_HandleMasterWrite (SMB_MODULE_T module_num)
{
    UINT16 wcount;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Optimization to avoid extra runtime calculations in case only one module is supported               */
    /*-----------------------------------------------------------------------------------------------------*/
#if (SMB_NUM_OF_MODULES == 1)
    module_num = 0;
#endif

    if (SMB_status[module_num].fifo_use == TRUE)
    {
        SET_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_THST, 1);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Master write operation - perform write of required number of bytes                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    if (SMB_status[module_num].write_index == SMB_status[module_num].write_size)
    {
        if ((SMB_status[module_num].fifo_use == TRUE) && (READ_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_BYTES) > 0))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* No more bytes to send (to add to the FIFO), however the FIFO is not empty yet and is still  */
            /* in the middle of transmitting. Currently there is nothing to do except for waiting to the   */
            /* end of the transmission. We will get an interrupt when the FIFO will get empty.             */
            /*---------------------------------------------------------------------------------------------*/
            return;
        }

        if (SMB_status[module_num].read_size == 0)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* All bytes have been written, in a pure write operation                                      */
            /*---------------------------------------------------------------------------------------------*/

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
            /*---------------------------------------------------------------------------------------------*/
            /* Enable "End of Busy" interrupt.                                                             */
            /*---------------------------------------------------------------------------------------------*/
            SET_REG_FIELD(SMBCTL1(module_num), SMBCTL1_EOBINTE, 1);
#endif
            /*---------------------------------------------------------------------------------------------*/
            /* Issue a STOP condition on the bus                                                           */
            /*---------------------------------------------------------------------------------------------*/
            SMB_Stop(module_num);

            /*---------------------------------------------------------------------------------------------*/
            /* Clear SDA Status bit (by writing dummy byte)                                                */
            /*---------------------------------------------------------------------------------------------*/
            SMB_WriteByte(module_num, 0xFF);

#ifdef SMB_CAPABILITY_END_OF_BUSY_SUPPORT
            SMB_status[module_num].operation_state = SMB_STOP_PENDING;
            SMB_status[module_num].stop_indication = SMB_MASTER_DONE_IND;
#else
            /*---------------------------------------------------------------------------------------------*/
            /* Reset state for new transaction                                                             */
            /*---------------------------------------------------------------------------------------------*/
            SMB_status[module_num].operation_state = SMB_IDLE;

            /*---------------------------------------------------------------------------------------------*/
            /* Notify upper layer of transaction completion                                                */
            /*---------------------------------------------------------------------------------------------*/
            EXECUTE_FUNC(SMB_callback, (module_num, SMB_MASTER_DONE_IND, 0));
#endif
        }
        else
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Last write-byte written on previous interrupt - need to restart & send slave address        */
            /*---------------------------------------------------------------------------------------------*/
            if ((SMB_status[module_num].PEC_use == TRUE) &&
                (SMB_status[module_num].read_size < SMB_BYTES_EXCLUDE_BLOCK_SIZE_FROM_BUFFER))
            {
                /*-----------------------------------------------------------------------------------------*/
                /* PEC is used but the protocol isn't block read protocol - add extra byte for PEC support */
                /*-----------------------------------------------------------------------------------------*/
                SMB_status[module_num].read_size += 1;
            }

            if (SMB_status[module_num].fifo_use == TRUE)
            {
                if ((SMB_status[module_num].read_size == 1 ||
                     SMB_status[module_num].read_size == SMB_BYTES_EXCLUDE_BLOCK_SIZE_FROM_BUFFER ||
                     SMB_status[module_num].read_size == SMB_BYTES_BLOCK_PROT))
                {
                    /*-------------------------------------------------------------------------------------*/
                    /* SMBus Block read transaction.                                                       */
                    /*-------------------------------------------------------------------------------------*/
                    REG_WRITE(SMBTXF_CTL(module_num), 0);
                    REG_WRITE(SMBRXF_CTL(module_num), 1);
                }
                else
                {
                    if (SMB_status[module_num].read_size > SMBUS_FIFO_SIZE)
                    {
                        SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR, SMBUS_FIFO_SIZE);
                    }
                    else
                    {
                        /*---------------------------------------------------------------------------------*/
                        /* Clear the status bits                                                           */
                        /*---------------------------------------------------------------------------------*/
                        SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_RX_THR, (UINT8)SMB_status[module_num].read_size);
                        SET_REG_FIELD(SMBRXF_CTL(module_num), SMBRXF_CTL_LAST_PEC, 1);
                    }
                }
            }

            /*---------------------------------------------------------------------------------------------*/
            /* Generate (Repeated) Start upon next write to SDA                                            */
            /*---------------------------------------------------------------------------------------------*/
            SMB_Start(module_num);

            if (SMB_status[module_num].read_size == 1)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Receiving one byte only - stall after successful completion of sending address byte.    */
                /* If we NACK here, and slave doesn't ACK the address, we  might unintentionally NACK the  */
                /* next multi-byte read                                                                    */
                /*-----------------------------------------------------------------------------------------*/
                SMB_StallAfterStart(module_num, TRUE);
            }

#ifdef SMB_HOOK_ADD_HOLD_TIME_BEFORE_REPEATED_START
            SMB_HOOK_ADD_HOLD_TIME_BEFORE_REPEATED_START(module_num, SMB_status[module_num].dest_addr);
#endif

            /*---------------------------------------------------------------------------------------------*/
            /* Send the slave address in read direction                                                    */
            /*---------------------------------------------------------------------------------------------*/
            SMB_WriteByte(module_num, SMB_status[module_num].dest_addr | 0x1);

            /*---------------------------------------------------------------------------------------------*/
            /* Next interrupt will occur on read                                                           */
            /*---------------------------------------------------------------------------------------------*/
            SMB_status[module_num].operation = SMB_READ_OPER;
        }
    }
    else
    {
        if ((SMB_status[module_num].PEC_use == TRUE) && (SMB_status[module_num].write_index == 0) &&
            (SMB_status[module_num].read_size == 0))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Extra bytes for PEC support                                                                 */
            /*---------------------------------------------------------------------------------------------*/
            SMB_status[module_num].write_size += 1;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Write next byte not last byte and not slave address                                             */
        /*-------------------------------------------------------------------------------------------------*/
        if ((SMB_status[module_num].fifo_use == FALSE) || (SMB_status[module_num].write_size == 1))
        {
            if ((SMB_status[module_num].PEC_use == TRUE) && (SMB_status[module_num].read_size == 0) &&
                (SMB_status[module_num].write_index+1 == SMB_status[module_num].write_size))
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Master write protocol to send PEC byte.                                                 */
                /*-----------------------------------------------------------------------------------------*/
                SMBUS_SEND_PEC_BYTE(module_num);

                SMB_status[module_num].write_index++;
            }
            else
            {
                SMB_WriteByte(module_num, SMB_status[module_num].write_data_buf[SMB_status[module_num].write_index++]);
            }
        }
        /*-------------------------------------------------------------------------------------------------*/
        /* FIFO is used                                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        else
        {
            wcount = SMB_status[module_num].write_size - SMB_status[module_num].write_index;

#ifdef SMB_CAPABILITY_HW_PEC_SUPPORT
            /*---------------------------------------------------------------------------------------------*/
            /* Put the PEC byte last after the FIFO becomes empty.                                         */
            /*---------------------------------------------------------------------------------------------*/
            if (SMB_status[module_num].PEC_use)
            {
                if (wcount > 1)
                {
                    /*-------------------------------------------------------------------------------------*/
                    /* More bytes to send, do not send the PEC byte                                        */
                    /*-------------------------------------------------------------------------------------*/
                    wcount--;
                }
                else if (READ_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_BYTES) > 0)
                {
                    /*-------------------------------------------------------------------------------------*/
                    /* No more bytes to send except for the PEC byte. However the FIFO is not empty yet    */
                    /* and is still in the middle of transmitting. Currently there is nothing to do except */
                    /* for waiting to the end of the transmission. We will get an interrupt when the FIFO  */
                    /* will get empty and only then send the PEC byte.                                     */
                    /*-------------------------------------------------------------------------------------*/
                    return;
                }
            }
#endif

            if (wcount > SMBUS_FIFO_SIZE)
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Data to send is more then FIFO size.                                                    */
                /* Configure the FIFO interrupt to be mid of FIFO.                                         */
                /*-----------------------------------------------------------------------------------------*/
                REG_WRITE(SMBTXF_CTL(module_num), BUILD_FIELD_VAL(SMBTXF_CTL_THR_TXIE,1) | (SMBUS_FIFO_SIZE/2));
            }
            else if ((wcount > SMBUS_FIFO_SIZE/2) && (SMB_status[module_num].write_index != 0))
            {
                /*-----------------------------------------------------------------------------------------*/
                /* write_index != 0 means that this is not the first write.                                */
                /* Since interrupt is in the mid of FIFO, only half of the FIFO is empty.                  */
                /* Continue to configure the FIFO interrupt to be mid of FIFO.                             */
                /*-----------------------------------------------------------------------------------------*/
                REG_WRITE(SMBTXF_CTL(module_num), BUILD_FIELD_VAL(SMBTXF_CTL_THR_TXIE,1) | (SMBUS_FIFO_SIZE/2));
            }
            else
            {
                /*-----------------------------------------------------------------------------------------*/
                /* This is the first write (write_index = 0) and data to send is less or equal to FIFO size*/
                /* or this is the last write and data to send is less or equal half FIFO size.             */
                /* In both cases disable the FIFO threshold interrupt.                                     */
                /* The next interrupt will happen after the FIFO will get empty.                           */
                /*-----------------------------------------------------------------------------------------*/
                REG_WRITE(SMBTXF_CTL(module_num), 0);
            }

            SMB_WriteToFifo(module_num, wcount);

            /*---------------------------------------------------------------------------------------------*/
            /* Clear status bit                                                                            */
            /*---------------------------------------------------------------------------------------------*/
            SET_REG_FIELD(SMBTXF_STS(module_num), SMBTXF_STS_TX_THST, 1);
#ifdef SMB_STALL_TIMEOUT_SUPPORT
            SMB_RESET_STALL_COUNTER(module_num);
#endif
        }
    }
}
#endif  /* SMB_MASTER_SUPPORT */

