/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2010-2024 by Nuvoton Technology Corporation                                              */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   gpio_tip_if.h                                                                                         */
/*            This file contains General Purpose I/O (GPIO) interface                                      */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef _GPIO_TIP_IF_H
#define _GPIO_TIP_IF_H

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 INCLUDES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include __CHIP_H_FROM_IF()

#if defined GPIO_TIP_MODULE_TYPE
#include __MODULE_HEADER(gpio_tip, GPIO_TIP_MODULE_TYPE)
#endif


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES & DEFINITIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Input related types                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum
{
    GPIO_TIP_EVENT_RISING_EDGE,
    GPIO_TIP_EVENT_FALLING_EDGE,
    GPIO_TIP_EVENT_HIGH_LEVEL,
    GPIO_TIP_EVENT_LOW_LEVEL,
    GPIO_TIP_EVENT_BOTH_EDGES,
    GPIO_TIP_EVENT_EDGE,
    GPIO_TIP_EVENT_NONE
} GPIO_TIP_EVENT_T;

typedef void (*GPIO_TIP_EVENT_HANDLE_T)(void* args);


/*---------------------------------------------------------------------------------------------------------*/
/* GPIO Direction                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef GPIO_TIP_DIR_T
typedef enum
{
    GPIO_TIP_DIR_INPUT = 0,
    GPIO_TIP_DIR_OUTPUT,
} GPIO_TIP_DIR_T;
#endif

typedef enum
{
    GPIO_TIP_BLINK_OFF          = 0x0,
    GPIO_TIP_BLINK_0_5_TO_1     = 0x1,
    GPIO_TIP_BLINK_1_TO_2       = 0x2,
    GPIO_TIP_BLINK_1_TO_4       = 0x3,
    GPIO_TIP_BLINK_0_25_TO_0_5  = 0x4,
    GPIO_TIP_BLINK_0_25_TO_1    = 0x5,
    GPIO_TIP_BLINK_3_TO_4       = 0x6,
    GPIO_TIP_BLINK_0_75_TO_1    = 0x7,
} GPIO_TIP_BLINK_T;


/*---------------------------------------------------------------------------------------------------------*/
/* GPIO Pull-Up/Down                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef GPIO_TIP_PULL_T
typedef enum
{
    GPIO_TIP_PULL_NONE = 0,
    GPIO_TIP_PULL_UP,
    GPIO_TIP_PULL_DOWN
} GPIO_TIP_PULL_T;
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Output buffer type                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef GPIO_TIP_OTYPE_T
typedef enum
{
    GPIO_TIP_OTYPE_PUSH_PULL = 0,
    GPIO_TIP_OTYPE_OPEN_DRAIN,
} GPIO_TIP_OTYPE_T;
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Build a GPIO number given its Port and Bit number                                                       */
/* The GPIO number is represented in hexadecimal                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define GPIO_TIP_GET_BIT_NUM(gpio)      LSN(gpio)

/*---------------------------------------------------------------------------------------------------------*/
/* Specify GPIO Port Number                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define GPIO_TIP_GET_PORT_NUM(gpio)     MSN(gpio)

/*---------------------------------------------------------------------------------------------------------*/
/* Structure for holding GPIO status                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct GPIO_TIP_STAT {
    UINT8        dataIn;
    UINT8        dataOut;
    GPIO_TIP_DIR_T   dir;
    GPIO_TIP_PULL_T  pull;
    GPIO_TIP_OTYPE_T OutputType;
#if defined GPIO_CAPABILITY_VDD_DRIVEN
    UINT8        VddDrv;        /* 1  - VDD Driven, 0 - Not VDD Driven */
#endif
#if defined GPIO_CAPABILITY_LOCK
    UINT8        locked;        /* 1  - Locked, 0 - Not Locked */
#endif
#if defined (GPIO_TIP_EVENT_SUPPORT) || (defined (MIWU_MODULE_TYPE) && defined (GPIO_MIWU_INTERRUPT_SUPPORT))
    UINT8        interruptPolarity;
    BOOLEAN      interruptEnabled;
    #if defined (GPIO_TIP_EVENT_SUPPORT)
            BOOLEAN      interruptIsEdge; 
            BOOLEAN      interruptBothEdges; 
    #endif
#endif
} GPIO_TIP_STAT_T;

#if defined GPIO_TIP_EVENT_SUPPORT
#define GPIO_TIP_INTR_RISING     GPIO_TIP_EVENT_RISING_EDGE
#define GPIO_TIP_INTR_FALLING    GPIO_TIP_EVENT_FALLING_EDGE
#define GPIO_TIP_INTR_ANY_EDGE   GPIO_TIP_EVENT_BOTH_EDGES
#define GPIO_TIP_INTR_EDGE       GPIO_TIP_EVENT_BOTH_EDGES
#define GPIO_TIP_INTR_LOW        GPIO_TIP_EVENT_LOW_LEVEL
#define GPIO_TIP_INTR_HIGH       GPIO_TIP_EVENT_HIGH_LEVEL
#define GPIO_TIP_INTR_NONE       GPIO_TIP_EVENT_NONE
#endif 

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS TIP GPIO                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#if defined (GPIO_TIP_EVENT_SUPPORT)
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ClearInterrupt                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to clear                                                       */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Clears a pending event of a given GPIO input                                           */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ClearInterrupt (UINT gpio);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_InstallHandler                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to configure                                                   */
/*                  handler   - gpio handler procedure to be installed.                                    */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Install a handler function to the relevant GPIO.                                       */
/*                  The interrupt itself is not enabled and not configured                                 */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_InstallHandler (UINT gpio, GPIO_TIP_HANDLER_T handler);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_SetInterruptType                                                              */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to configure                                                   */
/*                  intType   - Interrupt type, on of the following:                                       */
/*                              GPIO_TIP_INTR_RISING, GPIO_TIP_INTR_FALLING, GPIO_TIP_INTR_ANY_EDGE        */
/*                              GPIO_TIP_INTR_LOW, GPIO_TIP_INTR_HIGH                                      */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Configure the interrupt type for the specific GPIO                                     */
/*                  The interrupt itself is not enabled                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_SetInterruptType (UINT gpio, UINT intType);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_SetInterruptPolarity                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to configure                                                   */
/*                  polType   - polarity type,based on of the following:                                   */
/*                              GPIO_TIP_INTR_RISING, GPIO_TIP_INTR_FALLING, GPIO_TIP_INTR_ANY_EDGE        */
/*                              GPIO_TIP_INTR_LOW, GPIO_TIP_INTR_HIGH                                      */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Configure the interrupt type for the specific GPIO                                     */
/*                  The interrupt itself is not enabled                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_SetInterruptPolarity (UINT gpio, UINT intType);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigInterrupt                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to configure                                                   */
/*                  intType   - Interrupt type, one of the following:                                      */
/*                              GPIO_TIP_INTR_RISING, GPIO_TIP_INTR_FALLING, GPIO_TIP_INTR_ANY_EDGE        */
/*                              GPIO_TIP_INTR_LOW, GPIO_TIP_INTR_HIGH                                      */
/*                  handler   - gpio handler procedure to be installed.                                    */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Configure the interrupt type for the specific GPIO                                     */
/*                  Install the handler and enable the relevant interrupt.                                 */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigInterrupt (UINT gpio, UINT intType, GPIO_TIP_HANDLER_T handler);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_EnableInterrupt                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio     - gpio number in hex e.g 0x84 = GPIO84                                        */
/*                  enable   - TRUE to enable the gpio interrupt ; FALSE to disable.                       */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Description:     This routine enables/disables a given gpio interrupt.                                  */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_EnableInterrupt (UINT gpio ,BOOLEAN enable);

#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_Init                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      - GPIO number to configure                                                   */
/*                  gpioDir   - GPIO direction (IN/OUT)                                                    */
/*                  pullDir   - Pull-up/down enabling or disabling                                         */
/*                  outType   - GPIO pin Output buffer type OPEN DRAIN or PUSH PULL                        */
/*                  vddDriven - Enable if VDD driven                                                       */
/*                  lock      - Lock the GPIO mux settings                                                 */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs GPIO initialization                                              */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_Init (UINT gpio, GPIO_TIP_DIR_T gpioDir, GPIO_TIP_PULL_T pullDir, GPIO_TIP_OTYPE_T outType, BOOLEAN vddDriven, BOOLEAN lock);

#if defined GPIO_TIP_CAPABILITY_OPEN_DRAIN
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigOutBufferType                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                  outType - GPIO pin Output buffer type OPEN DRAIN or PUSH PULL                          */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the GPIO output buffer type                                    */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigOutBufferType (UINT gpio, GPIO_TIP_OTYPE_T outType);
#endif

#if defined GPIO_TIP_CAPABILITY_MUX_IN_GPIO_TIP_MODULE

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_Mux                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio      -    GPIO pin number                                                         */
/*                  isGPIO    -    boolean indicating whether the GPIO function should be enabled or       */
/*                  disabled. TRUE means GPIO, FALSE means other function                                  */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine ...                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_Mux (UINT gpio, BOOLEAN isGPIO);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_MuxPort                                                                       */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port      -   GPIO port number                                                         */
/*                  isGPIO    -    boolean indicating whether the GPIO function should be enabled or       */
/*                  disabled. TRUE means GPIO, FALSE means other function                                  */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine ...                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_MuxPort (UINT port, BOOLEAN isGPIO);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_GetMuxState                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    -    The number of the GPIO queried                                            */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine is a getter for the muxing state of a given GPIO                          */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN GPIO_TIP_GetMuxState (UINT gpio);

#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigDir                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio     - GPIO number.                                                                */
/*                  gpioDir  - GPIO direction [INPUT(0) or OUTPUT(1)].                                     */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the direction (INPUT or OUTPUT) of a given GPIO pin.           */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigDir (UINT gpio, GPIO_TIP_DIR_T gpioDir);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigPull                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio     - GPIO number.                                                                */
/*                  pullDir  - Pull selection [GPIO_TIP_PULL_NONE (0) or GPIO_TIP_PULL_UP(1) or GPIO_TIP_PULL_DOWN(2)] */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables/disables the pull up feature of the given GPIO pin and configures */
/*                  it to pull-up or pull-down (when applicable).                                          */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigPull (UINT gpio, GPIO_TIP_PULL_T pullDir);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_GetDir                                                                        */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                                                                                                         */
/* Returns:         GPIO pin direction                                                                     */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the GPIO pin direction (INPUT or OUTPUT)                          */
/*---------------------------------------------------------------------------------------------------------*/
GPIO_TIP_DIR_T GPIO_TIP_GetDir (UINT gpio);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_GetPullUp                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    -    The number of the GPIO queried                                            */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine is a getter for the pullup on a given GPIO                                */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN GPIO_TIP_GetPullUp (UINT gpio);

#if defined GPIO_TIP_CAPABILITY_VDD_DRIVEN

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigVddDriven                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio       - GPIO pin number.                                                          */
/*                  vddDriven  - TRUE to enable GPIO by VDD Present Control; FALSE otherwise.              */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures a given GPIO pin to be VDD-driven.                             */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigVddDriven (UINT gpio, BOOLEAN vddDriven);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_GetVddDriven                                                                  */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    -    The number of the GPIO queried                                            */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine is a getter for the vdd Driven state of a given GPIO                      */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN GPIO_TIP_GetVddDriven (UINT gpio);

#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_Read                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio - GPIO pin number.                                                                */
/*                                                                                                         */
/* Returns:         Read value (1 or 0).                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reads data from a given GPIO pin.                                         */
/*---------------------------------------------------------------------------------------------------------*/
UINT GPIO_TIP_Read (UINT gpio);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_Write                                                                         */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio     - GPIO number.                                                                */
/*                  pin_data - Value to be written (0 or 1).                                               */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine writes data to a given GPIO port.                                         */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_Write (UINT gpio, UINT pin_data);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ReadOutput                                                                    */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio - GPIO pin number.                                                                */
/*                                                                                                         */
/* Returns:         Read value (1 or 0).                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reads data from output buffer of the given GPIO pin.                      */
/*---------------------------------------------------------------------------------------------------------*/
UINT GPIO_TIP_ReadOutput (UINT gpio);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_InitPort                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port          - GPIO port number.                                                      */
/*                  dirMask       - GPIO port direction mask.                                              */
/*                  pullEnMask    - GPIO port pull-up enable mask.                                         */
/*                  pullDirMask   - GPIO port pull-up direction mask.                                      */
/*                  vddDrivenMask - GPIO port VDD Present Control mask.                                    */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine performs GPIO initialization for a given GPIO port.                       */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_InitPort (UINT port, UINT dirMask, UINT pullEnMask, UINT pullDirMask, UINT vddDrivenMask);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigPortMux                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port    - GPIO port number.                                                            */
/*                  dirMask - GPIO port alternate mask.                                                    */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the alternate (ALT or GPIO) of all GPIO pins in a given        */
/*                  GPIO port according to a given mask, as follows:                                       */
/*                  Bit 'n' with value 0/1 configures GPIO pin 'n' as GPIO/ALT, respectively.              */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigPortMux (UINT port, UINT muxMask);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigPortPushPull                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port         - GPIO port number.                                                       */
/*                  pushpullMask - GPIO port PushPull mask.                                                */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the push-pull mask of all GPIO pins                            */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigPortPushPull (UINT port, UINT pushpullMask);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigPortDir                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port    - GPIO port number.                                                            */
/*                  dirMask - GPIO port direction mask.                                                    */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the direction (INPUT or OUTPUT) of all GPIO pins in a given    */
/*                  GPIO port according to a given direction mask, as follows:                             */
/*                  Bit 'n' with value 0/1 configures GPIO pin 'n' as INPUT/OUTPUT, respectively.          */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigPortDir (UINT port, UINT dirMask);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigPortPull                                                                */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port        - GPIO port number.                                                        */
/*                  pullEnMask  - GPIO port pull-up enable mask.                                           */
/*                  pullDirMask - GPIO port pull-up direction mask.                                        */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine enables/disables the pull up feature of all GPIO pins in a given GPIO     */
/*                  port according to a given pull-up enable mask, as follows:                             */
/*                  Bit 'n' with value 0/1 configures GPIO pin 'n' as ENABLE/DISABLE, respectively.        */
/*                                                                                                         */
/*                  It also and configures the pull-up direction (PULL_UP/PULL_DOWN) of all GPIO pins      */
/*                  (when applicable) according to a given pull-up direction mask, as follows:             */
/*                  Bit 'n' with value 0/1 configures GPIO pin 'n' as PULL_UP/PULL_DOWN, respectively.     */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigPortPull (UINT port, UINT pullEnMask, UINT pullDirMask);

#if defined GPIO_TIP_CAPABILITY_VDD_DRIVEN
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ConfigPortVddDriven                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port          - GPIO port number.                                                      */
/*                  vddDrivenMask - GPIO port VDD Present Control mask.                                    */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine configures the VDD Presence Control (VDD_DRIVEN or VCC_DRIVEN) of all     */
/*                  GPIO pins in a given GPIO port according to a given VDD Present Control mask, as       */
/*                  follows:                                                                               */
/*                  Bit 'n' with value 0/1 configures GPIO pin 'n' as VCC_DRIVEN/VDD_DRIVEN, respectively. */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_ConfigPortVddDriven (UINT port, UINT vddDrivenMask);
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_ReadPort                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port - GPIO port number.                                                               */
/*                                                                                                         */
/* Returns:         Read value.                                                                            */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine reads data of a given GPIO port.                                          */
/*---------------------------------------------------------------------------------------------------------*/
UINT GPIO_TIP_ReadPort (UINT port);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_WritePort                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port  - GPIO port number.                                                              */
/*                  value - Value to be written.                                                           */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine writes data to a given GPIO port.                                         */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_WritePort (UINT port, UINT value);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_WritePortMask                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port  - GPIO port number.                                                              */
/*                  value - Value to be written.                                                           */
/*                  mask  - mask of bits to write                                                          */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine writes data to a given GPIO port.                                         */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_WritePortMask (UINT port, UINT value, UINT mask);

#if defined GPIO_TIP_CAPABILITY_LOCK
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_Lock                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio - GPIO pin number.                                                                */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine locks (i.e., disables writing to) the output data (in PxDOUT register)    */
/*                  and the configuration (in PxDIR, PxPULL, PxPUD, PxENVDD, PxOTYPE registers) for a      */
/*                  given GPIO                                                                             */
/*                  pin. (The data in PxDIN register, which is read-only, is not affected.)                */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_Lock (UINT gpio);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_isLocked                                                                      */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                                                                                                         */
/* Returns:         TRUE - if GPIO is locked                                                               */
/*                  FALSE - if GPIO is not locked                                                          */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the GPIO locj status.                                             */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN GPIO_TIP_isLocked (UINT gpio);
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_GetPullType                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                                                                                                         */
/* Returns:         GPIO_TIP_PULL_NONE, GPIO_TIP_PULL_UP, GPIO_TIP_PULL_DOWN                               */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the GPIO PULL Status                                              */
/*---------------------------------------------------------------------------------------------------------*/
GPIO_TIP_PULL_T GPIO_TIP_GetPullType (UINT gpio);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_GetOutputType                                                                 */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                                                                                                         */
/* Returns:         GPIO_TIP_OTYPE_PUSH_PULL, GPIO_TIP_OTYPE_OPEN_DRAIN                                    */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the GPIO Output Type (Push Pull or Open Drain)                    */
/*---------------------------------------------------------------------------------------------------------*/
GPIO_TIP_OTYPE_T GPIO_TIP_GetOutputType (UINT gpio);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_GetStatus                                                                     */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                  stat    - pointer to a GPIO_TIP_STAT_T structure                                       */
/*                                                                                                         */
/* Returns:         None                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine update the "stat" structure with the current GPIO status.                 */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_GetStatus (UINT gpio, GPIO_TIP_STAT_T* stat);

#if defined (GPIO_TIP_EVENT_SUPPORT)
/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_GetInterruptStatus                                                            */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  gpio    - GPIO pin number.                                                             */
/*                                                                                                         */
/* Returns:         boolean: true if interrupt is active, otherwise false                                  */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine returns the GPIO interrupt status                                         */
/*---------------------------------------------------------------------------------------------------------*/
BOOLEAN GPIO_TIP_GetInterruptStatus (UINT gpio);
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_PrintRegs                                                                     */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module registers                                               */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_PrintRegs (void);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_PrintModuleRegs                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  port - The port to be printed.                                                         */
/*                                                                                                         */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module instance registers                                      */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_PrintModuleRegs (UINT port);

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        GPIO_TIP_PrintVersion                                                                  */
/*                                                                                                         */
/* Parameters:      none                                                                                   */
/* Returns:         none                                                                                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine prints the module version                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void GPIO_TIP_PrintVersion (void);

#endif  /* _GPIO_TIP_IF_H */

