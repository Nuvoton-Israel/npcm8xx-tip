// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#ifndef _BMC_TASK_H_
#define _BMC_TASK_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "tip_firmware_component.h"
#include "FreeRTOS.h"
#include "task.h"

#define BMC_RESET_CMD    0x00
#ifdef BMC_DIRECT
#define BMC_DIRECT_NOTIFCATION 0x02

/**
* Notification target bit.
*/
enum bmc_commands_e {
	BMC_RESET_0 = 0,
	BMC_DIRECT_COMMAND_FL_PROG = 0x01,          /**< flash program */
	BMC_DIRECT_COMMAND_FL_READ = 0x02,          /**< flash read */
	BMC_DIRECT_COMMAND_FW_UPDATE = 0x03,        /**< FW update: combo0, combo1 or both */
	BMC_DIRECT_COMMAND_FL_ERASE_SECTOR = 0x04,  /**< flash erase */

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
 * Task context for processing bmc messages.
 */
struct bmc_task {
	TaskHandle_t bmc_task; /**< Task handler for bmc reset processing loop. */
	enum bmc_state bmc_state; /**< Current state of BMC */
};

int bmc_task_init (struct bmc_task *task, int priority, uint16_t stack_words);
void bmc_start (struct bmc_task *task, uint32_t addr, bool wait_for_finish);
void bmc_continue (void);
int tip_load_bmc_firmware (struct bmc_task *task, struct spi_flash *flash, 
	uint32_t start_offset, struct hash_engine *hash, uint16_t reset_source);
uint8_t *bmc_component_get_digest_buf (IMG_TYPE_E img);
void bmc_export_data (void);


#endif /* _BMC_TASK_H_ */
