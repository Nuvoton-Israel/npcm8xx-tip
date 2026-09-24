/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#ifdef CERBERUS_ENABLE_COMPONENT_ATTESTATION
#include "attestation/attestation_logging.h"
#include "attestation/attestation_requester.h"
#endif
#include "flash/spi_flash.h"
#include "flash/flash_xfer_tip.h"
#include "tip_boot.h"
#include "tip_utils.h"
#include "tip_flash.h"
#include "platform_io.h"
#include "crypto/hash_thread_safe.h"
#include "flash/spi_flash.h"
#include "bmc_task.h"
#include "platform_api.h"
#include "platform_io.h"
#include "rot_memory_map.h"
#include "tip_boot.h"
#include "tip_firmware_component.h"
#include "crypto/tip_ecc_hw_ncl.h"
#include "tip_flash.h"
#include "tip_fw_update_task.h"
#include "tip_reset.h"
#include "tip_mbx.h"
#include "tip_rom_utils.h"
#include "tip_utils.h"
#include "tip_reset.h"
#include "tip_hash_ncl.h"
#ifdef BMC_DIRECT
#include "tip_rng_ncl.h"
#include "tip_aes_ncl.h"
#endif
#ifdef BMC_DIRECT_COMPOSITE_EAT
#include "composite_eat/bmc_direct/bmc_direct_composite_eat_abi.h"
#include "composite_eat/bmc_direct/bmc_direct_composite_eat_status.h"
#include "composite_eat/bmc_direct/bmc_direct_composite_eat_transport.h"
#include "composite_eat/tip_main_token_generator.h"
#endif

/**
 * Buffer to store Bootblock hash.
 */
static uint8_t bb_hash[SHA512_HASH_LENGTH];

/**
 * Buffer to store BL31 hash.
 */
static uint8_t bl31_hash[SHA512_HASH_LENGTH];

/**
 * Buffer to store Optee hash.
 */
static uint8_t optee_hash[SHA512_HASH_LENGTH];

/**
 * Buffer to store Uboot hash.
 */
static uint8_t uboot_hash[SHA512_HASH_LENGTH];

/**
 * Wrapper for the shared hash engine.
 */
extern struct hash_engine_thread_safe shared_hash;

/**
 * Hardware RNG that will be shared between multiple components.
 */
extern struct tip_rng_ncl_engine system_rng;

/**
 * Main flash handler. BMC reloading only occurs from main flash.
 */
extern struct spi_flash *main_flash;

/**
 * Secondary key manifest used during boot-flow.
 */
extern struct tip_secondary_key_manifest tip_skmt;

/**
 * Task for handling bmc reset.
 */
extern struct bmc_task bmc_reset_task;

/**
 * start address of BMC images.
 */
static uint32_t bmc_start_addr[4];

/**
 * Offset of recovery image location.
 */
extern uint32_t recovery_flash_start_offset;

/**
 * Flag indicating the system booted from the recovery flash.
 */
extern bool recovery_boot;

/**
 * Handler for the active flash.
 *
 * Active flash is where the running fw image is loaded from.
 * It can be either main flash or recovery flash.
 */
extern struct spi_flash *active_flash;

/**
 * Reset counter
 */
static uint8_t reset_counter [RST_MAX];

/**
 * handler to regular mailbox
 */
extern struct tip_cmd_channel system_i2c;

/**
 * The task for executing Cerberus firmware update actions.
 */
extern struct tip_fw_update_task cerberus_update;

/**
 * Hash engine that will be shared between multiple components.
 */
extern struct tip_hash_ncl_engine system_hash;

extern struct tip_rom_ncl_shared_state *ncl_shared_state;

#ifdef BMC_DIRECT_COMPOSITE_EAT
static struct bmc_direct_composite_eat_state composite_eat_state;
static struct composite_eat_tip_main_token_generator *composite_eat_main_token_generator;

_Static_assert (COMPOSITE_EAT_TIP_REQUEST_SNAPSHOT_MAX ==
	BMC_DIRECT_COMPOSITE_EAT_REQ_SIZE, "Composite EAT request snapshot must match the ABI");
_Static_assert (COMPOSITE_EAT_TIP_MAX_RESPONSE_LENGTH ==
	BMC_DIRECT_COMPOSITE_EAT_RESP_SIZE, "Composite EAT response limit must match the ABI");
#endif

/**
 * Hardware ECC engine.
 */
extern struct tip_ecc_hw_ncl_engine hw_ecc;

#ifdef CERBERUS_ENABLE_COMPONENT_ATTESTATION
/**
 * Flag indicating if the system was initialized with an active PCD.
 */
extern bool has_active_pcd;

/**
 * Flag indicating if there is an active PCD with at least one component configured.
 */
extern bool pcd_has_components;

/**
 * Maximum duration to wait for MCTP bridge to assign Cerberus an EID before attempting to get
 * routing table.
 */
extern uint32_t get_routing_table_timeout_ms;

/**
 * The system attestation requester instance
 */
extern struct attestation_requester system_attestation_requester;

/**
 * Timer for delaying routing table sync request to BMC.
 */
extern platform_timer routing_table_rq_timer;
#endif

/**
 * TIP L1 header handler for flag SystemControlFlags (control in IGPS, TipFwAndHeader_L1.xml)
 */
extern struct tip_L1_system_control tip_L1_sys_ctrl;

/**
 * Disable all BMC watchdogs
 */
static void tip_bmc_tim_disable_all_wd (void)
{
	TMC_StopWatchDog (0);
	TMC_StopWatchDog (1);
	TMC_StopWatchDog (2);
}

/**
 * TIM (A.K.A. TMC in SWC_HAL) start.
 *
 * @param wd WD number
 * @param time Value for WTCR.WDT_CNT
 */
static void tip_bmc_tim_start_wd (uint32_t wd, uint32_t time)
{
	TMC_StartWatchDog (wd, time);
}

static uint16_t int_num;

/**
 * NVIC Reset BMC interrupt
 *
 * @param num Interrupt number
 */
void NVIC_BMC_reset (uint16_t num)
{
	BaseType_t reset_priority = pdTRUE;

	int_num = SCS_GetActiveVectorNumber () - NVIC_TRAP_NUM;

	tip_bmc_tim_disable_all_wd ();

#ifdef BMC_DIRECT_COMPOSITE_EAT
	bmc_direct_composite_eat_reset (&composite_eat_state);
	NVIC_EnableInt (NVIC_INT_2, FALSE);
#endif

	/* interrupt will be reenabled after BMC is reloaded in bmc_task */
	NVIC_EnableInt (NVIC_INT_46, FALSE);

#ifndef BMC_DIRECT
	/* Clear the scratchpad register on BMC reset.  This is to ensure TIP can successfully load
	 * BMC after reset if BMC got reset before clearing the shared scratchpad register
	 * (e.g. WD timeout) or did not clear sratchpad register after successful boot.
	 * Since we are running in an interrupt context, clearing the register directly will not impact
	 * flash access synchronization within TIP tasks. */
	REG_WRITE (FLASH_XFER_TIP_IN_PROGRESS_SCR, 0);
#endif

	xTaskNotifyFromISR (bmc_reset_task.bmc_task, BMC_RESET_CMD, eSetValueWithOverwrite,
		&reset_priority);
}

#ifdef BMC_DIRECT
static void tip_bmc_direct_finalize_command (int status, UINT32 notification)
{
	if (ROT_IS_ERROR(status) == false) {
		status = 0;
	}
	platform_printf_dbg ("Done status=%#010lx" NEWLINE, status);
	/* return status to BMC and the command register */
	if (notification == BMC_DIRECT_NOTIFCATION_FL) {
		REG_WRITE (FLASH_STATUS_COMMAND_SCR, status);
	}
	else if (notification == BMC_DIRECT_NOTIFCATION_RNG) {
		REG_WRITE (RNG_COMMAND_SCR, status);
	}
	else if (notification == BMC_DIRECT_NOTIFCATION_AES) {
		REG_WRITE (AES_COMMAND_SCR, status);
	}

	tip_mbx_clear_notification (notification);

	/* Clear BMC notification event */
	NVIC_ClearInt (NVIC_INT_2);

	/* Reenable the IRQ after complition */
	NVIC_EnableInt (NVIC_INT_2, true);

	/* notify BMC on complition */
	tip_mbx_notify_to_bmc (notification);
}
#endif

#ifdef BMC_DIRECT_COMPOSITE_EAT
void bmc_direct_composite_eat_configure (
	struct composite_eat_tip_main_token_generator *generator)
{
	composite_eat_main_token_generator = generator;
	bmc_direct_composite_eat_reset (&composite_eat_state);
	tip_mbx_clear_notification (BMC_DIRECT_NOTIFICATION_COMPOSITE_EAT);
	NVIC_ClearInt (NVIC_INT_2);
	NVIC_EnableInt (NVIC_INT_2, TRUE);
}

static void bmc_direct_composite_eat_write_scratchpad (void *context, uint32_t index,
	uint32_t value)
{
	(void) context;
	REG_WRITE (SCRPAD_10_41 (index), value);
}

static void bmc_direct_composite_eat_memory_barrier (void *context)
{
	(void) context;
	__asm volatile("dmb" ::: "memory");
}

static void bmc_direct_composite_eat_clear_notification (void *context,
	uint32_t notification)
{
	(void) context;
	tip_mbx_clear_notification (notification);
}

static void bmc_direct_composite_eat_notify_bmc (void *context, uint32_t notification)
{
	(void) context;
	tip_mbx_notify_to_bmc (notification);
}

static const struct bmc_direct_composite_eat_publication_ops composite_eat_publication_ops = {
	.write_scratchpad = bmc_direct_composite_eat_write_scratchpad,
	.memory_barrier = bmc_direct_composite_eat_memory_barrier,
	.clear_notification = bmc_direct_composite_eat_clear_notification,
	.notify_bmc = bmc_direct_composite_eat_notify_bmc,
};

static void bmc_direct_composite_eat_publish (enum bmc_direct_composite_eat_status status,
	uint32_t response_length, uint32_t request_id)
{
	(void) bmc_direct_composite_eat_publish_response (&composite_eat_publication_ops, NULL,
		status, response_length, request_id);
}

static bool bmc_direct_composite_eat_finish (enum bmc_direct_composite_eat_status status,
	uint32_t response_length, const struct bmc_direct_composite_eat_request *request)
{
	DISABLE_INTERRUPTS ();
	if (!bmc_direct_composite_eat_request_current (&composite_eat_state, request)) {
		ENABLE_INTERRUPTS ();
		return false;
	}

	bmc_direct_composite_eat_publish (status, response_length, request->id);
	bmc_direct_composite_eat_complete (&composite_eat_state);
	NVIC_ClearInt (NVIC_INT_2);
	NVIC_EnableInt (NVIC_INT_2, TRUE);
	ENABLE_INTERRUPTS ();
	return true;
}

struct bmc_direct_composite_eat_output {
	const struct bmc_direct_composite_eat_request *request;
	size_t offset;
};

static void bmc_direct_composite_eat_output_lock (void *context)
{
	(void) context;
	DISABLE_INTERRUPTS ();
}

static void bmc_direct_composite_eat_output_unlock (void *context)
{
	(void) context;
	ENABLE_INTERRUPTS ();
}

static int bmc_direct_composite_eat_output_write (void *context, uint32_t address,
	const uint8_t *data, size_t length)
{
	(void) context;
	memcpy ((void *) (uintptr_t) address, data, length);
	return 0;
}

static const struct bmc_direct_composite_eat_output_ops composite_eat_output_ops = {
	.lock = bmc_direct_composite_eat_output_lock,
	.unlock = bmc_direct_composite_eat_output_unlock,
	.write = bmc_direct_composite_eat_output_write,
};

static int bmc_direct_composite_eat_write_output (void *context, const uint8_t *data,
	size_t length)
{
	struct bmc_direct_composite_eat_output *output = context;

	return bmc_direct_composite_eat_write_response (&composite_eat_state, output->request,
		&composite_eat_output_ops, NULL, &output->offset, data, length) ? 0 : -1;
}
#endif

#if defined(BMC_DIRECT) || defined(BMC_DIRECT_COMPOSITE_EAT)

void NVIC_BMC_direct_handler (uint16_t num)
{
	uint32_t notification;
	uint32_t notification_active;
	BaseType_t reset_priority = pdTRUE;
	int_num = SCS_GetActiveVectorNumber () - NVIC_TRAP_NUM;
	uint32_t cmd_reg_ind = 0;

	if (bmc_reset_task.bmc_state != BMC_DDR_READY) {
		NVIC_ClearInt (int_num);
		return;
	}

	/* interrupt will be reenabled after BMC is reloaded in bmc_task */
	NVIC_EnableInt (int_num, FALSE);
	notification = tip_mbx_get_notification ();

	serial_printf_reconfig ();

	platform_printf_dbg (KGRN NEWLINE
		"======== TIP_FW: detected BMC int %d notification %#010lx cmd %#010lx" NEWLINE KNRM,
		int_num, notification, REG_READ (FLASH_STATUS_COMMAND_SCR));

#ifdef BMC_DIRECT_COMPOSITE_EAT
	if ((notification & BMC_DIRECT_NOTIFICATION_COMPOSITE_EAT) != 0) {
		struct bmc_direct_composite_eat_request request = {0};
		const uint8_t *request_snapshot;
		enum composite_eat_tip_main_token_status snapshot_status;
		uint32_t command =
			REG_READ (SCRPAD_10_41 (BMC_DIRECT_COMPOSITE_EAT_SCRPAD_COMMAND));

		request.id = REG_READ (SCRPAD_10_41 (BMC_DIRECT_COMPOSITE_EAT_SCRPAD_REQUEST_ID));
		request.request_address =
			REG_READ (SCRPAD_10_41 (BMC_DIRECT_COMPOSITE_EAT_SCRPAD_REQ_ADDR));
		request.request_length =
			REG_READ (SCRPAD_10_41 (BMC_DIRECT_COMPOSITE_EAT_SCRPAD_REQ_LEN));
		request.response_address =
			REG_READ (SCRPAD_10_41 (BMC_DIRECT_COMPOSITE_EAT_SCRPAD_RESP_ADDR));
		request.response_capacity =
			REG_READ (SCRPAD_10_41 (BMC_DIRECT_COMPOSITE_EAT_SCRPAD_RESP_CAP));
		REG_WRITE (SCRPAD_10_41 (BMC_DIRECT_COMPOSITE_EAT_SCRPAD_COMMAND), 0xffffffffu);

		if (command != BMC_DIRECT_COMMAND_COMPOSITE_EAT) {
			bmc_direct_composite_eat_publish (BMC_DIRECT_COMPOSITE_EAT_UNSUPPORTED, 0,
				request.id);
			goto composite_eat_rearm;
		}
		if (request.request_length > BMC_DIRECT_COMPOSITE_EAT_REQ_SIZE) {
			bmc_direct_composite_eat_publish (BMC_DIRECT_COMPOSITE_EAT_REQUEST_TOO_LARGE,
				0, request.id);
			goto composite_eat_rearm;
		}
		if (!bmc_direct_composite_eat_buffers_valid (request.request_address,
			request.request_length, request.response_address, request.response_capacity)) {
			bmc_direct_composite_eat_publish (BMC_DIRECT_COMPOSITE_EAT_ADDRESS_INVALID, 0,
				request.id);
			goto composite_eat_rearm;
		}
		if (!bmc_direct_composite_eat_begin (&composite_eat_state, &request)) {
			bmc_direct_composite_eat_publish (BMC_DIRECT_COMPOSITE_EAT_BUSY, 0, request.id);
			goto composite_eat_rearm;
		}
		request = composite_eat_state.pending;
		if (composite_eat_main_token_generator == NULL) {
			(void) bmc_direct_composite_eat_finish (BMC_DIRECT_COMPOSITE_EAT_INTERNAL, 0,
				&request);
			return;
		}
		snapshot_status = composite_eat_tip_main_token_snapshot_request (
			composite_eat_main_token_generator,
			(const uint8_t *) (uintptr_t) request.request_address, request.request_length,
			&request_snapshot);
		if (snapshot_status != COMPOSITE_EAT_TIP_MAIN_TOKEN_OK) {
			(void) bmc_direct_composite_eat_finish (BMC_DIRECT_COMPOSITE_EAT_INTERNAL, 0,
				&request);
			return;
		}
		composite_eat_state.pending.request_address = (uint32_t) (uintptr_t) request_snapshot;
		request = composite_eat_state.pending;
		if (!bmc_direct_composite_eat_request_current (&composite_eat_state, &request)) {
			return;
		}
		if (xTaskNotifyFromISR (bmc_reset_task.bmc_task, BMC_DIRECT_COMPOSITE_EAT_TASK_EVENT,
			eSetValueWithoutOverwrite, &reset_priority) != pdPASS) {
			(void) bmc_direct_composite_eat_finish (BMC_DIRECT_COMPOSITE_EAT_BUSY, 0,
				&request);
			return;
		}
		tip_mbx_clear_notification (BMC_DIRECT_NOTIFICATION_COMPOSITE_EAT);
		__asm volatile("dmb" ::: "memory");
		NVIC_ClearInt (int_num);
		NVIC_EnableInt (int_num, TRUE);
		return;

	composite_eat_rearm:
		NVIC_ClearInt (int_num);
		NVIC_EnableInt (int_num, TRUE);
		return;
	}
#endif

#ifdef BMC_DIRECT
#ifdef BMC_DIRECT_COMPOSITE_EAT
	if (composite_eat_state.active) {
		if ((notification & BMC_DIRECT_NOTIFCATION_FL) != 0) {
			tip_bmc_direct_finalize_command (CMD_CHANNEL_INVALID_PKT_STATE,
				BMC_DIRECT_NOTIFCATION_FL);
			return;
		}
		if ((notification & BMC_DIRECT_NOTIFCATION_RNG) != 0) {
			tip_bmc_direct_finalize_command (CMD_CHANNEL_INVALID_PKT_STATE,
				BMC_DIRECT_NOTIFCATION_RNG);
			return;
		}
		if ((notification & BMC_DIRECT_NOTIFCATION_AES) != 0) {
			tip_bmc_direct_finalize_command (CMD_CHANNEL_INVALID_PKT_STATE,
				BMC_DIRECT_NOTIFCATION_AES);
			return;
		}
	}
#endif
	/* check if it's a BMC_DIRECT Flash command */
	if ((notification & BMC_DIRECT_NOTIFCATION_FL) > 0) {
		notification_active = BMC_DIRECT_NOTIFCATION_FL;
		cmd_reg_ind = 10;
	}

	/* check if it's a BMC_DIRECT RNG command */
	else if ((notification & BMC_DIRECT_NOTIFCATION_RNG) > 0) {
		notification_active = BMC_DIRECT_NOTIFCATION_RNG;
		cmd_reg_ind = 21;
	}

	/* check if it's a BMC_DIRECT AES command */
	else if ((notification & BMC_DIRECT_NOTIFCATION_AES) > 0) {
		notification_active = BMC_DIRECT_NOTIFCATION_AES;
		cmd_reg_ind = 26;
	}

	else if (notification & system_i2c.notification_idx) {
		/* cerberus_utility interrupt. uses polling only. ignore and clear */
		tip_mbx_clear_notification (system_i2c.notification_idx);
		return;
	}

	else {
		platform_printf (KRED "BMC notification reg is %d unknown" NEWLINE KNRM, notification);
		tip_mbx_clear_notification (notification);
	}

	if (cmd_reg_ind != 0) {
		uint32_t cmd = REG_READ (SCRPAD_10_41 (cmd_reg_ind));
		platform_printf (KMAG "cmd %x cmd_ind %d" NEWLINE KNRM, cmd, cmd_reg_ind);

		/* prevent reuse of command. FFFFFFF means command is being executed. */
		REG_WRITE (SCRPAD_10_41 (cmd_reg_ind), 0xFFFFFFFF);
		if ((cmd > BMC_RESET_0) && (cmd < BMC_MAX_COMMANDS)) {
			xTaskNotifyFromISR (bmc_reset_task.bmc_task, cmd, eSetValueWithOverwrite,
				&reset_priority);
			return;
		}
		else {
			platform_printf (KRED "BMC direct command %#010lx unknown" NEWLINE KNRM, cmd);
			tip_bmc_direct_finalize_command (CMD_CHANNEL_INVALID_ARGUMENT, notification_active);
		}
	}

	/* check if it's a BMC_DIRECT RNG command */
	else if ((notification & BMC_DIRECT_NOTIFCATION_RNG) > 0) {
		uint32_t cmd = REG_READ (RNG_COMMAND_SCR);
		/* prevent reuse of command. FFFFFFF means command is being executed. */
		REG_WRITE (RNG_COMMAND_SCR, 0xFFFFFFFF);
		if ((cmd > BMC_RESET_0) && (cmd < BMC_MAX_COMMANDS)) {
			xTaskNotifyFromISR (bmc_reset_task.bmc_task, cmd, eSetValueWithOverwrite,
				&reset_priority);
			return;
		}
		else {
			platform_printf (KRED "BMC direct command %#010lx unknown" NEWLINE KNRM, cmd);
			tip_bmc_direct_finalize_command (CMD_CHANNEL_INVALID_ARGUMENT, BMC_DIRECT_NOTIFCATION_RNG);
		}
	}
	else if (notification & system_i2c.notification_idx) {
		/* cerberus_utility interrupt. uses polling only. ignore and clear */
		tip_mbx_clear_notification (notification);
		return;
	}
	else {
		platform_printf (KRED "BMC notification reg is %d unknown" NEWLINE KNRM, notification);
		tip_mbx_clear_notification (notification);
	}

	/* Clear BMC notification event */
	NVIC_ClearInt (int_num);
	NVIC_EnableInt (int_num, TRUE);
#else
	tip_mbx_clear_notification (notification);
	NVIC_ClearInt (int_num);
	NVIC_EnableInt (int_num, TRUE);
#endif
}
#endif /* BMC_DIRECT || BMC_DIRECT_COMPOSITE_EAT */


/**
 *  Release BMC from reset and wait for it to complete DDR init.
 *
 *  @param [in] addr  Addr of FW, including header.
 *  @param [in] wait_for_finish  True if waiting for BB to finish execution.
 *
 */
static void bmc_release_from_reset (struct bmc_task *task, uint32_t addr, bool wait_for_finish)
{
	uint32_t tip_bmc_rctl, a35_mode;

	a35_mode = (BUILD_FIELD_VAL (A35_MODE_AA64B_AA32, 1) | BUILD_FIELD_VAL (A35_MODE_AA32BAS, 1) |
		BUILD_FIELD_VAL (A35_MODE_AA64BA, (addr + sizeof (HEADER_GENERAL_T)) >> 8));

	task->bmc_state = BMC_DISABLE;

	platform_printf (KGRN "%s: addr %#010lx" NEWLINE KNRM, __func__,
		addr + sizeof (HEADER_GENERAL_T));

	/* clear notification status registers */
	REG_WRITE (B2TIPST0, 0xFFFF);
	REG_WRITE (B2TIPST1, 0xFFFF);

	/* At this stage, the A35 is released and starts running.
	 * Set the TIP to be reset on any BMC reset,
	 *  then the TIP should be stuck in endless loop
	 */
	platform_printf ("Open JTAG A35" NEWLINE);
	SET_REG_FIELD (TIP_DBG_CTL, TIP_DBG_CTL_BMC_JEN, TIP_SEC_TRUE);

	platform_printf ("Release A35, TIP_CTL_STS = %#010lx A35_MODE = %#010lx\n" NEWLINE,
		REG_READ (TIP_CTL_STS), a35_mode);
	REG_WRITE (A35_MODE, a35_mode);

	DISABLE_INTERRUPTS ();
	/* clear scratchpads before use */
	for (int i = 0; i < 31; i++)
		REG_WRITE (SCRPAD_10_41 (i), 0);

	REG_WRITE (B2TIPST0, 0xFFFF);
	REG_WRITE (B2TIPST1, 0xFFFF);

	tip_bmc_rctl = 0;

	/* Set TIP_BMC_RCTL_A35_ASYNC_RST to R/W */
	SET_VAR_FIELD (tip_bmc_rctl, TIP_BMC_RCTL_RCTL_QUAL, 0xA5);
	SET_VAR_FIELD (tip_bmc_rctl, TIP_BMC_RCTL_A35_ASYNC_RST, 1);
	SET_VAR_FIELD (tip_bmc_rctl, TIP_BMC_RCTL_A35_AFTR_CRST, 0);

	platform_printf ("TIP_BMC_RCTL => %#010lx " NEWLINE, tip_bmc_rctl);
	REG_WRITE (TIP_BMC_RCTL, tip_bmc_rctl);
	DELAY_USEC (100, 0);
	platform_printf ("TIP_BMC_RCTL = %#010lx " NEWLINE, REG_READ (TIP_BMC_RCTL));

	/* release A35 */
	SET_VAR_FIELD (tip_bmc_rctl, TIP_BMC_RCTL_A35_ASYNC_RST, 0);
	NVIC_ClearAll ();

	platform_printf ("TIP_BMC_RCTL => %#010lx \n\nWait for bootblock.." NEWLINE, tip_bmc_rctl);
	REG_WRITE (TIP_BMC_RCTL, tip_bmc_rctl);
	/* clear status registers of BMC */
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_BMC_CRST_STS, 1);
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_DBGRST_STS, 1);

	NVIC_ClearAll ();
	tip_mbx_clear_notification (0xFFFFFFFF);

	task->bmc_state = BMC_BOOTBLOCK_RUNNING;

	/* enable BMC reset event */
	NVIC_EnableInt ((NVIC_INT_SRC_T) NVIC_INT_46, TRUE);
	ENABLE_INTERRUPTS ();

	/* wait for BootBlock to init the DDR */
	if (wait_for_finish) {
		tip_bmc_tim_disable_all_wd ();
		tip_bmc_tim_start_wd (BMC_WD_NUM, BMC_WD_TIME_BOOTBLOCK);

		/* wait for bootblock notification or bootblock reset */
		while (1) {
			/* Check whether TIP got notification from BMC. */
			if (REG_READ (B2TIPST0) != 0) {
				break;
			}

			if (task->bmc_state == BMC_BOOTBLOCK_RESET) {
				break;
			}
		}

		/* check return status from bootblock */
		if ((REG_READ (B2TIPST0) == BMC_BOOTBLOCK_TRAINING_PASS) &&
			(task->bmc_state == BMC_BOOTBLOCK_RUNNING)) {
			task->bmc_state = BMC_DDR_READY;
		}
		tip_bmc_tim_disable_all_wd ();
		tip_mbx_clear_notification (0xFFFFFFFF);
		serial_printf_reconfig ();
	}

	/* clear status registers of BMC */
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_BMC_CRST_STS, 1);
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_DBGRST_STS, 1);
	NVIC_ClearInt (NVIC_INT_46);
}

/**
 * This function is called only after A35 firmware load.
 * Once FW is ready and verified on A35 RAM, reset A35 and configure it to start
 * executing the pre-loaded fw.
 * Retry bootblock up to 3 times. If that fails, reset and recovery.
 *
 * @param [in] task BMC task handler
 * @param [in] addr  Addr of FW, including header.
 * @param [in] wait_for_finish  True if waiting for BB to finish execution.
 *
 */
void bmc_start (struct bmc_task *task, uint32_t addr, bool wait_for_finish)
{
	int boot_retry = 3;
#ifdef BMC_DIRECT
	NVIC_EnableInt (NVIC_INT_2, FALSE);
#endif
	if (tip_L1_sys_ctrl.tip_disable_jtag) {
		CHIP_DisableJtag ();
	}
	while (boot_retry > 0) {
		if (READ_REG_FIELD (INTCR2, INTCR2_MC_INIT) == 0) {
			CLK_ResetMC ();
		}

		bmc_release_from_reset (task, addr, wait_for_finish);

		if (task->bmc_state == BMC_DDR_READY) {
			boot_retry = 0;
		}
		else {
			boot_retry--;
		}
	}

	/* if DDR is not ready after all retries: restart and recovers system */
	if (task->bmc_state != BMC_DDR_READY) {
		DISABLE_INTERRUPTS ();
		platform_printf (
			KRED "TIP ERROR: bootblock failed 3 times, resetting and try to recover" NEWLINE);
		platform_reset (0);
	}

	return;
}

/**
 * Signal the BMC to branch and run from addr.
 * Used when bootblock is polling at the end of its run.
 *
 *  @param [in] addr
 */
static void bmc_continue_l (uint32_t addr)
{
	UINT64 time64 = CLK_GetUpTimeMiliseconds ();
	platform_printf (KYEL "Uptime %lu ms" NEWLINE KNRM, (uint32_t) time64);

	/* set wake up address for core 0: */
	REG_WRITE (BMC_WAKE_CORE0_ADDR_HIGH_SCR, 0);
	REG_WRITE (BMC_WAKE_CORE0_ADDR_LOW_SCR, addr);

	/* notify BMC */
	SET_REG_BIT (TIP2BNT1, 3);

	/* wake core 1: */
	CLK_Delay_MicroSec (5);
	REG_WRITE (BMC_WAKE_CORE1_ADDR_HIGH_SCR, 0);
	REG_WRITE (BMC_WAKE_CORE1_ADDR_LOW_SCR, addr);

	/* wake core 2: */
	CLK_Delay_MicroSec (5);
	REG_WRITE (BMC_WAKE_CORE2_ADDR_HIGH_SCR, 0);
	REG_WRITE (BMC_WAKE_CORE2_ADDR_LOW_SCR, addr);

	/* wake core 3: */
	CLK_Delay_MicroSec (5);
	REG_WRITE (BMC_WAKE_CORE3_ADDR_HIGH_SCR, 0);
	REG_WRITE (BMC_WAKE_CORE3_ADDR_LOW_SCR, addr);

	/* let bootblock continue, then clear the scrpads */
	CLK_Delay_Cycles (1000);

	for (int i = 2; i < 10; i++)
		REG_WRITE (SCRPAD_10_41 (i), 0);
}

/**
 *  After A35 finishes bootblock execuation, BMC waits for TIP to verify the next images
 *  before jumping to the next iamge (bl31).
 *
 */
void bmc_continue (void)
{
	platform_printf (KCYN NEWLINE "TIP say to A35 to jump to BL31 at addr %#010lx" NEWLINE KNRM,
		bmc_start_addr[1] + sizeof (HEADER_GENERAL_T));

	tip_bmc_tim_disable_all_wd ();

	if (tip_L1_sys_ctrl.tip_start_wd) {
		platform_printf (KCYN "Start BMC WD\n" KNRM);
		tip_bmc_tim_start_wd (BMC_WD_NUM, BMC_WD_TIME_UBOOT);
	}

#ifndef BMC_DIRECT
	/* Mark the flash in use by BMC, so TIP doesn't access flash.
	 * BMC MUST clear this register so TIP can access flash. This is ONLY called when bootblock
	 * is loaded and not after soft bmc reset */
	flash_xfer_tip_set_xfer_in_progress ();
#endif

	/* continue to bl31 */
	bmc_continue_l (bmc_start_addr[1] + sizeof (HEADER_GENERAL_T));

#ifdef BMC_DIRECT
	NVIC_ClearInt (NVIC_INT_2);
	NVIC_EnableInt (NVIC_INT_2, TRUE);
#else
	/* Release the mutex acquired without clearing the scratchpad register.  Once BMC successfully
	 * boots, it will clear the register indicating TIP can initiate flash transfers. */
	flash_xfer_tip_mutex_release ();
#endif
}

/**
 * Load BMC firmware components to memory and verify in order
 *
 * @param flash Flash to load firmware from.
 * @param start_offset  Start offset from the flash
 * @param hash Hash engine to utilize.
 * @output combo1_start_addr indicating start address of combo1 image.
 * @return 0 for success or an error code.
 */
int tip_load_bmc_firmware (struct bmc_task *task, struct spi_flash *flash, uint32_t start_offset,
		struct hash_engine *hash, uint16_t reset_source, uint32_t *combo1_start_addr)
{
	struct tip_firmware_component fw;
	uint32_t img_type;
	int status = FIRMWARE_COMPONENT_BAD_HEADER;
	uint32_t scan_offset = start_offset;
	int img_cnt = 0;

	/* Iterate on the BMC images. Note: image enums are bitwise. */
	for (img_type = IMG_BOOTBLOCK; img_type <= IMG_UBOOT; img_type = img_type << 1) {
		platform_printf (KCYN NEWLINE "==========" NEWLINE "Start %s" NEWLINE
									  "==========" NEWLINE KNRM,
			tip_firmware_component_get_name (img_type));

		status = tip_firmware_component_init (&fw, flash, img_type, scan_offset, true);
		if (status != 0) {
			return status;
		}

		status = tip_firmware_component_load_to_memory (&fw);
		if (status != 0) {
			return status;
		}

		bmc_start_addr[img_cnt++] = (uint32_t) fw.header_flash->header.destAddr;

		status = tip_firmware_component_verify (&fw, hash, NULL, &tip_skmt,
			bmc_component_get_digest_buf (img_type), SHA512_HASH_LENGTH, &hw_ecc.base);

		if (status != 0 && TIP_SECBOOT_IS_ACTIVE ()) {
			/* anti-glitch: re-read secure boot flag */
			if (!TIP_SECBOOT_IS_ACTIVE ()) {
				status = 0;
			} else {
				platform_printf (KRED "Image at %#010lx failed authentication" NEWLINE KNRM,
					(uint32_t) fw.header_ram + sizeof (HEADER_GENERAL_T));

				return status;
			}
		}
		else {
			/* SECURE_BOOT disabled, ignore verification failure.*/
			status = 0;
		}

		/* handle external hardening table from IGPS (CSV file) */
		if (tip_firmware_component_check_external_reg_table (&fw) == true) {
			status = tip_firmware_component_load_external_reg_table (&fw, reset_source);
			if (ROT_IS_ERROR (status)) {
				return status;
			}
		}

		/* if bootblock not found return, otherwise release from reset */
		if (img_type == IMG_BOOTBLOCK) {
			*combo1_start_addr = (int) fw.header_flash;
			platform_printf (KMAG "combo1 found at %#010lx" NEWLINE KNRM, *combo1_start_addr);
			bmc_start (task, bmc_start_addr[0], true);
		}

		/* start search for the next BMC component address. images are all alligned to 4KB. */
		scan_offset = fw.base.start_addr + ROUND_UP (fw.size, 0x1000);
	}

	return status;
}

#ifdef _PLATFORM_RTOS_
/**
 * Task loop handles BMC reset.
 *
 * @param data Pointer to bmc task instance
 *
 */
static void bmc_task_loop (void *data)
{
	uint32_t notification = 0;
	int status;
	struct bmc_task *task = (struct bmc_task*) data;
	uint32_t fiu, cs;
	uint16_t reset_reg;
	uint16_t reset;
	uint32_t combo1_start_addr;

#ifdef BMC_DIRECT
	NCL_AES_OP_T op;
	NCL_AES_MODE_T mode;

	struct tip_aes_ncl_engine fw_enc_dec_aes;

	/* init the AES engine for the BMC_DIRECT AES commands */
	status = tip_aes_ncl_init (&fw_enc_dec_aes, ncl_shared_state);
	if (status != 0) {
		platform_printf (KRED "AES fail init" NEWLINE KNRM);
	}
#endif

	do {
		xTaskNotifyWait (pdFALSE, ULONG_MAX, &notification, portMAX_DELAY);

		/* handle BMC boot reset event */
		if (task->bmc_state == BMC_BOOTBLOCK_RUNNING) {
			platform_printf (KRED "BMC bootblock reset was detected" NEWLINE KNRM);
			task->bmc_state = BMC_BOOTBLOCK_RESET;
			platform_msleep (0);
		}

		/* handle post boot BMC reset event */
		else if (task->bmc_state == BMC_DDR_READY) {
			switch (notification) {
				case BMC_RESET_CMD:
#ifdef BMC_DIRECT_COMPOSITE_EAT
					bmc_direct_composite_eat_reset (&composite_eat_state);
#endif
					/* slow peripherals are configured to be reset in BMC reset */
					serial_printf_init (!tip_L1_sys_ctrl.tip_disable_print_to_uart, tip_L1_sys_ctrl.tip_print_to_memory);

					platform_printf (KGRN NEWLINE "========" NEWLINE "   TIP_FW   : detected   BMC reset"
						NEWLINE "========" NEWLINE "  NVIC_BMC_reset:" NEWLINE KNRM);

					reset_reg = tip_get_reset_indication ();

					/* Update WDC if needed */
					SET_REG_FIELD (INTCR2, INTCR2_WDC, recovery_boot);

					/* Update WDC if needed */
					SET_REG_FIELD (INTCR2, INTCR2_WDC, recovery_boot);

					platform_printf (KGRN "reload BMC" NEWLINE KNRM);

					/*
					* Since BMC was reset TIP is getting ready for a possible error in current
					* image. Only after current image is re-validated TIP will configure the
					* current main image as the next image.
					*/
					tip_select_next_boot_image (tip_flash_get_recovery_phys_addr());

					status = tip_flash_get_fiu_cs (active_flash, &fiu, &cs);
					if (status != 0) {
						platform_printf ("Active flash unknown" NEWLINE);
						platform_reset (0);
					}

					/* Do not trust BMC that SPI is still configured properly.
					* In next phase SPI will be locked so this step can be skipped */
					tip_flash_initialize_access_single_flash (fiu, cs);

					reset = tip_get_reset_indication ();

#ifdef CERBERUS_ENABLE_COMPONENT_ATTESTATION
					/* Handle MCTP bridge resets when there are attestable components. */
					if (has_active_pcd && pcd_has_components) {
						status = attestation_requestor_mctp_bridge_was_reset (
							&system_attestation_requester);
						if (status != 0) {
							debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR,
								DEBUG_LOG_COMPONENT_ATTESTATION,
								ATTESTATION_LOGGING_ROUTING_TABLE_REFRESH_REQUEST_FAILED, status,
								0);
						}

						/* Wait to ensure BMC is ready for routing table sync request. */
						status = platform_timer_arm_one_shot (&routing_table_rq_timer,
							get_routing_table_timeout_ms);
						if (status != 0) {
							debug_log_create_entry (DEBUG_LOG_SEVERITY_ERROR,
								DEBUG_LOG_COMPONENT_ATTESTATION,
								ATTESTATION_LOGGING_BRIDGE_FAILED_TO_DETECT_MCTP_BRIDGE_RESET,
								status, 0);
						}
					}
#endif /* CERBERUS_ENABLE_COMPONENT_ATTESTATION */

					/* Reload BMC only (without MC retraining) */
					status = tip_load_bmc_firmware (task, active_flash,
						recovery_boot ? recovery_flash_start_offset + ROT_COMBO1_ADDR_DEFAULT :
						ROT_COMBO1_ADDR_DEFAULT, &shared_hash.base, reset, &combo1_start_addr);


					if (ROT_IS_ERROR (status) == false) {
						/* mark that the reset was handled */
						tip_clear_reset_indication ();
					}

					if (status == 0) {
						bmc_export_data ();

#ifdef ENABLE_RECOVERY_PROTECTION
						/* recovery image is ready, lock any changes to recovery image until next CORST */
						if (tip_L1_sys_ctrl.tip_recovery_force) {
							protect_recovery_flash ();
						}
#endif
						/* trust this image again */
						if (recovery_boot == false) {
							tip_select_next_boot_image (SPI0CS0_BASE_ADDR);
						}

						if (recovery_boot == false) {
							tip_reset_counters_increment (reset_counter, reset_reg, task->system);
						}
						else {
							/* in case of recovery boot the reset counters are useless, TIP has no
							* better image to go to.*/
							tip_reset_counters_init (reset_counter);
						}

#ifdef BMC_DIRECT_COMPOSITE_EAT
						bmc_direct_composite_eat_reset (&composite_eat_state);
						tip_mbx_clear_notification (BMC_DIRECT_NOTIFICATION_COMPOSITE_EAT);
#endif

						bmc_continue ();

						/* Reenable the IRQ after complition */
						NVIC_EnableInt (NVIC_INT_46, TRUE);
#ifdef BMC_DIRECT_COMPOSITE_EAT
						NVIC_ClearInt (NVIC_INT_2);
						NVIC_EnableInt (NVIC_INT_2, TRUE);
#endif
#ifdef GPIO_WOL
						/* Reenable the GPIO IRQ after complition */
						if (tip_L1_sys_ctrl.tip_gpio_wol) {
							NVIC_EnableInt (GPIO_TIP_INTERRUPT (TIP_WOL_GPIO), TRUE);
						}
#endif
					}
					else {
						/* fail to reload, nothing left to do but reset TIP */
						platform_reset (0);
					}

					break;
#ifdef BMC_DIRECT_COMPOSITE_EAT
				case BMC_DIRECT_COMPOSITE_EAT_TASK_EVENT: {
					struct bmc_direct_composite_eat_request request = composite_eat_state.pending;
					struct bmc_direct_composite_eat_output output = {
						.request = &request,
					};
					size_t response_length = 0;
					enum composite_eat_tip_main_token_status generation_status;
					enum bmc_direct_composite_eat_status response_status;

					if ((composite_eat_main_token_generator == NULL) ||
						!composite_eat_state.active) {
						(void) bmc_direct_composite_eat_finish (
							BMC_DIRECT_COMPOSITE_EAT_INTERNAL, 0, &request);
						break;
					}

					generation_status = composite_eat_tip_main_token_generate_write (
						composite_eat_main_token_generator,
						(const uint8_t *) (uintptr_t) request.request_address,
						request.request_length,
						bmc_direct_composite_eat_write_output, &output,
						request.response_capacity, &response_length);
					response_status = bmc_direct_composite_eat_map_main_token_status (
						generation_status);
					(void) bmc_direct_composite_eat_finish (response_status,
						(uint32_t) response_length, &request);
					break;
				}
#endif
#ifdef BMC_DIRECT
				case BMC_DIRECT_COMMAND_FL_PROG: {
					uint32_t fiu, cs, spi, offset;
					struct spi_flash *fl;
					uint32_t src_addr = REG_READ (FLASH_PRM1_SCR);
					uint32_t dst_addr = REG_READ (FLASH_PRM2_SCR);
					uint32_t size = REG_READ (FLASH_PRM3_SCR);

					status = tip_flash_phys_to_logical (dst_addr, &fiu, &cs, &spi, &offset);
					if (status != 0) {
						platform_printf (KRED "error %#010lx out of range" NEWLINE KNRM, dst_addr);
					} else {
						fl = tip_flash_get_spi_flash (spi);

						if (offset + size > fl->state->device_size) {
							platform_printf (KRED "error end addr %#010lx out of flash (size=%#010lx)" NEWLINE KNRM,
								offset + size, fl->state->device_size);
							status = FLASH_ADDRESS_OUT_OF_RANGE;
						} else {
							/* write to flash */
							platform_printf_dbg ("copy from %#010lx to fiu%d, cs%d %#010lx bytes" NEWLINE,
								src_addr, fiu, cs, size);
							status = spi_flash_write (fl, offset, (uint8_t *) src_addr, size);
						}
					}
					tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_FL);
					break;
				}

				case BMC_DIRECT_COMMAND_FL_READ: {
					uint32_t fiu, cs, spi, offset;
					struct spi_flash *fl;
					uint32_t src_addr = REG_READ (FLASH_PRM1_SCR);
					uint32_t dst_addr = REG_READ (FLASH_PRM2_SCR);
					uint32_t size = REG_READ (FLASH_PRM3_SCR);

					status = tip_flash_phys_to_logical (src_addr, &fiu, &cs, &spi, &offset);
					if (status != 0) {
						platform_printf (KRED "error %#010lx out of range" NEWLINE KNRM, src_addr);
						tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_FL);
						break;
					}
					fl = tip_flash_get_spi_flash (spi);

					if (offset + size > fl->state->device_size) {
						platform_printf (KRED "error end addr %#010lx out of flash (size=%#010lx)" NEWLINE KNRM,
							offset + size, fl->state->device_size);
						status = FLASH_ADDRESS_OUT_OF_RANGE;
					} else {

						/* write to flash (todo: verifiy image if needed */
						platform_printf_dbg ("copy from %#010lx, fiu%d, cs%d %#010lx bytes to "
										"%#010lx" NEWLINE,
							src_addr, fiu, cs, size, dst_addr);
						status = spi_flash_read (fl, offset, (uint8_t *) dst_addr, size);
					}
					tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_FL);
					break;
				}

				case BMC_DIRECT_COMMAND_FL_ERASE_SECTOR: {
					uint32_t fiu, cs, spi, offset;
					struct spi_flash *fl;
					uint32_t dst_addr = REG_READ (FLASH_PRM2_SCR);
					uint32_t size = REG_READ (FLASH_PRM3_SCR);

					status = tip_flash_phys_to_logical (dst_addr, &fiu, &cs, &spi, &offset);
					if (status != 0) {
						platform_printf (KRED "error %#010lx out of range" NEWLINE KNRM, dst_addr);
						tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_FL);
						break;
					}
					fl = tip_flash_get_spi_flash (spi);

					if (offset + size > fl->state->device_size) {
						platform_printf (KRED "error end addr %#010lx out of flash (size=%#010lx)" NEWLINE KNRM,
							offset + size, fl->state->device_size);
						status = FLASH_ADDRESS_OUT_OF_RANGE;
					} else {

						/* erase */
						platform_printf_dbg ("erase fiu%d, cs%d %#010lx bytes" NEWLINE,
							fiu, cs, size);

						if ((size % _4KB_) || (dst_addr % _4KB_)) {
							platform_printf (KRED "erase flash should be of sectors size" NEWLINE KNRM);
							status = FLASH_ADDRESS_OUT_OF_RANGE;
						} else {
							for (int i = offset; i < offset + size; i += _4KB_) {
								platform_printf_dbg ("erase %#010lx" NEWLINE, i);
								status = spi_flash_sector_erase (fl, i);
							}
						}
					}

					tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_FL);
					break;
				}

				case BMC_DIRECT_COMMAND_FL_READ_PARMAS: {
					uint32_t fiu, cs, spi, offset;
					struct spi_flash *fl;
					uint32_t fl_addr = REG_READ (FLASH_PRM1_SCR);

					serial_printf_reconfig ();

					status = tip_flash_phys_to_logical (fl_addr, &fiu, &cs, &spi, &offset);
					if (status != 0) {
						platform_printf_dbg (KRED "error %#010lx out of range" NEWLINE KNRM, fl_addr);
						tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_FL);
						break;
					}
					fl = tip_flash_get_spi_flash (spi);

					tip_flash_initialize_access_single_flash (fiu, cs);

					if (fl->state->device_size == 0) {
						platform_printf (KRED "flash not connected" NEWLINE KNRM);
						status = FLASH_NO_MEMORY;
						tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_FL);
						break;
					}

					if (fl != NULL) {
						uint32_t size;

						REG_WRITE (FLASH_PRM2_SCR, fl->state->device_size);

						spi_flash_get_page_size (fl, &size);
						REG_WRITE (FLASH_PRM3_SCR, size);

						spi_flash_get_sector_size (fl, &size);
						REG_WRITE (FLASH_PRM4_SCR, size);

						spi_flash_get_block_size (fl, &size);
						REG_WRITE (FLASH_PRM5_SCR, size);
						REG_WRITE (FLASH_PRM6_SCR, *(uint32_t*) fl->state->device_id);
					}
					tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_FL);
					break;
				}

				case BMC_DIRECT_COMMAND_FW_UPDATE: {
uint32_t src_addr = REG_READ (FLASH_PRM1_SCR);
				uint32_t dst_addr = TIP_VIRTUAL_FLASH_BASE_ADDR;
				uint32_t size = REG_READ (FLASH_PRM3_SCR);

					/* copy to secured staging area */
					platform_printf (KGRN
						"FW UPDATE: copy %#010lx to %#010lx size %#010lx" NEWLINE KNRM,
						src_addr, dst_addr, size);
					memcpy ((void *) dst_addr, (void *) src_addr, size);

					status = cerberus_update.base.start_update (&cerberus_update.base);
					tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_FL);
					break;
				}

				case BMC_DIRECT_COMMAND_DRBG: {
					uint32_t num_of_random_bytes = REG_READ (RNG_SIZE_SCR);
					uint32_t addr = REG_READ (RNG_BUFFER_ADDR_SCR);

					/* copy to secured staging area */
					platform_printf_dbg (KGRN
						"DRBG: create %#010lx rand bytes at %#010lx" NEWLINE KNRM,
						num_of_random_bytes, addr);

					if ((addr < 96 * _1MB_) || ((addr + num_of_random_bytes) >= SDRAM_MAPPED_SIZE)) {
						platform_printf (KRED
						"ERROR DRBG: create %#010lx rand bytes at %#010lx out of range"
							NEWLINE KNRM, num_of_random_bytes, addr);
						status = RNG_ENGINE_NO_MEMORY;
					}
					else {
						status = system_rng.base.generate_random_buffer (&(system_rng.base), num_of_random_bytes, (uint8_t *)addr);
					}
					tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_RNG);
					break;
				}

				case BMC_DIRECT_COMMAND_AES_ENC_ECB:
				case BMC_DIRECT_COMMAND_AES_ENC_CBC:
				case BMC_DIRECT_COMMAND_AES_ENC_CTR:
				case BMC_DIRECT_COMMAND_AES_ENC_GCM:


				case BMC_DIRECT_COMMAND_AES_DEC_ECB:
				case BMC_DIRECT_COMMAND_AES_DEC_CBC:
				case BMC_DIRECT_COMMAND_AES_DEC_CTR:
				case BMC_DIRECT_COMMAND_AES_DEC_GCM: {

					op = (NCL_AES_OP_T) notification & 0x01;
					mode = (NCL_AES_MODE_T)(notification >> 4 ) - 1;

					/*
					 * SCRPAD 24 – address of block to encrypt/decrypt
					 * SCRPAD 25 – size of block
					 * SCRPAD 26 – operation. 0 encrypt, 1 decrypt
					 * SCRPAD 27 – destination of AES output
					 * SCRPAD 28 – IV info
					 * SCRPAD 29 – Key info
					 * SCRPAD 30 – TAG, Currently 0
					 *
					 * IV and Key info is a block of data
					 * Offset 0           length
					 * Offset 4-length+4  IV/Key data
					 */

					struct info {
						uint32_t size;
						uint32_t arr;
					};

					uint32_t addr_src = REG_READ (AES_BLOCK_ADDR_SCR);
					uint32_t addr_dst = REG_READ (AES_OUTPUT_ADDR_SCR);
					uint32_t size = REG_READ (AES_BLOCK_SIZE_SCR);

					struct info *iv_info = (struct info *)REG_READ (AES_IV_INFO_SCR);
					struct info *key_info = (struct info *)REG_READ (AES_KEY_INFO_SCR);
					struct info *tag_info = (struct info *)REG_READ (AES_TAG_INFO_SCR);

					uint8_t *iv = NULL;
					uint32_t iv_size = 0;
					uint8_t *key = NULL;
					uint32_t key_size = 0;
					uint8_t *tag = NULL;
					uint32_t tag_size = 0;

					if (iv_info != NULL) {
						iv = (uint8_t *)&iv_info->arr;
						iv_size = iv_info->size & 0x00007FF;
					}

					if (key_info != NULL) {
						key = (uint8_t *)&key_info->arr;
						key_size = key_info->size & 0x00007FF;
					}

					if (tag_info != NULL) {
						tag = (uint8_t *)&tag_info->arr;
						tag_size = tag_info->size & 0x00007FF;
					}

					platform_printf ( "scrpad24 %#010lx %#010lx \n", REG_ADDR (AES_BLOCK_ADDR_SCR),  REG_READ(AES_BLOCK_ADDR_SCR));
					platform_printf ( "scrpad27 %#010lx %#010lx \n", REG_ADDR (AES_OUTPUT_ADDR_SCR),  REG_READ(AES_OUTPUT_ADDR_SCR));
					platform_printf ( "scrpad28 %#010lx %#010lx \n", REG_ADDR (AES_IV_INFO_SCR),  REG_READ(AES_IV_INFO_SCR));
					platform_printf ( "scrpad29 %#010lx %#010lx \n", REG_ADDR (AES_KEY_INFO_SCR),  REG_READ(AES_KEY_INFO_SCR));

					hex_dump ((uint32_t)key, key_size, "key");

					hex_dump ((uint32_t)iv, iv_size, "iv");

					/* copy to secured staging area */
					platform_printf (KGRN
						"AES: addr_src = %#010lx "
						"addr_dst = %#010lx "
						"size = %#010lx "
						"iv = %#010lx "
						"iv_size = %#010lx "
						"key = %#010lx "
						"key_size = %#010lx "
						"tag = %#010lx "
						"tag_size = %#010lx "
						"op = %x "
						"mode = %x " NEWLINE,
						addr_src, addr_dst, size, iv, iv_size, key, key_size, tag, tag_size, op, mode);

						status = fw_enc_dec_aes.set_mode (&fw_enc_dec_aes, mode);
						if (status != 0) {
							goto bmc_direct_fin;
						}

						status = fw_enc_dec_aes.base.set_key (&fw_enc_dec_aes.base, (uint8_t*)key, key_size);
						if (status != 0) {
							goto bmc_direct_fin;
						}

						if (op == NCL_AES_OP_ENCRYPT) {
							status = fw_enc_dec_aes.base.encrypt_data (&(fw_enc_dec_aes.base), (uint8_t*) addr_src,
								size, (uint8_t*) iv, iv_size, (uint8_t*) addr_dst, size, (uint8_t*)tag,
								tag_size);
						} else {
							status = fw_enc_dec_aes.base.decrypt_data (&(fw_enc_dec_aes.base), (uint8_t*) addr_src,
								size, (uint8_t*)tag, (uint8_t*) iv, iv_size, (uint8_t*) addr_dst, size);
						}

bmc_direct_fin:
					tip_bmc_direct_finalize_command (status, BMC_DIRECT_NOTIFCATION_AES);
					break;
				}

#endif /* BMC_DIRECT */

				default:
					platform_msleep (0);
			}
		}
		else {
			/* Clear and enable BMC reset event */
			NVIC_ClearInt ((NVIC_INT_SRC_T) NVIC_INT_46);
			NVIC_EnableInt ((NVIC_INT_SRC_T) NVIC_INT_46, TRUE);

			platform_msleep (0);
		}

	} while (1);
}

#if configSUPPORT_DYNAMIC_ALLOCATION == 1
/**
 * Initialize and start the task to process bmc reset.
 *
 * @note PCD and CFM must be initialized prior to calling this, if component attestation is enabled.
 *
 * @param task The bmc command task to initialize.
 * @param priority The priority level for running the bmc reset task.
 * @param stack_words The size of the command task stack. The stack size is measured in words.
 * @param system TIP system manager to utilize.
 *
 * @return Initialization status, 0 if success or an error code.
 */
int bmc_task_init (struct bmc_task *task, int priority, uint16_t stack_words,
	struct system *system)
{
	int status;

	if (task == NULL) {
		return -1;
	}

	memset (task, 0, sizeof (struct bmc_task));

	task->bmc_state = BMC_DISABLE;
	task->system = system;
	tip_reset_counters_init (reset_counter);

#ifdef BMC_DIRECT
	/* Detect BMC reset interrupt: INT46 Level High BMC CPU reset Interrupt */
	NVIC_InstallSwHandler (NVIC_INT_2, (SW_HANDLER_T) NVIC_BMC_direct_handler);
	NVIC_ConfigPriority (NVIC_INT_2, 0x5);
	NVIC_ClearInt (NVIC_INT_2);
	NVIC_EnableInt (NVIC_INT_2, FALSE);
#endif

	status = xTaskCreate (bmc_task_loop, "BMC_LOOP", stack_words, (void*) task, priority,
		&task->bmc_task);
	if (status != pdPASS) {
		return status;
	}

	return 0;
}
#endif

#if configSUPPORT_STATIC_ALLOCATION == 1
/**
 * Create BMC reset handling task with statically allocated stack and start the task to process bmc reset.
 *
 * @note PCD and CFM must be initialized prior to calling this, if component attestation is enabled.
 *
 * @param task The bmc command task to initialize.
 * @param context Statically allocated context for the FreeRTOS task.
 * @param stack Buffer to use for the task's stack.
 * @param stack_words The size of the command task stack. The stack size is measured in words.
 * @param priority The priority level for running the bmc reset task.
 * @param system TIP system manager to utilize.
 *
 * @return Initialization status, 0 if success or an error code.
 */
int bmc_task_init_static (struct bmc_task *task, StaticTask_t *context, StackType_t *stack,
	uint16_t stack_words, int priority, struct system *system)
{
	if (task == NULL) {
		return -1;
	}

	memset (task, 0, sizeof (struct bmc_task));

	task->bmc_state = BMC_DISABLE;
	tip_reset_counters_init (reset_counter);

#ifdef BMC_DIRECT
	/* Detect BMC reset interrupt: INT46 Level High BMC CPU reset Interrupt */
	NVIC_InstallSwHandler (NVIC_INT_2, (SW_HANDLER_T) NVIC_BMC_direct_handler);
	NVIC_ConfigPriority (NVIC_INT_2, 0x5);
	NVIC_ClearInt (NVIC_INT_2);
	NVIC_EnableInt (NVIC_INT_2, FALSE);
#endif

	task->bmc_task = xTaskCreateStatic (bmc_task_loop, "BMC_LOOP", stack_words, (void*) task,
		priority, stack, context);
	if (task->bmc_task == NULL) {
		/* TODO : Fix this to return meaningful error code. */
		return -1;
	}

	return 0;
}
#endif /* configSUPPORT_STATIC_ALLOCATION */

#endif /* _PLATFORM_RTOS_ */

/**
 * Get the buffer to store BMC component digest.
 *
 * @param img Image type
 * @return uint8_t*  Digest buffer
 */
uint8_t *bmc_component_get_digest_buf (IMG_TYPE_E img)
{
	uint8_t *digest_buf;

	switch (img) {
		case IMG_BOOTBLOCK:
			digest_buf = bb_hash;
			break;
		case IMG_BL31:
			digest_buf = bl31_hash;
			break;
		case IMG_OPTEE:
			digest_buf = optee_hash;
			break;
		case IMG_UBOOT:
			digest_buf = uboot_hash;
			break;
		default:
			digest_buf = NULL;
			break;
	}

	return digest_buf;
}

/**
 * Export data to BMC directly. For direct data processing w\o utilites.
 * uboot\optee can read this data directly and send to TPM.
 * data will be stored on PCI mailbox.
 */
void bmc_export_data (void)
{
	uint8_t *mailbox = (uint8_t *) PCIMBX_BASE_ADDR (0);
	struct riot_shared_attestation *riot = (struct riot_shared_attestation *) RIOT_SHARED_ADDRESS;
	uint32_t chip_revision = CHIP_Get_Version ();
	struct tip_rom_dme_public_evidence dme;
	size_t size_to_copy;
	uint8_t *mailbox_attestation;
	int status;

	/* export not needed for Z1 */
	if (chip_revision == ARBEL_VERSION_Z1) {
		return;
	}
	if (tip_rom_dme_handoff_get (&dme) != TIP_ROM_DME_HANDOFF_OK) {
		return;
	}

#if TIP_DUMMY_OTP_ON_RAM
	memset (mailbox, 0, PCIMBX_RAM_SIZE - SHA512_HASH_LENGTH);
#else
	memset (mailbox, 0, PCIMBX_RAM_SIZE);
#endif

	platform_printf ("export dme_nonce at %#010lx %dB" NEWLINE, mailbox,
		dme.dme_nonce.length);
	memcpy (mailbox, dme.dme_nonce.data, dme.dme_nonce.length);
	mailbox += dme.dme_nonce.length;

	if (dme.dme_challenge.length != 0) {
		platform_printf ("export dme_challenge at %#010lx %dB" NEWLINE, mailbox,
			dme.dme_challenge.length);
		memcpy (mailbox, dme.dme_challenge.data, dme.dme_challenge.length);
	}
	else {
		platform_printf ("export challenge not supported. clear %#010lx %dB" NEWLINE,
			mailbox, SEC_DME_CHALLENGE_LENGTH);
		memset (mailbox, 0, SEC_DME_CHALLENGE_LENGTH);
	}
	mailbox += SEC_DME_CHALLENGE_LENGTH;

	platform_printf ("export dice_pub_key at %#010lx %dB" NEWLINE, mailbox,
		dme.dice_public_key.length);
	memcpy (mailbox, dme.dice_public_key.data, dme.dice_public_key.length);
	mailbox += dme.dice_public_key.length;

	platform_printf ("export dme_pcr0 at %#010lx %dB" NEWLINE, mailbox,
		dme.dme_pcr0.length);
	memcpy (mailbox, dme.dme_pcr0.data, dme.dme_pcr0.length);
	mailbox += dme.dme_pcr0.length;

	platform_printf ("export dme_pub_key at %#010lx %dB" NEWLINE, mailbox,
		dme.dme_public_key.length);
	memcpy (mailbox, dme.dme_public_key.data, dme.dme_public_key.length);
	mailbox += dme.dme_public_key.length;

	platform_printf ("export dme_signature at %#010lx %dB" NEWLINE, mailbox,
		dme.dme_signature.length);
	memcpy (mailbox, dme.dme_signature.data, dme.dme_signature.length);
	mailbox += dme.dme_signature.length;

	/* Copy shared attestation area, excluding alias_key */
	size_to_copy = offsetof(struct riot_shared_attestation, alias_key);
	mailbox_attestation = mailbox;
	memcpy (mailbox, (uint8_t *) riot, size_to_copy);

	/* skip alias key, but keep struct format the same */
	mailbox += size_to_copy + MAX_ALIAS_KEY_LENGTH + sizeof(int);

	/* Copy second portion of RIoT struct, not including the hash at the end, since it must be
	 * calculated again without the alias key.
	 */
	size_to_copy = sizeof (struct riot_shared_attestation) - offsetof (struct riot_shared_attestation, alias_cert) - SHA256_HASH_LENGTH;
	memcpy (mailbox, (uint8_t *) riot + offsetof (struct riot_shared_attestation, alias_cert), size_to_copy);

	/* Calculate the hash of all the keys and the certificates shared with the main application. */
	status = system_hash.base.calculate_sha256 (&system_hash.base, (uint8_t*) mailbox_attestation,
		sizeof (struct riot_shared_attestation) - sizeof (riot->attestation_hash),
		mailbox_attestation + offsetof (struct riot_shared_attestation, attestation_hash),
		sizeof (riot->attestation_hash));
	if (status) {
		platform_printf ("attestation hash fail" NEWLINE);
	}

	mailbox += size_to_copy;

	memcpy (mailbox, bb_hash, sizeof (bb_hash));
	mailbox += sizeof (bb_hash);

	memcpy (mailbox, bl31_hash, sizeof (bl31_hash));
	mailbox += sizeof (bl31_hash);

	memcpy (mailbox, optee_hash, sizeof (optee_hash));
	mailbox += sizeof (optee_hash);

	memcpy (mailbox, uboot_hash, sizeof (uboot_hash));
	mailbox += sizeof (uboot_hash);

	return;
}
