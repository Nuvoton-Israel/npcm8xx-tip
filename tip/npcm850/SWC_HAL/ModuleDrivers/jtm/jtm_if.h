/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2022 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   jtm_if.h                                                                                              */
/*            This file contains JTM module interface                                                      */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
 
#ifndef JTM_IF_H
#define JTM_IF_H
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()
#if defined JTM_MODULE_TYPE
#include __MODULE_HEADER(jtm, JTM_MODULE_TYPE)
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* JTM mode                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/

#define BITS_PER_BYTE  8

typedef struct TCK_BITBANG {
	UINT8     tms;
	UINT8     tdi;
	UINT8     tdo;
}  JTM_TCK_BITBANG_T;



/*---------------------------------------------------------------------------------------------------------*/
/* JTM mode                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
	JTAGTLR,
	JTAGRTI,
	JTAGSELDR,
	JTAGCAPDR,
	JTAGSHFDR,
	JTAGEX1DR,
	JTAGPAUDR,
	JTAGEX2DR,
	JTAGUPDDR,
	JTAGSELIR,
	JTAGCAPIR,
	JTAGSHFIR,
	JTAGEX1IR,
	JTAGPAUIR,
	JTAGEX2IR,
	JTAGUPDIR,
	JTAG_STATE_CURRENT
} JTAG_STATE_T;

typedef struct  JTAG_XFER {
	UINT8	type;
	UINT8	direction;
	UINT8	from;
	UINT8	endstate;
	UINT32	padding;
	UINT32	length;
	UINT8	*tdo;				// Data to send
	UINT8	*tdi;				// Data to recv
	BOOLEAN  blocking;			// Busy wait or return to sender
} JTM_XFER_T;




typedef struct  TAP_STATE {
	UINT8	reset;
	JTAG_STATE_T	from;
	JTAG_STATE_T	endstate;
	UINT8	tck;
}JTM_TAP_STATE;



typedef enum  {
	JTM_NO_RESET = 0,
	JTM_FORCE_RESET = 1,
}JTAG_RESET;

typedef enum  {
	JTAG_SIR_XFER = 0,
	JTAG_SDR_XFER = 1,
	JTAG_RUNTEST_XFER = 2,
}JTAG_XFER_T;

typedef enum  {
	JTAG_READ_XFER = 1,
	JTAG_WRITE_XFER = 2,
	JTAG_READ_WRITE_XFER = 3,
}JTAG_XFER_DIRECTION;



/*---------------------------------------------------------------------------------------------------------*/
/* JTM module number                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    JTM_MODULE_0  = 0,
	JTM_MODULE_1,
    JTM_MODULE_LAST
} JTM_MODULE_T;




typedef struct JTM_INFO {
	
	BOOLEAN xfer_done;
	UINT8 *tx_buf;
	UINT8 *rx_buf;
	UINT8 *tms_buf;
	UINT32 tx_len;
	UINT32 rx_len;
	UINT32 ck_cnt;
	JTAG_STATE_T tapstate;
	UINT32 freq;
	BOOLEAN end_tms_high;
	BOOLEAN complete_shift;
	BOOLEAN complete_transfer;
}JTM_INFO_T;






typedef struct TMSCYCLE {
	UINT8 tmsbits;
	UINT8 count;
}TMSCYCLE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* JLM status fields                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct  JTM_STATUS {
	
		
}JTM_STATUS_T; 




typedef struct  BITBANG_PACKET {
	JTM_TCK_BITBANG_T *data;
	UINT32	length;
} JTM_BITBANG_PACKET_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        Callback function                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module               - JTM module number.                                              */
/*                  jtmStatus            - The actual value that was read from the JTM Status register     */
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
/*                  Callback function for the JTM module. Called from the JTM interrupt.                   */
/*---------------------------------------------------------------------------------------------------------*/
typedef void (*JTM_CALLBACK_T)(JTM_MODULE_T module, UINT32 jtmStatus);


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/* this is the complete set TMS cycles for going from any TAP state to
 * any other TAP state, following a “shortest path” rule
 */

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        Callback function                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module               - JTM module number.                                              */
/*                  jtmStatus            - The actual value that was read from the JTM Status register     */
/*                                         TBD															 */
/*                                                                                                         */
/* Description:                                                                                            */
/*                  Callback function for the JTM module. Called from the JTM interrupt.                   */
/*---------------------------------------------------------------------------------------------------------*/
typedef void (*JTM_CALLBACK_T)(JTM_MODULE_T module, UINT32 jtmStatus);


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Init                                                                               */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function initiates JTM driver global variables.                                   */
/*---------------------------------------------------------------------------------------------------------*/
void JTM_Init (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Config                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */
/*                                                                                                         */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures an JTM module by selecting JTM inputs interface on             */
/*                  multiplexed pins.                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void JTM_Config (JTM_MODULE_T module);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_InitModule                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module               - JTM module number.                                              */
/*					baudrate 			 - Baud rate 													   */
/*					ClkHz 			 	 - Referance clock 												   */
/*                  callback             - function to be called on an JTM interrupt                       */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function handles the JTM configuration.                                           */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS JTM_InitModule (
    JTM_MODULE_T    module,
	UINT32 ClkHz,
	UINT32 baudrate,
    JTM_CALLBACK_T  callback
);



/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_InterruptEnable                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module  - JTM module number.                                                           */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function enables/disables the interrupts (and wakeup) according to the given mask */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS JTM_InterruptEnable (JTM_MODULE_T module);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Enable                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number                                                             */
/*                  enable - If TRUE enables the module, if FALSE disables it                              */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function enables/disables the module                                              */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS JTM_Enable (JTM_MODULE_T module, BOOLEAN enable);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Enabled                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number                                                             */
/*                                                                                                         */
/* Returns:         TRUE in case JTM module is enabled; FALSE otherwise                                    */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the JTM module enable state                                       */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN JTM_Enabled (JTM_MODULE_T module);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_SetBaudrate                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module  - JTM module number                                                            */
/*					clk_hz  - External clock frequency                                                     */   
/*					speed   - JTAG frequency                                            						 */ 
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK when successful                                    */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets the JTAG expected frequency                                       */
/*-------------------------------------------------------------------------------------*/

DEFS_STATUS JTM_SetBaudrate(JTM_MODULE_T module, UINT32 clk_hz, UINT32 speed);

//#ifdef JTM_SELF_TEST
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_SelfTest                                                                           */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine test the JTM driver                                                       */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS JTM_SelfTest (void);


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Runtest                                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module 			- JTM module number.                  		                           */
/*					tcks 		- number of bytes to send												   */
/* Returns:         SEF_STATUS_OK if transaction ended well. TBD owtherwise                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine Run in rti state for specific number of tcks							   */
/*---------------------------------------------------------------------------------------------------------*/

DEFS_STATUS JTM_Runtest (JTM_MODULE_T   module, UINT32 tcks);

	
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Transfer                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module 			- JTM module number.                  		                           */
/*					xfer 		- data object to transefr												   */
/* Returns:         DEF_STATUS_OK if transaction ended well. TBD owtherwise   , Data is returned in xfer   */ 
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sends data to JTAG slave 									               */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS JTM_Transfer(JTM_MODULE_T module, JTM_XFER_T *xfer);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_SetTAPState                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */
/*					from  - state to transit from 														   */
/*                  to	  - statte to transit to       		                                               */
/* Returns:         DEFS_STATUS_OK or    DEFS_STATUS_INVALID_PARAMETER, No data                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine changes TAP states												           */
/*---------------------------------------------------------------------------------------------------------*/

DEFS_STATUS JTM_SetTAPState(JTM_MODULE_T   module,    JTM_TAP_STATE *tapstate);


	
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_CompleteTask                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */
/*					xfer -  the object containing the data to transfer									   */
/*					 		                                                                               */
/* Returns:         DEFS_STATUS_OK if succesful                                                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine completes the tasks pending in non-blocking mode				           */
/*---------------------------------------------------------------------------------------------------------*/

	
DEFS_STATUS	JTM_CompleteTask(JTM_MODULE_T module, JTM_XFER_T *xfer);
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void JTM_PrintRegs (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_PrintModuleRegs                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module_num - The module to be printed.                                                 */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module instance registers                                      */
/*---------------------------------------------------------------------------------------------------------*/
void JTM_PrintModuleRegs (JTM_MODULE_T module_num);


#endif /* _JTM_IF_H */

