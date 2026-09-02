/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2010-2018 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   twd_drv.c                                                                                             */
/*            This file contains Timer Watch-Dog (TWD) driver implementation                               */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_DRV()

#include "twd_drv.h"
#include "twd_regs.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Module Dependencies                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#if defined PMC_MODULE_TYPE
#include __MODULE_IF_HEADER_FROM_DRV(pmc)
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Clocks definitions                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define CLOCK_FREQ              TWD_SOURCE_CLOCK    // this should be defined in the specific chip.h
#define TWCP_MDIV_MAX           10
#define MAX_CLOCK_FREQ          CLOCK_FREQ
#define MIN_CLOCK_FREQ          (CLOCK_FREQ >> TWCP_MDIV_MAX)

/*---------------------------------------------------------------------------------------------------------*/
/* Data match value                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define WD_DATA_MATCH_VAL       0x5C

/*---------------------------------------------------------------------------------------------------------*/
/* TWD Reset timeout                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define TWD_RST_TIMEOUT         1000

/*---------------------------------------------------------------------------------------------------------*/
/* TWD get timer timeout                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define TWD_GET_TIMER_TIMEOUT   100

/*---------------------------------------------------------------------------------------------------------*/
/* TWD min/max preset value                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define TWD_MIN_PRESET_VAL      0x1UL
#define TWD_MAX_PRESET_VAL      0x10000UL


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            GLOBAL VARIABLES                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* User callback to be called from timer interrupt                                                         */
/*---------------------------------------------------------------------------------------------------------*/
static TWD_CALLBACK TWD_timer_callback;

/*---------------------------------------------------------------------------------------------------------*/
/* Boolean for data match mechanism for WD                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
static BOOLEAN      TWD_wd_data_match;

/*---------------------------------------------------------------------------------------------------------*/
/* Global to hold Watch-Dog counter value                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
static UINT8        TWD_wd_counter;

/*---------------------------------------------------------------------------------------------------------*/
/* TWD Prerequisite for before entering Idle or Deep Idle mode                                             */
/*---------------------------------------------------------------------------------------------------------*/
#if defined PMC_MODULE_TYPE
static PMC_CALLBACK_T TWD_IdleModePrerequisite;
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                  LOCAL FUNCTIONS FORWARD DECLARATIONS                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#if defined PMC_MODULE_TYPE
static DEFS_STATUS  TWD_IdleModeCallback    (void);
#endif
static DEFS_STATUS  TWD_Reload              (void);


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_Init                                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  timer_handler - Application callback to be called from timer interrupt handler.        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine initializes the TWD module.                                               */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_Init (TWD_CALLBACK timer_handler)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Initialize global variables                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    TWD_wd_data_match = FALSE;
    TWD_wd_counter    = 0x0F;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Keep global callback to the application callback to be called from timer interrupt                  */
    /*-----------------------------------------------------------------------------------------------------*/
    TWD_timer_callback = timer_handler;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Register TWD Prerequisite for entering Idle mode                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
#if defined PMC_MODULE_TYPE
    TWD_IdleModePrerequisite.callback = TWD_IdleModeCallback;
    PMC_RegisterRequisite(&TWD_IdleModePrerequisite, PMC_PRE_REQUISITE);
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Mask/disable timer interrupt                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_ENABLE(TWD_INTERRUPT_PROVIDER, TWD_INTERRUPT, FALSE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Install timer interrupt handler in dispatch table                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_REGISTER_HANDLER(TWD_INTERRUPT_PROVIDER, TWD_INTERRUPT, TWD_TimerHandler);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set Timer and Watchdog polarity and priority                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_SET_POLARITY(TWD_INTERRUPT_PROVIDER, TWD_INTERRUPT, TWD_INTERRUPT_POLARITY);
    INTERRUPT_SET_PRIORITY(TWD_INTERRUPT_PROVIDER, TWD_INTERRUPT, TWD_INTERRUPT_PRIORITY);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear Timer and Watchdog interrupt                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_CLEAR(TWD_INTERRUPT_PROVIDER, TWD_INTERRUPT);

#ifdef TWD_CAPABILITY_TWD_INTERRUPT_EN
    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable Watchdog Interrupts                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(T0CSR, T0CSR_INTEN, 1);
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set Timer and Watchdog Configuration to default value                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(TWCFG, 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set Timer and Watchdog Clock Prescaler to default value                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(TWCP, TWCP_MDIV, 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set Timer 0 counter to default value                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(TWDT0, 0xFFFF);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_TimerConfig_us                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  time - desired system tick time in usec.                                               */
/*                                                                                                         */
/* Returns:         DEFS Error code.                                                                       */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets the timer to "time" microseconds.                                    */
/*                  It is assumed that the core is running in a frequency, sufficient to track RST         */
/*                  toggling before start counting (guaranteed by design).                                 */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TWD_TimerConfig_us (UINT16 time)
{
    UINT32  twtdo_preset, clk_freq, max_time_val, divisor, time_l;
    UINT8   twcp_mdiv;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Parameters check                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK(time > 0, DEFS_STATUS_INVALID_PARAMETER);

    time_l = time;
    clk_freq = MAX_CLOCK_FREQ;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Reduce time and devisor (1M) to avoid overflow in (clk_freq * time_l) calculation                   */
    /*-----------------------------------------------------------------------------------------------------*/
    max_time_val = 0xFFFFFFFF / clk_freq;
    divisor = _1SEC_IN_USEC_;
    while (time_l > max_time_val && divisor > 1) // while time is too big, and divisor/2 is not 0
    {
        time_l  /= 2;
        divisor /= 2;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Calcualte PRESET value                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    twtdo_preset =  ((UINT32)clk_freq * time_l) / divisor;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set pre-scale ratio                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    twcp_mdiv = 0;
    while (twcp_mdiv <= TWCP_MDIV_MAX)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* if twtdo_preset >= 0xFFFF twtdo_preset will fit TWDT0 16 bit register, so the values are ready  */
        /* to set. Otherwise, continue dividing it (and incrementing the pre-scaler accordingly)           */
        /*-------------------------------------------------------------------------------------------------*/
        if (twtdo_preset < 0x10000)
        {
            break;
        }

        twcp_mdiv++;
        twtdo_preset /= 2;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure the Timer Clock Prescaler                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(TWCP, TWCP_MDIV, twcp_mdiv);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure Timer 0 Register value                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(TWDT0, LSW(twtdo_preset-1));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Reload and restart TWD Timer 0                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_RET_CHECK(TWD_Reload());

    /*-----------------------------------------------------------------------------------------------------*/
    /* Unmask/enable timer interrupt                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_ENABLE(TWD_INTERRUPT_PROVIDER, TWD_INTERRUPT, TRUE);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_TimerConfig                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  time - desired system tick time in msec.                                               */
/*                                                                                                         */
/* Returns:         DEFS Error code.                                                                       */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine sets the timer to "time" milliseconds.                                    */
/*                  It is assumed that the core is running in a frequency, sufficient to track RST         */
/*                  toggling before start counting (guaranteed by design).                                 */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TWD_TimerConfig (UINT16 time)
{
    UINT8   twcp_mdiv;
    UINT32  twtdo_preset;
    UINT32  time_l = time;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Parameters check                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK(time > 0, DEFS_STATUS_INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check maximum time which can be configured                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK(time_l < ((UINT32)TWD_MAX_PRESET_VAL * _1SEC_IN_MSEC_ / MIN_CLOCK_FREQ),
                           DEFS_STATUS_PARAMETER_OUT_OF_RANGE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set pre-scale ratio                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    twcp_mdiv = TWCP_MDIV_MAX + 1;

    /*-----------------------------------------------------------------------------------------------------*/
    /* check the TWD clock is high enough. At least 2MHz to function properly                              */
    /*-----------------------------------------------------------------------------------------------------*/
    //lint -e{774} suppress PC-Lint warning on 'Boolean within 'if' always evaluates to False'
    DEFS_STATUS_COND_CHECK(((UINT32)MAX_CLOCK_FREQ > (UINT32)(1 << twcp_mdiv)), DEFS_STATUS_PARAMETER_OUT_OF_RANGE);

    while (twcp_mdiv)
    {
        twcp_mdiv--;
        twtdo_preset = (((UINT32)MAX_CLOCK_FREQ >> twcp_mdiv) * time_l) / _1SEC_IN_MSEC_;
        /*-------------------------------------------------------------------------------------------------*/
        /* if twtdo_preset >= 0x8000 we can't reduce twcp_mdiv any more since twtdo_preset will be more    */
        /* than 0x10000 and will not fit into the TWDT0 16 bit register.                                   */
        /*-------------------------------------------------------------------------------------------------*/
        if (twtdo_preset >= 0x8000)
        {
            break;
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure the Timer Clock Prescaler                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(TWCP, TWCP_MDIV, twcp_mdiv);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure Timer 0 Register value                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(TWDT0, LSW(twtdo_preset-1));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Reload and restart TWD Timer 0                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_RET_CHECK(TWD_Reload());

    /*-----------------------------------------------------------------------------------------------------*/
    /* Unmask/enable timer interrupt                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPT_ENABLE(TWD_INTERRUPT_PROVIDER, TWD_INTERRUPT, TRUE);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_TimerConfigLock                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  config_lock   - if TRUE locks (write protected) TWD module configuration (TWCFG).      */
/*                  prescale_lock - if TRUE locks (write protected) the pre-dcale ratio (TWCP.MDIV).       */
/*                  counter_lock  - if TRUE locks (write protected) TWDT0 counter and it's                 */
/*                                  control & status register (T0CSR).                                     */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine locks (write protected) configurations.                                   */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_TimerConfigLock (
    BOOLEAN config_lock,
    BOOLEAN prescale_lock,
    BOOLEAN counter_lock
)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Locks (write protected) TWD module configuration                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    if (config_lock)
    {
        SET_REG_FIELD(TWCFG, TWCFG_LTWCFG, TRUE);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Locks (write protected) the pre-scale ratio                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    if (prescale_lock)
    {
        SET_REG_FIELD(TWCFG, TWCFG_LTWCP, TRUE);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Locks (write protected) TWDT0 counter and it's control                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    if (counter_lock)
    {
        SET_REG_FIELD(TWCFG, TWCFG_LTWDT0, TRUE);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_TimerGetMdiv                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         mdiv value.                                                                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the current value of the pre-scale ratio (TWCP.MDIV) of the input */
/*                  clock.                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
UINT8 TWD_TimerGetMdiv (void)
{
    return (READ_REG_FIELD(TWCP, TWCP_MDIV));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatcDogConfig                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  preset     - WD counter value.                                                         */
/*                  source     - Source clock for WD - T0IN/T0OUT.                                         */
/*                  reset      - Type of reset generated by a watchdog event.                              */
/*                  data_match - Boolean indicating whether to use Data Match mechanism.                   */
/*                  wdiv       - watch-dog Prescaler value.                                                */
/*                                                                                                         */
/* Returns:         DEFS Error code.                                                                       */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the WD service - counter value, source clock and whether to    */
/*                  use data match mechanism.                                                              */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TWD_WatchDogConfig (
    UINT8            preset,
    TWD_WD_SOURCE_T  source,
    TWD_WD_RESET_T   reset,
    BOOLEAN          data_match,
    UINT8            wdiv
)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Parameters check                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    DEFS_STATUS_COND_CHECK(preset > 0, DEFS_STATUS_INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Keep WD counter on global variable for TWD_WatchDogRestart()                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    TWD_wd_counter = preset;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear Watchdog Configuration Register                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(TWCFG, 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set source clock                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(TWCFG, TWCFG_WDCT0I, source);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Whether to use data match                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(TWCFG, TWCFG_WDSDME, data_match);
    TWD_wd_data_match = data_match;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set reset type                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(TWCFG, TWCFG_WDRST_MODE, reset);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the watch-dog Prescaler value                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_REG_FIELD(WDCP, WDCP_WDIV, wdiv);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set WDCNT register to WD counter value                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(WDCNT, preset);

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogStart                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  ms - number of milliseconds                                                            */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK if the TWD was set correctly, otherwise returns DEFS_STATUS_FAIL        */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Start the WatchDog countdown with the "ms" milliseconds.                               */
/*                  Support up to 65 seconds (65535 ms) however note that:                                 */
/*                  if ms <= 256 the timer resolution is 1ms                                               */
/*                  if   2^n =< ms < 2^(n+1)  the resolution is +- 2^(n-7) ms                              */
/*                  if   512 =< ms < 1024  the resolution is up to +-4ms                                   */
/*                  if  1024 =< ms < 2048  the resolution is up to +-8ms                                   */
/*                  if  2048 =< ms < 4096  the resolution is up to +-16ms                                  */
/*                  if  4096 =< ms < 8192  the resolution is up to +-32ms                                  */
/*                  if  8192 =< ms < 16384 the resolution is up to +-64ms                                  */
/*                  if 16384 =< ms < 32768 the resolution is up to +-128ms                                 */
/*                  if 32768 =< ms < 65536 the resolution is up to +-256ms                                 */
/*                  For example setting ms to count to 5000ms will result in 4992 ms , since it is the     */
/*                  closest number to 5000 which can be divided to 32ms ("the resolution")                 */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TWD_WatchDogStart (UINT16 ms)
{
    UINT    i;
    UINT8   mdiv;
    UINT8   wdiv = 5;

    for (i = 0; i < 8 ; i++)
    {
        if (ms <= 0xFF)
        {
            mdiv = TWD_TimerGetMdiv();

            if (mdiv > wdiv)
            {
                return DEFS_STATUS_FAIL;
            }

            wdiv = wdiv - mdiv;
            DEFS_STATUS_RET_CHECK(TWD_WatchDogConfig((UINT8)ms, TIMER_IN, COLD_RESET, FALSE, wdiv));
            return DEFS_STATUS_OK;
        }
        ms >>= 1;
        wdiv++;
    }

    return DEFS_STATUS_FAIL;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_DisableTooEarlyTouch                                                               */
/*                                                                                                         */
/* Parameters:      disVal - TRUE : To Disable the "Too Early Touch" mechanism                             */
/*                         - FALSE: To Ebable the "Too Early Touch" mechanism                              */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Disables (or enable) the WD "Too Early Touch" mechanism                                */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_DisableTooEarlyTouch (BOOLEAN disVal)
{
    UINT8 t0csr;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read TWDT0 Control and Status Register                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    t0csr = REG_READ(T0CSR);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Retain Watchdog status state (R/W1C bit), in order not to affect WatchDog Reset Occurred Indication */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_VAR_FIELD(t0csr, T0CSR_WDRST_STS, FALSE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure Too Early Touch Disable (TESDIS) bit                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_VAR_FIELD(t0csr, T0CSR_TESDIS, disVal);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Write TWDT0 Control and Status Register                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(T0CSR, t0csr);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogConfigLock                                                                 */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine locks (write protected) WD count register (WDCNT).                        */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_WatchDogConfigLock (void)
{
    SET_REG_FIELD(TWCFG, TWCFG_LWDCNT, TRUE);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogRestart                                                                    */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine restarts/touches the WD service.                                          */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_WatchDogRestart (void)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* If data match mechanism is enabled restart is done by writing WD_DATA_MATCH_VAL to WDSDM            */
    /*-----------------------------------------------------------------------------------------------------*/
    if (TWD_wd_data_match)
    {
        REG_WRITE(WDSDM, WD_DATA_MATCH_VAL);
    }
    /*-----------------------------------------------------------------------------------------------------*/
    /* If data match mechanism is disabled restart is done by writing the count down value to WDCNT        */
    /*-----------------------------------------------------------------------------------------------------*/
    else
    {
        REG_WRITE(WDCNT, TWD_wd_counter);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogUpdate                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  preset - new WD count down value.                                                      */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine Update the WD count-down value.                                           */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_WatchDogUpdate (UINT8 preset)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Keep the new WD counter value on the global var (for TWD_wd_restart)                                */
    /*-----------------------------------------------------------------------------------------------------*/
    TWD_wd_counter = preset;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set WDCNT register to the updated WD counter value                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(WDCNT, preset);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogStop                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine stops watchdog operation by performing a stop sequence.                   */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_WatchDogStop (void)
{
    REG_WRITE(WDSDM, 0x87);
    REG_WRITE(WDSDM, 0x61);
    REG_WRITE(WDSDM, 0x63);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_UnlockRegisters                                                                    */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine unlocks TWD registers if they have been locked.                           */
/*                  Note that watchdog mechanism should be stopped before unlocking TWD registers.         */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_UnlockRegisters (void)
{
    TWD_WatchDogStop();
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogResetOccurred                                                              */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         TRUE in case Watch-Dog reset was generated; FALSE otherwise.                           */
/* Side effects:    In case Watch-Dog reset occurred, clear the Watch-Dog Reset Status bit.                */
/* Description:                                                                                            */
/*                  This routine checks whether a Watch-Dog reset occurred.                                */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN TWD_WatchDogResetOccurred (void)
{
    return (READ_REG_FIELD(T0CSR, T0CSR_WDRST_STS) == 1);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogResetClearEvent                                                            */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none.                                                                                  */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine clears the Watch-Dog Reset Status bit.                                    */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_WatchDogResetClearEvent (void)
{
    SET_REG_FIELD(T0CSR, T0CSR_WDRST_STS, TRUE);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_TimerHandler                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  int_num - number of triggered interrupt                                                */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  System Tick handler function - invokes the application callback function,              */
/*                  and clears the interrupt.                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_TimerHandler (_UNUSED_ UINT16 int_num)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Invoke the application callback                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    EXECUTE_FUNC(TWD_timer_callback, ());
}

#ifdef TWD_CAPABILITY_TIMER_READ_COUNTER
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_TimerGetCounter                                                                    */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         Timer counter value.                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the value of the T0OUT counter (TWMT0).                           */
/*---------------------------------------------------------------------------------------------------------*/
UINT16 TWD_TimerGetCounter (void)
{
    return (REG_READ(TWMT0));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_TimerTimeRemaining                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  time - remaining time in msec ptr.                                                     */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the time remaining until the T0 counter reaches 0.                */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TWD_TimerTimeRemaining (UINT32* time)
{
    UINT16 i;
    UINT16 prevT0;
    UINT16 curT0 = TWD_TimerGetCounter();

    for (i = 0; i < TWD_GET_TIMER_TIMEOUT; i++)
    {
        prevT0 = curT0;
        curT0  = TWD_TimerGetCounter();

        if (prevT0 == curT0)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* the time remaining until the T0 counter reaches 0 is: (T0_COUNT + 1) x T0IN-cycle.          */
            /*---------------------------------------------------------------------------------------------*/
            *time = (((UINT32)(((UINT32)(curT0 + 1)) * (1 << (READ_REG_FIELD(TWCP, TWCP_MDIV))) * 1000)) /
                    ((UINT32)MAX_CLOCK_FREQ));

            return DEFS_STATUS_OK;
        }
    }

    return DEFS_STATUS_RESPONSE_TIMEOUT;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_TimerTimeRemaining_us                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  time - remaining time in usec ptr.                                                     */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the time remaining until the T0 counter reaches 0.                */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS TWD_TimerTimeRemaining_us (UINT32* time)
{
    UINT16 i;
    UINT16 prevT0;
    UINT16 curT0 = TWD_TimerGetCounter();

    for (i = 0; i < TWD_GET_TIMER_TIMEOUT; i++)
    {
        prevT0 = curT0;
        curT0  = TWD_TimerGetCounter();

        if (prevT0 == curT0)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* the time remaining until the T0 counter reaches 0 is: (T0_COUNT + 1) x T0IN-cycle.          */
            /*---------------------------------------------------------------------------------------------*/
            *time = (((UINT32)(((UINT32)(curT0 + 1)) * (1 << (READ_REG_FIELD(TWCP, TWCP_MDIV))) * 1000000)) /
                    ((UINT32)MAX_CLOCK_FREQ));

            return DEFS_STATUS_OK;
        }
    }

    return DEFS_STATUS_RESPONSE_TIMEOUT;
}
#endif

#ifdef TWD_CAPABILITY_WD_READ_COUNTER
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogGetCounter                                                                 */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         Watchdog counter value.                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the value of the WDCNT counter (TWMWD).                           */
/*---------------------------------------------------------------------------------------------------------*/
UINT8 TWD_WatchDogGetCounter (void)
{
    return (REG_READ(TWMWD));
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogIsRunning                                                                  */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         TRUE  - WatchDog is enabled and running (counting down).                               */
/*                  FALSE - otherwise                                                                      */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks & returns is the WatchDog is running or not                        */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN TWD_WatchDogIsRunning (void)
{
    return (READ_REG_FIELD(T0CSR, T0CSR_WD_RUN) == 1);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_PrintRegs                                                                          */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_PrintRegs (void)
{
    HAL_PRINT("/*--------------*/\n");
    HAL_PRINT("/*     TWD      */\n");
    HAL_PRINT("/*--------------*/\n\n");

    HAL_PRINT("TWCFG               = 0x%02X\n", REG_READ(TWCFG));
    HAL_PRINT("TWCP                = 0x%02X\n", REG_READ(TWCP));
    HAL_PRINT("TWDT0               = 0x%04X\n", REG_READ(TWDT0));
    HAL_PRINT("T0CSR               = 0x%02X\n", REG_READ(T0CSR));
#ifdef TWD_CAPABILITY_TIMER_READ_COUNTER
    HAL_PRINT("TWMT0               = 0x%04X\n", REG_READ(TWMT0));
#endif
#ifdef TWD_CAPABILITY_WD_READ_COUNTER
    HAL_PRINT("TWMWD               = 0x%02X\n", REG_READ(TWMWD));
#endif
    HAL_PRINT("WDCP                = 0x%02X\n", REG_READ(WDCP));
#ifdef TWD_CAPABILITY_VERSION
    HAL_PRINT("TWD_VER             = 0x%02X\n", REG_READ(TWD_VER));
#endif

    HAL_PRINT("\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_PrintVersion                                                                       */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void TWD_PrintVersion (void)
{
    HAL_PRINT("TWD         = %X\n", MODULE_VERSION(TWD_MODULE_TYPE));
}

#if defined PMC_MODULE_TYPE
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_IdleModeCallback                                                                   */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         DEFS Status                                                                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine handles TWD Prerequisites for before entering Idle or Deep Idle mode.     */
/*---------------------------------------------------------------------------------------------------------*/
static DEFS_STATUS TWD_IdleModeCallback (void)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Before entering Idle or Deep Idle mode, the firmware must verify that a write to any WDT register   */
    /* is not pending or in progress. To do this, the firmware must verify that Watchdog Last Touch Delay  */
    /* is clear                                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    while (TWD_WatchDogTouched()) ;

    return DEFS_STATUS_OK;
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_WatchDogTouched                                                                    */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         TRUE in case Watch-Dog was "touched"; FALSE otherwise.                                 */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine checks whether a valid Watch-Dog "touch" operation is now performed.      */
/*                  This is important when switching to Idle or Deep Idle mode.                            */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN TWD_WatchDogTouched (void)
{
    return (BOOLEAN)(READ_REG_FIELD(T0CSR, T0CSR_WDLTD));
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        TWD_Reload                                                                             */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         DEFS Status                                                                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reloads and restarts the TWD Timer.                                       */
/*---------------------------------------------------------------------------------------------------------*/
static DEFS_STATUS TWD_Reload (void)
{
    UINT16 volatile saved_psr = 0;
    UINT16 volatile timeout;
    UINT8           t0csr;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Ensure no context switch is performed when performing a Reload operation.                           */
    /* Otherwise, we may miss RST toggling                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPTS_SAVE_DISABLE(saved_psr);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read TWDT0 Control and Status Register                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    t0csr = REG_READ(T0CSR);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Retain Watchdog status state (R/W1C bit), in order not to affect WatchDog Reset Occurred Indication */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_VAR_FIELD(t0csr, T0CSR_WDRST_STS, FALSE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set RST bit for start counting                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_VAR_FIELD(t0csr, T0CSR_RST, TRUE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Start counting                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    REG_WRITE(T0CSR, t0csr);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait till RST is toggled successfully (set and cleared) to ensure TWDT0 is loaded with new PRESET   */
    /* value, and not old PRESET value is taken                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    timeout = TWD_RST_TIMEOUT;

    do
    {
        if (--timeout == 0)
        {
            INTERRUPTS_RESTORE(saved_psr);
            return DEFS_STATUS_RESPONSE_TIMEOUT;
        }
    }
    while (! READ_REG_FIELD(T0CSR, T0CSR_RST));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait till RST is cleared                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    timeout = TWD_RST_TIMEOUT;

    do
    {
        if (--timeout == 0)
        {
            INTERRUPTS_RESTORE(saved_psr);
            return DEFS_STATUS_RESPONSE_TIMEOUT;
        }
    }
    while (READ_REG_FIELD(T0CSR, T0CSR_RST));

    INTERRUPTS_RESTORE(saved_psr);

    return DEFS_STATUS_OK;
}

