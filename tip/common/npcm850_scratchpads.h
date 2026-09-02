
#ifndef __NPCM850_TIP_SCRATCHPAD_H__
#define __NPCM850_TIP_SCRATCHPAD_H__
// TIP Scratch Pads
#define TIP_SEARCH_HDR_START_SCR     TIP_SCR0
#define TIP_RESET_INDICATION_SCR     TIP_SCR1
#define TIP_INVALIDATE_OTP_KEYS_SCR  TIP_SCR2
#define TIP_RESET_COUNTERS_SCR       TIP_SCR3
#define TIP_PLL0_ACTUAL_SCR          TIP_SCR6
#define TIP_L1_SYS_CTRL_FLAG_SCR     TIP_SCR7

// BMC Scratch Pads (examples)
#define BMC_WAKE_CORE0_ADDR_LOW_SCR      SCRPAD_10_41 (2)
#define BMC_WAKE_CORE0_ADDR_HIGH_SCR     SCRPAD_10_41 (3)
#define BMC_WAKE_CORE1_ADDR_LOW_SCR      SCRPAD_10_41 (4)
#define BMC_WAKE_CORE1_ADDR_HIGH_SCR     SCRPAD_10_41 (5)
#define BMC_WAKE_CORE2_ADDR_LOW_SCR      SCRPAD_10_41 (6)
#define BMC_WAKE_CORE2_ADDR_HIGH_SCR     SCRPAD_10_41 (7)
#define BMC_WAKE_CORE3_ADDR_LOW_SCR      SCRPAD_10_41 (8)
#define BMC_WAKE_CORE3_ADDR_HIGH_SCR     SCRPAD_10_41 (9)


#define FLASH_STATUS_COMMAND_SCR         SCRPAD_10_41 (10)  // Status (pass = 0)
#define FLASH_PRM1_SCR                   SCRPAD_10_41 (11)
#define FLASH_PRM2_SCR                   SCRPAD_10_41 (12)
#define FLASH_PRM3_SCR                   SCRPAD_10_41 (13)
#define FLASH_PRM4_SCR                   SCRPAD_10_41 (14)
#define FLASH_PRM5_SCR                   SCRPAD_10_41 (15)
#define FLASH_PRM6_SCR                   SCRPAD_10_41 (16)

#define BMC_LOG_START_ADDR_SCR           SCRPAD_10_41 (17)
#define BMC_LOG_SIZE_SCR                 SCRPAD_10_41 (18)
#define BMC_LOG_NEXT_ADDR_SCR            SCRPAD_10_41 (19)

#define FLASH_XFER_TIP_IN_PROGRESS_SCR   SCRPAD_10_41 (20)

#define RNG_COMMAND_SCR                  SCRPAD_10_41 (21)  // Command (0x06), reused for status
#define RNG_SIZE_SCR                     SCRPAD_10_41 (22)  // Size of buffer to fill
#define RNG_BUFFER_ADDR_SCR              SCRPAD_10_41 (23)  // Address of buffer to fill with DRBG bytes

#define AES_BLOCK_ADDR_SCR               SCRPAD_10_41 (24)
#define AES_BLOCK_SIZE_SCR               SCRPAD_10_41 (25)
#define AES_COMMAND_SCR                  SCRPAD_10_41 (26)  // 0 = encrypt, 1 = decrypt
#define AES_OUTPUT_ADDR_SCR              SCRPAD_10_41 (27)
#define AES_IV_INFO_SCR                  SCRPAD_10_41 (28)  // IV size and data
#define AES_KEY_INFO_SCR                 SCRPAD_10_41 (29)  // Key size and data
#define AES_TAG_INFO_SCR                 SCRPAD_10_41 (30)  // TAG size and data

#endif // __NPCM850_TIP_SCRATCHPAD_H__

