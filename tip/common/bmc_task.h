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


/**
 * Task context for processing bmc messages.
 */
struct bmc_task {
	TaskHandle_t bmc_task; /**< Task handler for bmc reset processing loop. */
};

int bmc_task_init (struct bmc_task *task, int priority, uint16_t stack_words);
void bmc_start (uint32_t addr, bool wait_for_finish);
void bmc_continue (void);
int tip_load_bmc_firmware (struct spi_flash *flash, uint32_t start_offset,
	struct hash_engine *hash);
uint8_t *bmc_component_get_digest_buf (IMG_TYPE_E img);
void bmc_export_data (void);


#endif /* _BMC_TASK_H_ */
