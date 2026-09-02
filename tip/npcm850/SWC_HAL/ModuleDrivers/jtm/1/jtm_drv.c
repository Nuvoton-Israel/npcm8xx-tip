/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2022 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   jtm_drv.c                                                                                             */
/*            This file contains JTM module driver implementation                                          */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#include __CHIP_H_FROM_DRV()



#include "jtm_regs.h"
#include "jtm_drv.h"



/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               DEFINITIONS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 MACROS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            GLOBAL VARIABLES                                             */
/*---------------------------------------------------------------------------------------------------------*/

static JTM_CALLBACK_T JTM_callback[JTM_NUM_OF_MODULES];
static JTM_INFO_T	JTM_info[JTM_NUM_OF_MODULES];

/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                       LOCAL FUNCTIONS DECLARATION                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/



static DEFS_STATUS JTM_PerformTransfer(JTM_MODULE_T   module,     JTM_XFER_T *xfer);
static DEFS_STATUS JTM_PerformSetTapstate(JTM_MODULE_T   module, JTAG_STATE_T from, JTAG_STATE_T  to);
static DEFS_STATUS JTM_Shift(JTM_MODULE_T module, UINT8 *jtm_tdo,	UINT8 *jtm_tdi, UINT8 *tms, UINT32 tcks, BOOLEAN blocking);
static UINT32 JTM_Send(JTM_MODULE_T module);
static UINT32 DIV_ROUND_CLOSEST(UINT32 x, UINT32 divisor);
static UINT32 DIV_ROUND_UP(UINT32 x, UINT32 divisor);


DEFS_STATUS JTM_BitBangs(JTM_MODULE_T module, JTM_BITBANG_PACKET_T *bitbangs);
DEFS_STATUS JTM_ClearEventsStatus (JTM_MODULE_T module);
void    JTM_IntHandler  (UINT16 int_num);


const  TMSCYCLE_T tmscyclelookup[][16] = {
/*      TLR        RTI       SelDR      CapDR      SDR      */
/*      Ex1DR      PDR       Ex2DR      UpdDR      SelIR    */
/*      CapIR      SIR       Ex1IR      PIR        Ex2IR    */
/*      UpdIR                                               */
/* TLR */
	{
		{0x01, 1}, {0x00, 1}, {0x02, 2}, {0x02, 3}, {0x02, 4},
		{0x0a, 4}, {0x0a, 5}, {0x2a, 6}, {0x1a, 5}, {0x06, 3},
		{0x06, 4}, {0x06, 5}, {0x16, 5}, {0x16, 6}, {0x56, 7},
		{0x36, 6}
	},
/* RTI */
	{
		{0x07, 3}, {0x00, 1}, {0x01, 1}, {0x01, 2}, {0x01, 3},
		{0x05, 3}, {0x05, 4}, {0x15, 5}, {0x0d, 4}, {0x03, 2},
		{0x03, 3}, {0x03, 4}, {0x0b, 4}, {0x0b, 5}, {0x2b, 6},
		{0x1b, 5}
	},
/* SelDR */
	{
		{0x03, 2}, {0x03, 3}, {0x00, 0}, {0x00, 1}, {0x00, 2},
		{0x02, 2}, {0x02, 3}, {0x0a, 4}, {0x06, 3}, {0x01, 1},
		{0x01, 2}, {0x01, 3}, {0x05, 3}, {0x05, 4}, {0x15, 5},
		{0x0d, 4}
	},
/* CapDR */
	{
		{0x1f, 5}, {0x03, 3}, {0x07, 3}, {0x00, 0}, {0x00, 1},
		{0x01, 1}, {0x01, 2}, {0x05, 3}, {0x03, 2}, {0x0f, 4},
		{0x0f, 5}, {0x0f, 6}, {0x2f, 6}, {0x2f, 7}, {0xaf, 8},
		{0x6f, 7}
	},
/* SDR */
	{
		{0x1f, 5}, {0x03, 3}, {0x07, 3}, {0x07, 4}, {0x00, 0},
		{0x01, 1}, {0x01, 2}, {0x05, 3}, {0x03, 2}, {0x0f, 4},
		{0x0f, 5}, {0x0f, 6}, {0x2f, 6}, {0x2f, 7}, {0xaf, 8},
		{0x6f, 7}
	},
/* Ex1DR */
	{
		{0x0f, 4}, {0x01, 2}, {0x03, 2}, {0x03, 3}, {0x02, 3},
		{0x00, 0}, {0x00, 1}, {0x02, 2}, {0x01, 1}, {0x07, 3},
		{0x07, 4}, {0x07, 5}, {0x17, 5}, {0x17, 6}, {0x57, 7},
		{0x37, 6}
	},
/* PDR */
	{
		{0x1f, 5}, {0x03, 3}, {0x07, 3}, {0x07, 4}, {0x01, 2},
		{0x05, 3}, {0x00, 1}, {0x01, 1}, {0x03, 2}, {0x0f, 4},
		{0x0f, 5}, {0x0f, 6}, {0x2f, 6}, {0x2f, 7}, {0xaf, 8},
		{0x6f, 7}
	},
/* Ex2DR */
	{
		{0x0f, 4}, {0x01, 2}, {0x03, 2}, {0x03, 3}, {0x00, 1},
		{0x02, 2}, {0x02, 3}, {0x00, 0}, {0x01, 1}, {0x07, 3},
		{0x07, 4}, {0x07, 5}, {0x17, 5}, {0x17, 6}, {0x57, 7},
		{0x37, 6}
	},
/* UpdDR */
	{
		{0x07, 3}, {0x00, 1}, {0x01, 1}, {0x01, 2}, {0x01, 3},
		{0x05, 3}, {0x05, 4}, {0x15, 5}, {0x00, 0}, {0x03, 2},
		{0x03, 3}, {0x03, 4}, {0x0b, 4}, {0x0b, 5}, {0x2b, 6},
		{0x1b, 5}
	},
/* SelIR */
	{
		{0x01, 1}, {0x01, 2}, {0x05, 3}, {0x05, 4}, {0x05, 5},
		{0x15, 5}, {0x15, 6}, {0x55, 7}, {0x35, 6}, {0x00, 0},
		{0x00, 1}, {0x00, 2}, {0x02, 2}, {0x02, 3}, {0x0a, 4},
		{0x06, 3}
	},
/* CapIR */
	{
		{0x1f, 5}, {0x03, 3}, {0x07, 3}, {0x07, 4}, {0x07, 5},
		{0x17, 5}, {0x17, 6}, {0x57, 7}, {0x37, 6}, {0x0f, 4},
		{0x00, 0}, {0x00, 1}, {0x01, 1}, {0x01, 2}, {0x05, 3},
		{0x03, 2}
	},
/* SIR */
	{
		{0x1f, 5}, {0x03, 3}, {0x07, 3}, {0x07, 4}, {0x07, 5},
		{0x17, 5}, {0x17, 6}, {0x57, 7}, {0x37, 6}, {0x0f, 4},
		{0x0f, 5}, {0x00, 0}, {0x01, 1}, {0x01, 2}, {0x05, 3},
		{0x03, 2}
	},
/* Ex1IR */
	{
		{0x0f, 4}, {0x01, 2}, {0x03, 2}, {0x03, 3}, {0x03, 4},
		{0x0b, 4}, {0x0b, 5}, {0x2b, 6}, {0x1b, 5}, {0x07, 3},
		{0x07, 4}, {0x02, 3}, {0x00, 0}, {0x00, 1}, {0x02, 2},
		{0x01, 1}
	},
/* PIR */
	{
		{0x1f, 5}, {0x03, 3}, {0x07, 3}, {0x07, 4}, {0x07, 5},
		{0x17, 5}, {0x17, 6}, {0x57, 7}, {0x37, 6}, {0x0f, 4},
		{0x0f, 5}, {0x01, 2}, {0x05, 3}, {0x00, 1}, {0x01, 1},
		{0x03, 2}
	},
/* Ex2IR */
	{
		{0x0f, 4}, {0x01, 2}, {0x03, 2}, {0x03, 3}, {0x03, 4},
		{0x0b, 4}, {0x0b, 5}, {0x2b, 6}, {0x1b, 5}, {0x07, 3},
		{0x07, 4}, {0x00, 1}, {0x02, 2}, {0x02, 3}, {0x00, 0},
		{0x01, 1}
	},
/* UpdIR */
	{
		{0x07, 3}, {0x00, 1}, {0x01, 1}, {0x01, 2}, {0x01, 3},
		{0x05, 3}, {0x05, 4}, {0x15, 5}, {0x0d, 4}, {0x03, 2},
		{0x03, 3}, {0x03, 4}, {0x0b, 4}, {0x0b, 5}, {0x2b, 6},
		{0x00, 0}
	},
};
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
void JTM_Init (void)
{
    UINT i;

    for (i = 0; i < JTM_NUM_OF_MODULES; i++)
    {
        JTM_callback[i] = NULL;
    }
	
	// Initialize the data structure 
	 for (i = 0; i < JTM_NUM_OF_MODULES; i++)
    {
        memset(JTM_info,sizeof(JTM_INFO_T),0);
		JTM_info[i].xfer_done =  FALSE;
		JTM_info[i].freq =  NPCM_JTM_DEFAULT_RATE;
		JTM_info[i].tapstate = JTAG_STATE_CURRENT;
		
	}
	
	
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Config                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */

/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures an JTM module by selecting JTM inputs interface on             */
/*                  multiplexed pins.                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void JTM_Config (JTM_MODULE_T module)
{
    JTM_MUX(module);
	JTM_RESET(module,1);
	JTM_RESET(module,0);
	
}



/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Send                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */

/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine Sends a buffer of data to the slave device                                                                      */
/*------------------------------------------------------------------------------*/

static UINT32 JTM_Send(JTM_MODULE_T module)
{
	
	JTM_INFO_T *jtm_info = (JTM_INFO_T *)&JTM_info[module];
	UINT32 *tdo32 = (UINT32 *)jtm_info->tx_buf;
	UINT32 *tms32 = (UINT32 *)jtm_info->tms_buf;
	
	UINT8 *tdo8, *tms8;
	UINT32 cnt;
	UINT32 val, tmsval;
	int n, i;
	int words, bytes;
	


	if (jtm_info->tx_len > NPCM_JTM_FIFO_SIZE)
		cnt = NPCM_JTM_FIFO_SIZE;
	else
		cnt = jtm_info->tx_len;

	words = cnt / 32;
	bytes = DIV_ROUND_UP((cnt % 32), 8);

	for (n = 0; n < words; n++) {
		if (tdo32 != NULL) {
			REG_WRITE(JTM_TDO_OUT(module,n),*tdo32);
			tdo32++;
		} else
			REG_WRITE(JTM_TDO_OUT(module,n),0);
		if (jtm_info->tms_buf != NULL) {
			tmsval = *tms32;
			tms32++;
		} else
			tmsval = 0;
		if ((jtm_info->end_tms_high == TRUE) && (cnt == jtm_info->tx_len)
				&& (bytes == 0) && (n == (words - 1)))
				{
			tmsval |= (1 << 31);
				}
		REG_WRITE(JTM_TMS_OUT(module,n),tmsval);
	}

	if (bytes) {
		tdo8 = (UINT8 *)tdo32;
		tms8 = (UINT8 *)tms32;
		val = 0;
		tmsval = 0;
		for (i = 0; i < bytes; i++) {
			if (tdo8)
				val |= tdo8[i] << (i * 8);
			if (jtm_info->tms_buf)
				tmsval |= tms8[i] << (i * 8);
		}
		if ((jtm_info->end_tms_high == TRUE)  && (cnt == jtm_info->tx_len))
		{
			tmsval |= (1 << ((cnt % 32) - 1));
		}
		REG_WRITE( JTM_TDO_OUT(module,n),val);
		REG_WRITE( JTM_TMS_OUT(module,n),tmsval);
	}

	jtm_info->ck_cnt = cnt;
	jtm_info->tx_len -= cnt;
	if (jtm_info->tx_buf)
	{
		jtm_info->tx_buf += (cnt / 8);
	}
	if (jtm_info->tms_buf)
	{
		jtm_info->tms_buf += (cnt / 8);
	}

	/* Start */
	val = REG_READ( JTM_CMD(module));
	SET_VAR_FIELD(val,JTM_CMD_CK_CNT,cnt);
	SET_VAR_FIELD(val,JTM_CMD_ST_OP,1);
	REG_WRITE(JTM_CMD(module),val);

	return cnt;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Recv                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */
/*					cnt - expected number of bytes to use
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine Sends a buffer of data to the slave device                                                                      */
/*------------------------------------------------------------------------------*/

static DEFS_STATUS JTM_Recv(JTM_MODULE_T module,UINT32 cnt)
{
	JTM_INFO_T *jtm_info = (JTM_INFO_T *)&JTM_info[module];
	UINT32 *buf32 = (UINT32 *)jtm_info->rx_buf;
	UINT8 *buf;
	UINT32 val;
	int n, i;
	int words, bytes;

	if (jtm_info->rx_len < cnt)
		return DEFS_STATUS_INVALID_PARAMETER;

	words = cnt / 32;
	bytes = DIV_ROUND_UP((cnt % 32), 8);
	for (n = 0; n < words; n++) {
		val = REG_READ( JTM_TDI_IN(module,n));
		if (buf32) {
			*buf32 = val;
			buf32++;
		}
	}

	if (bytes) {
		buf = (UINT8 *)buf32;
		val = REG_READ( JTM_TDI_IN(module,n));
		if (buf)
			for (i = 0; i < bytes; i++)
				buf[i] = (val >> (i * 8)) & 0xFF;
	}
	jtm_info->rx_len -= cnt;
	if (jtm_info->rx_buf)
		jtm_info->rx_buf += cnt / 8;

	
	return DEFS_STATUS_OK;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_IntHandler                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_num - number of triggered interrupt                                                */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Interrupt handlers for JTM interrupts.                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void JTM_IntHandler (UINT16 int_num)
{

    JTM_MODULE_T module     = JTM_INTERRUPT_MODULE(int_num);
	ASSERT(module < JTM_NUM_OF_MODULES);
	
    UINT32 jtmStatus  	= 	REG_READ(JTM_STAT(module));
	
	if (READ_VAR_FIELD(jtmStatus,JTM_STAT_DONE) != 0)
	{
		JTM_ClearEventsStatus(module);
		if (JTM_info[module].rx_len && JTM_info[module].ck_cnt)
		{
			JTM_Recv(module,JTM_info[module].ck_cnt);
		}
		if (JTM_info[module].rx_len == 0)
		{
			//Complete transfer;
			JTM_info[module].xfer_done = TRUE;
			if (JTM_callback[module] != NULL)
				EXECUTE_FUNC(JTM_callback[module], (module, jtmStatus));
		}
	}
	if (READ_VAR_FIELD(jtmStatus,JTM_STAT_BUSY) == 0)
	{
		if (JTM_info[module].tx_len > 0)
		{
			JTM_Send(module);
		}
	}
		
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Shift                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */
/*					jtm_tdo: master to target
/*					jtm_tdi: target to master 
/*					tms,tck values for shift
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK when successful                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs a single shift to the slave device			*/
/*------------------------------------------------------------------------------*/

/* jtm_tdo: master to target, jtm_tdi: target to master */
static DEFS_STATUS JTM_Shift(JTM_MODULE_T module, UINT8 *jtm_tdo,	UINT8 *jtm_tdi, UINT8 *tms, UINT32 tcks, BOOLEAN blocking)			
{
	UINT32 val;
	UINT8 stat;
	unsigned long flags;
	DEFS_STATUS ret = DEFS_STATUS_OK;
	JTM_INFO_T *jtm_info = (JTM_INFO_T *)&JTM_info[module];

	if (!tcks)
		return DEFS_STATUS_INVALID_PARAMETER;

	jtm_info->tx_len = tcks;
	jtm_info->tx_buf = jtm_tdo;
	jtm_info->rx_len = tcks;
	jtm_info->rx_buf = jtm_tdi;
	jtm_info->tms_buf = tms;
	
	stat = REG_READ(JTM_STAT(module));
	
	if (READ_VAR_FIELD(stat,JTM_STAT_BUSY) != 0) {
		HAL_PRINT("jtm state busy 0x%X\n",stat);
		return DEFS_STATUS_FAIL;
	}

	// re-initialize transfer status 
	jtm_info->xfer_done = FALSE;
	/* enable module and interrupt */
	val = REG_READ(JTM_CTL(module));
	SET_VAR_FIELD(val,JTM_CTL_JTM_EN,1);
	SET_VAR_FIELD(val,JTM_CTL_DONE_IE,1);
	REG_WRITE(JTM_CTL(module),val);

	JTM_Send(module);

	
	if (blocking == FALSE)
	{
		jtm_info->complete_shift = TRUE;
		return ret;
	}
	
	jtm_info->complete_shift = FALSE;
	while(jtm_info->xfer_done != TRUE);
	 
	/* disable module and interrupt */
	SET_VAR_FIELD(val,JTM_CTL_JTM_EN,0);
	SET_VAR_FIELD(val,JTM_CTL_DONE_IE,0);
	REG_WRITE(JTM_CTL(module),val);

	return ret;
}
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_ShiftComplete                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */
/*					jtm_tdo: master to target															   */	
/*					jtm_tdi: target to master 															   */
/*					tms,tck values for shift															   */	
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK when successful                                                         */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine  disable module and interrupt after Shift in non-blocking mode            */
/*---------------------------------------------------------------------------------------------------------*/

/* jtm_tdo: master to target, jtm_tdi: target to master */
static DEFS_STATUS JTM_ShiftComplete(JTM_MODULE_T module)		
{	
	UINT32 val = REG_READ(JTM_CTL(module));
	SET_VAR_FIELD(val,JTM_CTL_JTM_EN,0);
	SET_VAR_FIELD(val,JTM_CTL_DONE_IE,0);
	REG_WRITE(JTM_CTL(module),val);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_HW_Reset                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */

/* Returns:         none						                                                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine resets the module 												           */
/*---------------------------------------------------------------------------------------------------------*/
static void JTM_HW_Reset(JTM_MODULE_T module)
{
	JTM_RESET(module,1);
	JTM_RESET(module,0);
}




/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_SetBaudrate                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */
/*					clk_khz - module clock frequency in khz
/*                  speed	- Baudrate to set       		                                                                                */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures an JTM module by selecting the baud rate			           */
/*---------------------------------------------------------------------------------------------------------*/


DEFS_STATUS JTM_SetBaudrate(JTM_MODULE_T module, UINT32 clk_khz, UINT32 speed)
{
	UINT32 ckdiv;
	UINT32 regtemp;


	if (speed > NPCM_JTM_MAX_RATE)
	{
		return DEFS_STATUS_INVALID_PARAMETER;
	}

	ckdiv = DIV_ROUND_CLOSEST(clk_khz, (2 * speed)) - 1;

	regtemp = REG_READ(JTM_CTL(module));
	SET_VAR_FIELD(regtemp,JTM_CTL_CKDV,ckdiv);
	REG_WRITE(JTM_CTL(module),regtemp);
	
	
	return DEFS_STATUS_OK;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_PerformSetTapstate                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */
/*					from - tap state currently in
/*                  to	- tap state to switch to      		                                                                                */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine is static, called by other functions to switch between two tap states			           */
/*---------------------------------------------------------------------------------------------------------*/


static DEFS_STATUS JTM_PerformSetTapstate(JTM_MODULE_T   module, JTAG_STATE_T from, JTAG_STATE_T to)
{
		
	UINT8 tdo[2], tdi[2], tms[2];
	UINT8 count;
	DEFS_STATUS ret = DEFS_STATUS_OK;
	ASSERT(from != to);
	if ((to == JTAG_STATE_CURRENT)  || (from == to))
	{
		return ret;
	}
	ASSERT(from <= JTAG_STATE_CURRENT);
	ASSERT(to <= JTAG_STATE_CURRENT);
	

	JTM_info[module].end_tms_high  = FALSE;
	if (to == JTAGTLR) {
		tms[0] = 0xff;
		tms[1] = 0x01;
		tdo[0] = tdo[1] = 0;
		ret = JTM_Shift(module, tdo, tdi, tms, JTAG_TLR_TMS_COUNT, TRUE);
		JTM_info[module].tapstate = JTAGTLR;
		return ret;
	}

	if (from == JTAG_STATE_CURRENT)
		from = JTM_info[module].tapstate;

	tms[0] = tmscyclelookup[from][to].tmsbits;
	count   = tmscyclelookup[from][to].count;

	if (count == 0)
		return DEFS_STATUS_OK;

	tdo[0] = 0;
	ret = JTM_Shift(module, tdo, tdi, tms, count, TRUE);
	HAL_PRINT("jtag: change state %d -> %d\n", from, to);
	JTM_info[module].tapstate = to;

	return ret;
}




/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_PerformTransfer                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module 			- JTM module number.                  		                           */
/*					xfer 		- object containing data to transfer												*/
/*                     		                                        */
/* Returns:         SEF_STATUS_OK if transaction ended well. TBD owtherwise                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sends data to JTAG slave 									           */
/*---------------------------------------------------------------------------------------------------------*/

static DEFS_STATUS JTM_PerformTransfer(JTM_MODULE_T   module,     JTM_XFER_T *xfer)
{
	
	
	DEFS_STATUS ret = DEFS_STATUS_OK;
	
	
	UINT32 bytes = DIV_ROUND_UP(xfer->length, BITS_PER_BYTE);
	
	if (xfer->type == JTAG_SIR_XFER)
	{
		JTM_PerformSetTapstate(module, xfer->from, JTAGSHFIR);
	}
	else if (xfer->type == JTAG_SDR_XFER)
	{
		JTM_PerformSetTapstate(module, xfer->from, JTAGSHFDR);
	}
	else if (xfer->type == JTAG_RUNTEST_XFER)
	{
		JTM_PerformSetTapstate(module, xfer->from, JTAGRTI);
	}

	/* SIR/SDR: the last bit should be shifted with TMS high */
	if ((xfer->type == JTAG_SIR_XFER && xfer->endstate != JTAGSHFIR) ||
			(xfer->type == JTAG_SDR_XFER && xfer->endstate != JTAGSHFDR)) {
		JTM_info[module].end_tms_high =  TRUE;
		JTM_info[module].tapstate = (JTM_info[module].tapstate == JTAGSHFDR) ?
				JTAGEX1DR : JTAGEX1IR;
	} else
		JTM_info[module].end_tms_high = FALSE;

	ret = JTM_Shift(module, xfer->tdo,xfer->tdi, NULL, xfer->length, xfer->blocking);
	
	if (xfer->blocking == FALSE)
	{
		JTM_info[module].complete_transfer = TRUE;
		return ret;
	}
	JTM_info[module].complete_transfer = FALSE;
	
	// In blocking mode, we know that the transaction completed so we can coomplete the step
	JTM_PerformSetTapstate(module, JTAG_STATE_CURRENT, xfer->endstate);
	// No need to copy, because tdi/tdo are valid (not using 1 data array for both
	/*if (xfer->type != JTAG_RUNTEST_XFER) {
		if (jtm_tdo  && (ret == DEFS_STATUS_OK))
		{
			memcpy(jtm_tdo, JTM_TDI, bytes);
		}
	}
   */
	return ret;
}

static DEFS_STATUS JTM_PerformTransferComplete(JTM_MODULE_T   module,     JTM_XFER_T *xfer)
{
	JTM_PerformSetTapstate(module, JTAG_STATE_CURRENT, xfer->endstate);
}


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                       INTERFACE FUNCTIONS DECLARATION                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

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

// Note:RUNTEST doesnt return any data to caller! 
DEFS_STATUS JTM_Runtest(JTM_MODULE_T   module, UINT32 tcks)
{
	JTM_XFER_T xfer;
	UINT32 bytes = DIV_ROUND_UP(tcks, BITS_PER_BYTE);
	DEFS_STATUS ret = DEFS_STATUS_OK;

	xfer.type = JTAG_RUNTEST_XFER;
	xfer.direction = JTAG_WRITE_XFER;
	xfer.from = JTAG_STATE_CURRENT;
	xfer.endstate = JTAGRTI;
	xfer.length = tcks;
	xfer.tdo = NULL;				// Data to send
	xfer.tdi = NULL;				// Data to recv	
	xfer.blocking = TRUE;
	ret = JTM_PerformTransfer(module, &xfer);
	

	return ret;
}
	


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_BitBangs                                                                             */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module 			- JTM module number.                  		                           */
/*					bitbangs 		- format of data to send												*/
/* Returns:         SEF_STATUS_OK if transaction ended well. Data is returned in  bitbangs                 */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sens data in bit-banging mode 									           */
/*---------------------------------------------------------------------------------------------------------*/

	
DEFS_STATUS JTM_BitBangs(JTM_MODULE_T module, JTM_BITBANG_PACKET_T *bitbangs)
{
	UINT32 data_size = 0;
	DEFS_STATUS ret = DEFS_STATUS_OK;
	int i;
	UINT8 *jtm_tdo, *jtm_tdi, *tms;
	
	JTM_TCK_BITBANG_T  *bitbang_data = bitbangs->data;
	
	if (bitbangs->length >= JTAG_MAX_XFER_DATA_LEN)
	{
		HAL_PRINT("length exceeds JTAG_MAX_XFER_DATA_LEN\n");
		return DEFS_STATUS_INVALID_PARAMETER;
	}

	JTM_info[module].end_tms_high = FALSE;
	for (i = 0; i < bitbangs->length; i++) {
		jtm_tdo = &(bitbang_data[i].tdi);
		jtm_tdi = &(bitbang_data[i].tdo);
		tms = &(bitbang_data[i].tms);
		ret = JTM_Shift(module, jtm_tdo, jtm_tdi, tms, 1, TRUE);
		if (ret != DEFS_STATUS_OK)
			break;
	}

	return ret;
}


	
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_Transfer                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module 			- JTM module number.                  		                           */
/*					xfer 		- data object to transefr												   */
/* Returns:         DEF_STATUS_OK if transaction ended well. TBD owtherwise   , Data is returned in xfer   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sens data to JTAG slave 									               */
/*---------------------------------------------------------------------------------------------------------*/

	
DEFS_STATUS JTM_Transfer(JTM_MODULE_T module, JTM_XFER_T *xfer)
{
	//UINT32 data_size = 0;
		if (xfer->length >= JTAG_MAX_XFER_DATA_LEN)
			return DEFS_STATUS_INVALID_PARAMETER;

		if (xfer->type > JTAG_SDR_XFER)
			return DEFS_STATUS_INVALID_PARAMETER;

		if (xfer->direction > JTAG_READ_WRITE_XFER)
			return DEFS_STATUS_INVALID_PARAMETER;

		if (xfer->from > JTAG_STATE_CURRENT)
			return DEFS_STATUS_INVALID_PARAMETER;

		if (xfer->endstate > JTAG_STATE_CURRENT)
			return DEFS_STATUS_INVALID_PARAMETER;

		
		// No need to provide tdio as seperate array because in our implementation, it is already embedded in xfer
		return  JTM_PerformTransfer(module, xfer);
		
}	


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
DEFS_STATUS JTM_SetTAPState(JTM_MODULE_T   module,    JTM_TAP_STATE *tapstate)
{
	UINT8 tdo[2], tdi[2], tms[2];
	UINT8 count;
	DEFS_STATUS ret = DEFS_STATUS_OK;
	
	if (tapstate->from > JTAG_STATE_CURRENT)
		return DEFS_STATUS_INVALID_PARAMETER;

	if (tapstate->endstate > JTAG_STATE_CURRENT)
		return DEFS_STATUS_INVALID_PARAMETER;

	if (tapstate->reset > JTM_FORCE_RESET)
		return DEFS_STATUS_INVALID_PARAMETER;
		
		
	if (tapstate->reset == JTM_FORCE_RESET)
	{
			ret = JTM_PerformSetTapstate(module, JTAG_STATE_CURRENT,	  JTAGTLR) ;
			if (ret != DEFS_STATUS_OK)
			{
				return ret;
			}
	}
	return JTM_PerformSetTapstate(module, tapstate->from,  tapstate->endstate);
	
}


	
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

	
DEFS_STATUS	JTM_CompleteTask(JTM_MODULE_T module, JTM_XFER_T *xfer)
{
	DEFS_STATUS ret = DEFS_STATUS_OK;
	if (JTM_info[module].complete_shift  == TRUE)
	{
		JTM_ShiftComplete(module);
	}
	JTM_info[module].complete_shift = FALSE;
	
	if (JTM_info[module].complete_transfer  == TRUE)
	{
		JTM_PerformTransferComplete(module,xfer);
	}
	JTM_info[module].complete_transfer = FALSE;
	
	return ret;
	
}
	
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_InitModule                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module               - JTM module number.                                              */
/*					baudrate 			 - Baud rate 
/*                  callback             - function to be called on an JTM interrupt                       */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function handles the JTM configuration.                                           */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS JTM_InitModule (
    JTM_MODULE_T    module,
	UINT32 			ClkKHz,
	UINT32 			baudrate,
    JTM_CALLBACK_T  callback
)
{
    UINT32 configReg;
	int irq;
    ASSERT(module < JTM_NUM_OF_MODULES);


	DEFS_STATUS ret = DEFS_STATUS_OK;
	
	JTM_info[module].xfer_done =  FALSE;
	JTM_SetBaudrate(module,  ClkKHz	,  baudrate);
	JTM_callback[module] = callback;
	/*-----------------------------------------------------------------------------------------------------*/
    /* Interrupt configuration                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_REGISTER_AND_ENABLE(JTM_INTERRUPT_PROVIDER, JTM_INTERRUPT(module), JTM_IntHandler,
                                  JTM_INTERRUPT_POLARITY, JTM_INTERRUPT_PRIORITY);
								  
	JTM_PerformSetTapstate(module, JTAG_STATE_CURRENT, JTAGTLR);
    
    return DEFS_STATUS_OK;
}


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_GetEventsStatus                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module - JTM module number.                                                            */
/*                  status - a pointer to store the status information                                     */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function fills the given status pointer with the status information.              */
/*                  Note!!: Should only be used when JTM interrupts are disabled. When interrupts are      */
/*                          enabled the status is passed to the callback function and automatically        */
/*                          cleared by the handler.                                                        */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS JTM_GetEventsStatus (JTM_MODULE_T module, JTM_STATUS_T* status)
{
    UINT32 jtm_stat;

    ASSERT(module < JTM_NUM_OF_MODULES);

    jtm_stat                        = REG_READ(JTM_STAT(module));
    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_ClearEventsStatus                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  module    - JTM module number.                                                         */
/*                																	               */
/* Returns:         DEFS_STATUS_OK on success, and other DEFS_STATUS on failure.                           */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This function clears the if an event is pending.                    */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS JTM_ClearEventsStatus (JTM_MODULE_T module)
{
    ASSERT(module < JTM_NUM_OF_MODULES);

    SET_REG_FIELD(JTM_STAT(module), JTM_STAT_DONE,1);

    return DEFS_STATUS_OK;
}


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
DEFS_STATUS JTM_InterruptEnable (JTM_MODULE_T module)
{
	UINT32 tempReg = 0;
    ASSERT(module < JTM_NUM_OF_MODULES);

	tempReg =  REG_READ(JTM_CTL(module));
	SET_VAR_FIELD(tempReg,JTM_CTL_DONE_IE,1);
	REG_WRITE(JTM_CTL(module),tempReg);
    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        JTM_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void JTM_PrintRegs (void)
{
    UINT i;

    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     JTM      */\n");
    HAL_PRINT("/*--------------*/\n\n");

    for (i = 0; i < JTM_NUM_OF_MODULES; i++)
    {
        JTM_PrintModuleRegs((JTM_MODULE_T)i);
    }
}

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
/*lint -e{715}      Suppress 'module_num' not referenced                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void JTM_PrintModuleRegs (JTM_MODULE_T module_num)
{
    UINT8 i;

    ASSERT(module_num < JTM_NUM_OF_MODULES);

    HAL_PRINT("JTM%d:\n", (module_num+1));
    HAL_PRINT("--------\n");
    HAL_PRINT("JTM_CTL             = 0x%08X\n", REG_READ(JTM_CTL(module_num)));
    HAL_PRINT("JTM_STAT            = 0x%08X\n", REG_READ(JTM_STAT(module_num)));


    HAL_PRINT("\n");
}
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
DEFS_STATUS JTM_SelfTest (void)
{
   JTM_Init();
   JTM_Config(0);
   JTM_InitModule (0,125000,30000,NULL);
   JTM_Runtest(0,0x8);

    return DEFS_STATUS_OK;
}
//#endif //JTM_SELF_TEST


UINT32 DIV_ROUND_CLOSEST(UINT32 x, UINT32 divisor)
{
	UINT32 val = x / divisor;
	UINT32 modulu = x % divisor;
	if (modulu >= (divisor >> 1))
	{
		
		return val + 1;
	}
	return  val;
}

UINT32 DIV_ROUND_UP(UINT32 x, UINT32 divisor)
{
	UINT32 val = x / divisor;
	UINT32 modulu = x % divisor;
	
	if (modulu == 0)
		return val;
	return (val+1);
}

