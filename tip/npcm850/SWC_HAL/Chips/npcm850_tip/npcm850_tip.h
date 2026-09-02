/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2020 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*   npcm850_tip.h                                                                                         */
/*            This file contains chip definitions for the given project                                    */
/* Project:                                                                                                */
/*            SWC HAL                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef __NPCM850_TIP_H_
#define __NPCM850_TIP_H_

/*---------------------------------------------------------------------------------------------------------*/
/* Chip Name                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define NPCM850_TIP

#include "../npcm850/npcm850.h"


/* special flag for fusing the OTP into RAM3 area instead of actual OTP , for debug purposes */
#ifndef TIP_DUMMY_OTP_ON_RAM
#define TIP_DUMMY_OTP_ON_RAM 0 /* in production should be 0 */
#endif

#define FUSE_OTP 0 /* in production should be 1 */

#if (TIP_DUMMY_OTP_ON_RAM == 1)
#undef FUSE_OTP
#define FUSE_OTP 1 /* since we are in virtual we ignore previews decision and always program to virtual OTP */
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                              CHIP                                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                              MEMORIES                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* ROM                                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_ROM_BASE_ADDR                   0x00000000
#define TIP_ROM_MEMORY_SIZE                 _128KB_

/*---------------------------------------------------------------------------------------------------------*/
/* TIP_RAM access window: Mapped to TIP_RAM directly, in the TIP subsystem                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_RAM_BASE_ADDR                   0x00020000
#define TIP_RAM_CODE_ADDR                   (TIP_RAM_BASE_ADDR)
#define TIP_RAM_CODE_SIZE                   (_256KB_)
#define TIP_RAM_CODE_END                    (TIP_RAM_CODE_ADDR + TIP_RAM_CODE_SIZE)
#define TIP_RAM_DATA_ADDR                   (TIP_RAM_CODE_END)
#define TIP_RAM_DATA_SIZE                   (_64KB_)
#define TIP_RAM_MEMORY_SIZE                 (TIP_RAM_CODE_SIZE + TIP_RAM_DATA_SIZE)

/*---------------------------------------------------------------------------------------------------------*/
/* Trusted Integrated Processor PKA                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_PKA_BASE_ADDR                   0x00070000
#define TIP_PKA_MEMORY_SIZE                 _4KB_

/*---------------------------------------------------------------------------------------------------------*/
/* Trusted Integrated Processor SHA                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SHA_BASE_ADDR                   0x00071000
#define TIP_SHA_MEMORY_SIZE                 _4KB_

/*---------------------------------------------------------------------------------------------------------*/
/* Trusted Integrated Processor AES                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_AES_BASE_ADDR                   0x00072000
#define TIP_AES_MEMORY_SIZE                 _4KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC System RAM2 access window via SYS_AHB: RAM2 at system address FFFB_0000h (256KB), according to      */
/* SRAMWINC register                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_RAM2_BASE_ADDR                  0x00080000
#define TIP_RAM2_MEMORY_SIZE                _256KB_

/*---------------------------------------------------------------------------------------------------------*/
/* Reserved (IAD interrupt)                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_RESEREVED2_BASE_ADDR            0x000C0000

/*---------------------------------------------------------------------------------------------------------*/
/* BMC Peripheral access window 0 via SYS_AHB: Window size is to 128 KB, located in the top 256MB of system*/
/* memory map, according to PWIN0C1-2 registers                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_PERIPH_WIN0_BASE_ADDR           0x00100000
#define TIP_PERIPH_WIN0_MEMORY_SIZE         _128KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC Peripheral access window 1 via SYS_AHB: Window size is to 128 KB, located in the top 256MB of system*/
/* memory map, according to PWIN1C1-2 registers                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_PERIPH_WIN1_BASE_ADDR           0x00120000
#define TIP_PERIPH_WIN1_MEMORY_SIZE         _128KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC Peripheral access window 2 via SYS_AHB: Window size is up to 128 KB, located in the top 256MB of    */
/* system memory map, according to PWIN2C1-2 registers                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_PERIPH_WIN2_BASE_ADDR           0x00140000
#define TIP_PERIPH_WIN2_MEMORY_SIZE         _128KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC Peripheral access window 3 via SYS_AHB: Window size is up to 128 KB, located in the top 256MB of    */
/* system memory map, according to PWIN3C1-2 registers                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_PERIPH_WIN3_BASE_ADDR           0x00160000
#define TIP_PERIPH_WIN3_MEMORY_SIZE         _128KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC Peripheral access window 4 via SYS_AHB: Window size is up to 128 KB, located in the top 256MB of    */
/* system memory map, according to PWIN4C1-2 registers                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_PERIPH_WIN4_BASE_ADDR           0x00180000
#define TIP_PERIPH_WIN4_MEMORY_SIZE         _128KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC Peripheral access window 5 via SYS_AHB: Window size is up to 128 KB, located in the top 256MB of    */
/* system memory map, according to PWIN6C1-2 registers                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_PERIPH_WIN5_BASE_ADDR           0x001A0000
#define TIP_PERIPH_WIN5_MEMORY_SIZE         _128KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC Peripheral access window 6 via SYS_AHB: Window size is up to 128 KB, located in the top 256MB of    */
/* system memory map, according to PWIN6C1-2 registers                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_PERIPH_WIN6_BASE_ADDR           0x001C0000
#define TIP_PERIPH_WIN6_MEMORY_SIZE         _128KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC Peripheral access window 7 via SYS_AHB: Window size is up to 128 KB, located in the top 256MB of    */
/* system memory map, according to PWIN7C1-2 registers                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_PERIPH_WIN7_BASE_ADDR           0x001E0000
#define TIP_PERIPH_WIN7_MEMORY_SIZE         _128KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC system access window 0 via SYS_AHB: Window size is up to 2 MB, located anywhere within system memory*/
/* map, according to MMWIN0C1-2 registers                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SYS_WIN0_BASE_ADDR              0x00000000
#define TIP_SYS_WIN0_MEMORY_SIZE            _2MB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC system access window 1 via SYS_AHB: Window size is up to 2 MB, located anywhere within system memory*/
/* map, according to MMWIN1C1-2 registers                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SYS_WIN1_BASE_ADDR              0x00200000
#define TIP_SYS_WIN1_MEMORY_SIZE            _2MB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC system access window 2 via SYS_AHB: Window size is up to 2 MB, located anywhere within system memory*/
/* map, according to MMWIN2C1-2 registers                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SYS_WIN2_BASE_ADDR              0x00400000
#define TIP_SYS_WIN2_MEMORY_SIZE            _2MB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC system access window 3 via SYS_AHB: Window size is up to 2 MB, located anywhere within system memory*/
/* map, according to MMWIN3C1-2 registers                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SYS_WIN3_BASE_ADDR              0x00600000
#define TIP_SYS_WIN3_MEMORY_SIZE            _2MB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC system access window 4 via SYS_AHB: Window size is up to 2 MB, located anywhere within system memory*/
/* map, according to MMWIN4C1-2 registers                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SYS_WIN4_BASE_ADDR              0x00800000
#define TIP_SYS_WIN4_MEMORY_SIZE            _2MB_

/*---------------------------------------------------------------------------------------------------------*/
/* Reserved (IAD interrupt)                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_RESEREVED3_BASE_ADDR            0x00C00000

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip TWD Module Registers (on APB)                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_TWD_BASE_ADDR                   0x00FFF800
#define TIP_TWD_MEMORY_SIZE                 _128B_

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip TIP Control Registers (on APB)                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_CTRL_BASE_ADDR                  0x00FFF880
#define TIP_CTRL_MEMORY_SIZE                _128B_

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip UART Registers (on APB)                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_UART_BASE_ADDR                  0x00FFF900
#define TIP_UART_MEMORY_SIZE                _128B_

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip TIP Security Registers (on APB)                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SEC_BASE_ADDR                   0x00FFF980
#define TIP_SEC_MEMORY_SIZE                 _128B_

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip STC Registers (on APB)                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_STC_BASE_ADDR                   0x00FFFA00
#define TIP_STC_MEMORY_SIZE                 _128B_

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip RNG Registers (on APB)                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_RNG_BASE_ADDR                   0x00FFFA80
#define TIP_RNG_MEMORY_SIZE                 _128B_

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip I2C-1 Registers (on APB)                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_I2C_1_BASE_ADDR                 0x00FFFB00
#define TIP_I2C_1_MEMORY_SIZE               _128B_

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip I2C-2 Registers (on APB)                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_I2C_2_BASE_ADDR                 0x00FFFB80
#define TIP_I2C_2_MEMORY_SIZE               _128B_

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip FUSE Registers (on APB)                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_FUSE_BASE_ADDR                  0x00FFFC00
#define TIP_FUSE_MEMORY_SIZE                _128B_

/*---------------------------------------------------------------------------------------------------------*/
/* Reserved (IAD interrupt)                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_RESEREVED4_BASE_ADDR            0x00FFFC80

/*---------------------------------------------------------------------------------------------------------*/
/* BMC SPI0 and SPI1 access window via SYS_AHB: Window size is 128 MB per chip select, directly mapped,    */
/* according to SPI0WINC register                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SPI0_BASE_ADDR                  SPI0CS0_BASE_ADDR
#define TIP_SPI0_MEMORY_SIZE                _512MB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC SPI3 access window via SYS_AHB: Window size is 128 MB per chip select, directly mapped, according to*/
/* SPI3WINC register                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SPI3_BASE_ADDR                  SPI3CS0_BASE_ADDR
#define TIP_SPI3_MEMORY_SIZE                (_512MB_ + _64KB_)

/*---------------------------------------------------------------------------------------------------------*/
/* Reserved (IAD interrupt)                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_RESEREVED5_BASE_ADDR            0xC0010000

/*---------------------------------------------------------------------------------------------------------*/
/* BMC External DRAM access window 0 via SYS_AHB: Window size is up to 64 MB per chip select, mapped       */
/* directly according to DRAM0WINA register                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_DRAM_WIN0_BASE_ADDR             0xD0000000
#define TIP_DRAM_WIN0_MEMORY_SIZE           _64MB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC External DRAM access window 1 via SYS_AHB: Window size is up to 64 MB per chip select, mapped       */
/* directly according to DRAM0WINA register                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_DRAM_WIN1_BASE_ADDR             0xD4000000
#define TIP_DRAM_WIN1_MEMORY_SIZE           _64MB_

/*---------------------------------------------------------------------------------------------------------*/
/* Reserved (IAD interrupt)                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_RESEREVED6_BASE_ADDR            0xD8000000

/*---------------------------------------------------------------------------------------------------------*/
/* BMC SPIX access window via SYS_AHB: Window size is up to 32 MB, directly mapped, according to SPIXWIN   */
/* register                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SPIX_BASE_ADDR                  0xF8000000
#define TIP_SPIX_MEMORY_SIZE                _16MB_    /* 16MB * 2 CS~ */

/*---------------------------------------------------------------------------------------------------------*/
/* Reserved (IAD interrupt)                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_RESEREVED7_BASE_ADDR            0xFA000000

/*---------------------------------------------------------------------------------------------------------*/
/* BMC SPI0 registers access window via SYS_AHB: Window size is 4 KB, mapped directly according to         */
/* SPI01WIN register                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SPI0_REGS_BASE_ADDR             FIU0_BASE_ADDR
#define TIP_SPI0_REGS_MEMORY_SIZE           _4KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC SPIX module registers access window via SYS_AHB: Window size is 4 KB, mapped directly according to  */
/* SPIXWINC register                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SPIX_REGS_BASE_ADDR             FIUX_BASE_ADDR
#define TIP_SPIX_REGS_MEMORY_SIZE           _4KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC SPI1 registers access window via SYS_AHB: Window size is 4 KB, mapped directly according to         */
/* SPI01WIN register                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SPI1_REGS_BASE_ADDR             FIU1_BASE_ADDR
#define TIP_SPI1_REGS_MEMORY_SIZE           _4KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC SPI3 registers access window via SYS_AHB: Window size is 4 KB, mapped directly according to         */
/* SPI3WINC register                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_SPI3_REGS_BASE_ADDR             FIU3_BASE_ADDR
#define TIP_SPI3_REGS_MEMORY_SIZE           _4KB_

/*---------------------------------------------------------------------------------------------------------*/
/* BMC ROM (and A9 exception table) access window via SYS_AHB: Window size is up to 64 KB, directly mapped,*/
/* Read Only access Depends on GLBLEN bit in MISCWINC register (IAD interrupt if disabled)                 */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_BMC_ROM_BASE_ADDR               0xFFFF0000
#define TIP_BMC_ROM_MEMORY_SIZE             _64KB_


#define TIP_PERIPH_WIN_BASE_ADDR(module)   (TIP_PERIPH_WIN0_BASE_ADDR + (module) * (TIP_PERIPH_WIN0_MEMORY_SIZE))
#define TIP_SYS_WIN_BASE_ADDR(module)      (TIP_SYS_WIN0_BASE_ADDR    + (module) * (TIP_SYS_WIN0_MEMORY_SIZE))


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               CLOCKS                                                    */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/* Core Clock Frequency                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define CLKREF                              25000000
#define REFERENCE_CLK                       CLKREF
#define MAX_CORE_CLK                        1000000000

/*---------------------------------------------------------------------------------------------------------*/
/* Core Clock Low Frequency                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define LFCG_CORE_CLK                       32768
#define HFCG_CORE_CLK                       CLK_GetCPFreq()

/*---------------------------------------------------------------------------------------------------------*/
/* Number of cycles per iteration                                                                          */
/*---------------------------------------------------------------------------------------------------------*/

// Note : there is a HAL naming issue with the following parameter
// After fix, squeeze next lines:
#define HFCG_NUM_OF_CYCLES_PER_ITERATION    10
#define HAL_NUM_OF_CYCLES_PER_ITERATION     HFCG_NUM_OF_CYCLES_PER_ITERATION


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                              MODULE DRIVERS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Chip System Configuration                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_CTRL_ACCESS                 MEM

/*---------------------------------------------------------------------------------------------------------*/
/* Timer Watch-Dog Module                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define TWD_MODULE_TYPE                 4
#define TWD_ACCESS                      MEM
#define TWD_BASE_ADDR                   TIP_TWD_BASE_ADDR
#define TWD_INTERRUPT                   NVIC_INT_1
#define TWD_INTERRUPT_PROVIDER          INTERRUPT_PROVIDER_NVIC
#define TWD_INTERRUPT_POLARITY          INTERRUPT_POLARITY_RISING_EDGE
#define TWD_INTERRUPT_PRIORITY          1
#define TWD_SOURCE_CLOCK                ((UINT32)(CLK_GetCPFreq() / 16))

/*---------------------------------------------------------------------------------------------------------*/
/* System Control Space Module                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_MODULE_TYPE                 1
#define SCS_ACCESS                      MEM
#define SCS_BASE_ADDR                   0xE000E000

/*---------------------------------------------------------------------------------------------------------*/
/* Nested Vectored Interrupt Controller Module                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define NVIC_MODULE_TYPE                1
#define NVIC_ACCESS                     MEM
#define NVIC_BASE_ADDR                  0xE000E100
#define NVIC_TRAP_NUM                   16
#define NVIC_INTERRUPT_NUM              96
#define NVIC_PRIORITY_BITS              4
#define NVIC_MAX_NUM_OF_INTERRUPTS      96

/*---------------------------------------------------------------------------------------------------------*/
/* Memory Protection Unit                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define MPU_MODULE_TYPE                 1
#define MPU_ACCESS                      MEM
#define MPU_BASE_ADDR                   0xE000ED90

/*---------------------------------------------------------------------------------------------------------*/
/* On-Chip AES (SCA) Registers (on APB) - SecureIC                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#undef AES_MODULE_TYPE
#undef AES_BASE_ADDR
#undef AES_INTERRUPT_NUM
#undef AES_INTERRUPT_POLARITY
#undef AES_INTERRUPT_PRIORITY
#undef AES_INTERRUPT_PROVIDER
#undef AES_SOURCE_CLOCK
#undef AES_POWER_DOWN
#undef AES_NUM_OF_KEYS

#define AES_MODULE_TYPE                 7
#define AES_ACCESS                      MEM
#define AES_BASE_ADDR                   (TIP_AES_BASE_ADDR)
#define AES_INTERRUPT_NUM               NVIC_INT_30
#define AES_INTERRUPT_POLARITY          INTERRUPT_POLARITY_RISING_EDGE
#define AES_INTERRUPT_PRIORITY          0
#define AES_INTERRUPT_PROVIDER          INTERRUPT_PROVIDER_NVIC
#define AES_SOURCE_CLOCK                CLK_GetCPFreq()
#define AES_POWER_DOWN(power)
#define AES_NUM_OF_KEYS                 6

/*---------------------------------------------------------------------------------------------------------*/
/* Public Key Accelerator Module                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#undef PKA_BASE_ADDR
#undef PKA_INTERRUPT
#undef PKA_INTERRUPT_POLARITY
#undef PKA_INTERRUPT_PRIORITY
#undef PKA_INTERRUPT_PROVIDER
#undef PKA_SOURCE_CLOCK

#define PKA_BASE_ADDR                   TIP_PKA_BASE_ADDR
#define PKA_INTERRUPT                   NVIC_INT_87
#define PKA_INTERRUPT_POLARITY          INTERRUPT_POLARITY_RISING_EDGE
#define PKA_INTERRUPT_PRIORITY          0
#define PKA_INTERRUPT_PROVIDER          INTERRUPT_PROVIDER_NVIC
#define PKA_SOURCE_CLOCK                CLK_GetCPFreq()

/*---------------------------------------------------------------------------------------------------------*/
/* Secure Hash Algorithm (SHA) Module                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#undef SHA_BASE_ADDR
#undef SHA_SOURCE_CLOCK

#define SHA_BASE_ADDR                   (TIP_SHA_BASE_ADDR)
#define SHA_SOURCE_CLOCK(module)        CLK_GetCPFreq()

/*---------------------------------------------------------------------------------------------------------*/
/* Random Number Generator (RNG) Module                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#undef RNG_BASE_ADDR
#undef RNG_INTERRUPT
#undef RNG_INTERRUPT_POLARITY
#undef RNG_INTERRUPT_PROVIDER
#undef RNG_INTERRUPT_PRIORITY
#undef RNG_PRESCALER_CLOCK

#define RNG_BASE_ADDR(n)                (TIP_RNG_BASE_ADDR)
#define RNG_INTERRUPT                   NVIC_INT_47
#define RNG_INTERRUPT_POLARITY          INTERRUPT_POLARITY_RISING_EDGE
#define RNG_INTERRUPT_PROVIDER          INTERRUPT_PROVIDER_NVIC
#define RNG_INTERRUPT_PRIORITY          0
#define RNG_PRESCALER_CLOCK             CLK_GetAPBFreq(CLK_TIP_APB)   /* to acheive the highest freq */


/*---------------------------------------------------------------------------------------------------------*/
/* Fuse Module                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#undef  FUSE_BASE_ADDR

#define FUSE_BASE_ADDR                  (TIP_FUSE_BASE_ADDR)

/*---------------------------------------------------------------------------------------------------------*/
/* Flash Interface Unit (FIU) Module                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#undef  FIU_BASE_ADDR
#define FIU_BASE_ADDR(n)                ( ((n) == FIU_MODULE_0 ) ? TIP_SPI0_REGS_BASE_ADDR :    \
                                         (((n) == FIU_MODULE_1 ) ? TIP_SPI1_REGS_BASE_ADDR :    \
                                         (((n) == FIU_MODULE_2 ) ? TIP_SPI3_REGS_BASE_ADDR :    \
                                         (((n) == FIU_MODULE_3 ) ? TIP_SPI3_REGS_BASE_ADDR :    \
                                         TIP_SPIX_REGS_BASE_ADDR))))

/*---------------------------------------------------------------------------------------------------------*/
/* TIP GPIO Interrupts                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define TIP_GPIO_INTERRUPT              NVIC_INT_18
#define TIP_GPIO_INTERRUPT_PROVIDER     INTERRUPT_PROVIDER_NVIC

/*---------------------------------------------------------------------------------------------------------*/
/* Memory Mapped IO                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define IOMEMORY_BASE_ADDR              0xF0000000

#define TIP_CHECK_MAP(addr, bmc_base, tip_base, size)   ((((addr) >= (bmc_base)) && ((addr) < ((bmc_base) + (size)))) ? \
                                                         ((addr) - (bmc_base) + (tip_base)) :
#define TIP_MAPPED_ADDRESS(addr) ( \
           TIP_CHECK_MAP(addr, TIP_PERIPH_WIN0_BASE_ADDR, 0xF0000000, _128KB_) \
           TIP_CHECK_MAP(addr, TIP_PERIPH_WIN1_BASE_ADDR, 0xF0080000, _128KB_) \
           TIP_CHECK_MAP(addr, TIP_PERIPH_WIN2_BASE_ADDR, 0xF0100000, _128KB_) \
           TIP_CHECK_MAP(addr, TIP_PERIPH_WIN3_BASE_ADDR, 0xF0180000, _128KB_) \
           TIP_CHECK_MAP(addr, TIP_PERIPH_WIN4_BASE_ADDR, 0xF0200000, _128KB_) \
           TIP_CHECK_MAP(addr, TIP_PERIPH_WIN5_BASE_ADDR, 0xF0800000, _128KB_) \
           TIP_CHECK_MAP(addr, TIP_PERIPH_WIN6_BASE_ADDR, 0x00000000,    0   ) \
           TIP_CHECK_MAP(addr, TIP_PERIPH_WIN7_BASE_ADDR, 0xFFFF0000,  _64KB_) \
           NULL)

#endif //__NPCM850_TIP_H_

