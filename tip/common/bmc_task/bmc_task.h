/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#ifndef _BMC_TASK_H_
#define _BMC_TASK_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "system/system.h"
#include "task.h"
#include "tip_firmware_component.h"
#ifdef BMC_DIRECT_COMPOSITE_EAT
#include "composite_eat/composite_eat_generator.h"
#endif

#define BMC_RESET_CMD    0x00
#ifdef BMC_DIRECT
#define BMC_DIRECT_NOTIFCATION_FL   0x02
#define BMC_DIRECT_NOTIFCATION_AES  0x04
#define BMC_DIRECT_NOTIFCATION_RNG  0x08

/**
* Notification target bit.
*/
enum bmc_commands_e {
	BMC_RESET_0 = 0,
	BMC_DIRECT_COMMAND_FL_PROG = 0x01,          /**< flash program */
	BMC_DIRECT_COMMAND_FL_READ = 0x02,          /**< flash read */
	BMC_DIRECT_COMMAND_FW_UPDATE = 0x03,        /**< FW update: combo0, combo1 or both */
	BMC_DIRECT_COMMAND_FL_ERASE_SECTOR = 0x04,  /**< flash erase */
	BMC_DIRECT_COMMAND_FL_READ_PARMAS = 0x05,   /**< export flash parameters */
	BMC_DIRECT_COMMAND_DRBG = 0x06,             /**< get rng bytes from NCL to BMC */
	BMC_DIRECT_COMMAND_AES_ENC_ECB = 0x10,             /**< AES Encrypt ECB*/
	BMC_DIRECT_COMMAND_AES_ENC_CBC = 0x20,             /**< AES Encrypt CBC*/
	BMC_DIRECT_COMMAND_AES_ENC_CTR = 0x30,             /**< AES Encrypt CTR*/
	BMC_DIRECT_COMMAND_AES_ENC_GCM = 0x40,             /**< AES Encrypt GCM*/
	BMC_DIRECT_COMMAND_AES_DEC_ECB = 0x11,             /**< AES decrypt ECB*/
	BMC_DIRECT_COMMAND_AES_DEC_CBC = 0x21,             /**< AES decrypt CBC*/
	BMC_DIRECT_COMMAND_AES_DEC_CTR = 0x31,             /**< AES decrypt CTR*/
	BMC_DIRECT_COMMAND_AES_DEC_GCM = 0x41,             /**< AES decrypt GCM*/

	/* note: plan to add more commands , for example: */
	BMC_DIRECT_COMMAND_ENABLE_FILTER_5,
	BMC_DIRECT_COMMAND_DISABLE_FILTER_6,
	BMC_DIRECT_COMMAND_BIOS_PROG_7,
	BMC_DIRECT_MANIFEST_UPDATE_8,
	BMC_DIRERT_MANIFEST_ACTIVATE_9,

	BMC_MAX_COMMANDS
};
#endif /* BMC_DIRECT */

enum bmc_state {
	BMC_DISABLE = 0,
	BMC_BOOTBLOCK_RUNNING = 1,
	BMC_BOOTBLOCK_RESET = 2,
	BMC_DDR_READY = 3,
	BMC_RESET = 4
};

#define BMC_BOOTBLOCK_TRAINING_PASS 0x01
#define BMC_BOOTBLOCK_TRAINING_FAIL 0x02

/**
 * TIP init WD before bmc_continue. 0x55 is exquivalent to ~120 seconds.
 */
#define BMC_WD_TIME_UBOOT                             0x55

/**
 * TIP init WD before bmc_start. 0x55 is exquivalent to ~5.5 minutes.
 */
#define BMC_WD_TIME_BOOTBLOCK                         0xFF

/**
 * TIP init WD before bmc_continue index (0,1,2 possible values)
 */
#define BMC_WD_NUM                                     1

/**
 * Task context for processing bmc messages.
 */
struct bmc_task {
	TaskHandle_t bmc_task;			/**< Task handler for bmc reset processing loop. */
	enum bmc_state bmc_state;		/**< Current state of BMC */
	struct system *system;			/**< The system manager instance. */
};

#if configSUPPORT_DYNAMIC_ALLOCATION == 1
int bmc_task_init (struct bmc_task *task, int priority, uint16_t stack_words,
	struct system *system);
#endif

#if configSUPPORT_STATIC_ALLOCATION == 1
int bmc_task_init_static (struct bmc_task *task, StaticTask_t *context, StackType_t *stack,
	uint16_t stack_words, int priority, struct system *system);
#endif

void bmc_start (struct bmc_task *task, uint32_t addr, bool wait_for_finish);
void bmc_continue (void);
int tip_load_bmc_firmware (struct bmc_task *task, struct spi_flash *flash,
	uint32_t start_offset, struct hash_engine *hash, uint16_t reset_source,
	uint32_t *combo1_start_addr);
uint8_t *bmc_component_get_digest_buf (IMG_TYPE_E img);
void bmc_export_data (void);

#ifdef BMC_DIRECT_COMPOSITE_EAT
void bmc_direct_composite_eat_configure (struct composite_eat_generator *generator);
#endif


#endif /* _BMC_TASK_H_ */
