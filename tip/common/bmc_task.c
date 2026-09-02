// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "attestation/attestation_requester.h"
#include "attestation/attestation_logging.h"
#include "common/unused.h"
#include "crypto/hash_thread_safe.h"
#include "flash/flash_common.h"
#include "flash/spi_flash.h"
#include "bmc_task.h"
#include "platform_api.h"
#include "platform_io.h"
#include "rot_memory_map.h"
#include "tip_boot.h"
#include "tip_firmware_component.h"
#include "tip_flash.h"
#include "tip_fw_update_task.h"
#include "tip_mbx.h"
#include "tip_rom_utils.h"
#include "tip_utils.h"
#include "tip_reset.h"

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

/**
 * NVIC Reset BMC interrupt
 *
 * @param num Interrupt number
 */
void NVIC_BMC_reset (uint16_t num)
{
	BaseType_t reset_priority = pdTRUE;
	uint16_t int_num = SCS_GetActiveVectorNumber () - NVIC_TRAP_NUM;
	uint16_t reset_reg;

	tip_bmc_tim_disable_all_wd ();

	/* slow peripherals are configured to be reset in BMC reset */
	serial_printf_init ();

	/* interrupt will be reenabled after BMC is reloaded in bmc_task */
	NVIC_EnableInt (int_num, FALSE);


	platform_printf (KGRN NEWLINE "========" NEWLINE "TIP_FW: detected BMC reset" NEWLINE
								  "========" NEWLINE "NVIC_BMC_reset: int %d" NEWLINE KNRM,
		int_num);

	reset_reg = tip_get_reset_indication ();
	if (recovery_boot == false) {
		tip_reset_counters_increment (reset_counter, reset_reg);
	}
	else {
		/* in case of recovery boot the reset counters are useless, TIP has no 
		 * better image to go to.
		 */
		tip_reset_counters_init (reset_counter);
	}

	xTaskNotifyFromISR (bmc_reset_task.bmc_task, BMC_RESET_CMD, eSetValueWithOverwrite,
		&reset_priority);
}

#ifdef BMC_DIRECT
void NVIC_BMC_direct_handler (uint16_t num)
{
	uint32_t notification;
	BaseType_t reset_priority = pdTRUE;
	bool int_processed = false;
	uint16_t int_num = SCS_GetActiveVectorNumber () - NVIC_TRAP_NUM;

	if (bmc_reset_task.bmc_state != BMC_DDR_READY) {
		NVIC_ClearInt (int_num);
		return;
	}

	/* interrupt will be reenabled after BMC is reloaded in bmc_task */
	NVIC_EnableInt (int_num, FALSE);
	notification = tip_mbx_get_notification ();

	serial_printf_reconfig ();

	platform_printf (KGRN NEWLINE
		"======== TIP_FW: detected BMC int %d notification %#010lx" NEWLINE KNRM,
		int_num, notification);

	/* check if it's a BMC_DIRECT command */
	if ((notification & BMC_DIRECT_NOTIFCATION) > 0) {
		uint32_t cmd = REG_READ (SCRPAD_10_41 (10));
		/* prevent reuse of command. FFFFFFF means command is being executed. */
		REG_WRITE (SCRPAD_10_41 (10), 0xFFFFFFFF);
		if ((cmd > BMC_RESET_0) && (cmd < BMC_MAX_COMMANDS)) {
			xTaskNotifyFromISR (bmc_reset_task.bmc_task, cmd, eSetValueWithOverwrite,
				&reset_priority);
			int_processed = true;
		}
		else {
			platform_printf (KRED "BMC direct command %d unknown" NEWLINE KNRM, cmd);
		}

		tip_mbx_clear_notification (BMC_DIRECT_NOTIFCATION);
	}
	else if (notification & system_i2c.notification_idx) {
		/* cerberus_utility interrupt. uses polling only. ignore and avoid clearing */
		return;
	}
	else {
		platform_printf (KRED "BMC notification reg is %d unknown" NEWLINE KNRM, notification);
		tip_mbx_clear_notification (notification);
	}

	/* Clear BMC notification event */
	NVIC_ClearInt (int_num);
	if (int_processed == false) {
		NVIC_EnableInt (int_num, TRUE);
	}
}
#endif /* BMC_DIRECT */


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
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_TIP_RST_CTL, 1);
	SET_REG_FIELD (TIP_DBG_CTL, TIP_DBG_CTL_BMC_JEN, TIP_SEC_TRUE);

	platform_printf ("Release A35, TIP_CTL_STS = %#010lx A35_MODE = %#010lx\n" NEWLINE,
		REG_READ (TIP_CTL_STS), a35_mode);
	REG_WRITE (A35_MODE, a35_mode);

	/* clear scratchpads before use */
	for (int i = 0; i < 31; i++)
		REG_WRITE (SCRPAD_10_41 (i), 0);

	DISABLE_INTERRUPTS ();
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
	serial_printf (KYEL "Uptime %lu ms" NEWLINE KNRM, (uint32_t) time64);

	/* set wake up address for core 0: */
	REG_WRITE (SCRPAD_10_41 (3), 0);
	REG_WRITE (SCRPAD_10_41 (2), addr);

	/* notify BMC */
	SET_REG_BIT (TIP2BNT1, 3);

	/* wake core 1: */
	CLK_Delay_MicroSec (5);
	REG_WRITE (SCRPAD_10_41 (5), 0);
	REG_WRITE (SCRPAD_10_41 (4), addr);

	/* wake core 2: */
	CLK_Delay_MicroSec (5);
	REG_WRITE (SCRPAD_10_41 (7), 0);
	REG_WRITE (SCRPAD_10_41 (6), addr);

	/* wake core 3: */
	CLK_Delay_MicroSec (5);
	REG_WRITE (SCRPAD_10_41 (9), 0);
	REG_WRITE (SCRPAD_10_41 (8), addr);

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
	//  wait for NTTA !!!!   tip_bmc_tim_start_wd (BMC_WD_NUM, BMC_WD_TIME_UBOOT);

	/* continue to bl31 */
	bmc_continue_l (bmc_start_addr[1] + sizeof (HEADER_GENERAL_T));

#ifdef BMC_DIRECT
	NVIC_ClearInt (NVIC_INT_2);
	NVIC_EnableInt (NVIC_INT_2, TRUE);
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
			bmc_component_get_digest_buf (img_type), SHA512_HASH_LENGTH);

		if (status != 0 && TIP_SECBOOT_IS_ACTIVE ()) {
			platform_printf (KRED "Image at %#010lx failed authentication" NEWLINE KNRM,
				(uint32_t) fw.header_ram + sizeof (HEADER_GENERAL_T));

			return status;
		}

		status = tip_firmware_component_load_external_reg_table (&fw, reset_source);
		if (ROT_IS_ERROR (status)) {
			return status;
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
	struct bmc_task *task = (struct bmc_task *) data;
	uint16_t reset;
	uint32_t combo1_start_addr;
	uint32_t fiu, cs;

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
#endif

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

						/* recovery image is ready, lock any changes to recovery image until next CORST */
						protect_recovery_flash ();

						/* trust this image again */
						if (recovery_boot == false) {
							tip_select_next_boot_image (SPI0CS0_BASE_ADDR);
						}

						bmc_continue ();

						/* Reenable the IRQ after complition */
						NVIC_EnableInt (NVIC_INT_46, TRUE);
					}
					else {
						/* fail to reload, nothing left to do but reset TIP */
						platform_reset (0);
					}

					break;
#ifdef BMC_DIRECT
				case BMC_DIRECT_COMMAND_FL_PROG: {
					uint32_t fiu, cs, spi, offset;
					struct spi_flash *fl;
					uint32_t src_addr = REG_READ (SCRPAD_10_41 (11));
					uint32_t dst_addr = REG_READ (SCRPAD_10_41 (12));
					uint32_t size = REG_READ (SCRPAD_10_41 (13));

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
							platform_printf ("copy from %#010lx to fiu%d, cs%d %#010lx bytes" NEWLINE,
								src_addr, fiu, cs, size);
							tip_flash_initialize_access_single_flash (fiu, cs);
							status = spi_flash_write (fl, offset, (uint8_t *) src_addr, size);
						}
					}
					goto bmc_direct_finalize;
					break;
				}

				case BMC_DIRECT_COMMAND_FL_READ: {
					uint32_t fiu, cs, spi, offset;
					struct spi_flash *fl;
					uint32_t src_addr = REG_READ (SCRPAD_10_41 (11));
					uint32_t dst_addr = REG_READ (SCRPAD_10_41 (12));
					uint32_t size = REG_READ (SCRPAD_10_41 (13));

					status = tip_flash_phys_to_logical (src_addr, &fiu, &cs, &spi, &offset);
					if (status != 0) {
						platform_printf (KRED "error %#010lx out of range" NEWLINE KNRM, src_addr);
						goto bmc_direct_finalize;
						break;
					}
					fl = tip_flash_get_spi_flash (spi);

					if (offset + size > fl->state->device_size) {
						platform_printf (KRED "error end addr %#010lx out of flash (size=%#010lx)" NEWLINE KNRM,
							offset + size, fl->state->device_size);
						status = FLASH_ADDRESS_OUT_OF_RANGE;
						goto bmc_direct_finalize;
					} else {

						/* write to flash (todo: verifiy image if needed */
						platform_printf ("copy from %#010lx, fiu%d, cs%d %#010lx bytes to "
										 "%#010lx" NEWLINE,
							src_addr, fiu, cs, size, dst_addr);
						tip_flash_initialize_access_single_flash (fiu, cs);
						status = spi_flash_read (fl, offset, (uint8_t *) dst_addr, size);
					}
					goto bmc_direct_finalize;
					break;
				}

				case BMC_DIRECT_COMMAND_FL_ERASE_SECTOR: {
					uint32_t fiu, cs, spi, offset;
					struct spi_flash *fl;
					uint32_t dst_addr = REG_READ (SCRPAD_10_41 (12));
					uint32_t size = REG_READ (SCRPAD_10_41 (13));

					status = tip_flash_phys_to_logical (dst_addr, &fiu, &cs, &spi, &offset);
					if (status != 0) {
						platform_printf (KRED "error %#010lx out of range" NEWLINE KNRM, dst_addr);
						goto bmc_direct_finalize;
					}
					fl = tip_flash_get_spi_flash (spi);

					if (offset + size > fl->state->device_size) {
						platform_printf (KRED "error end addr %#010lx out of flash (size=%#010lx)" NEWLINE KNRM,
							offset + size, fl->state->device_size);
						status = FLASH_ADDRESS_OUT_OF_RANGE;
						goto bmc_direct_finalize;
					} else {

						/* erase */
						platform_printf ("erase fiu%d, cs%d %#010lx bytes" NEWLINE,
							fiu, cs, size);

						if ((size % _4KB_) || (dst_addr % _4KB_)) {
							platform_printf (KRED "erase flash should be of sectors size" NEWLINE KNRM);
							status = FLASH_ADDRESS_OUT_OF_RANGE;
							goto bmc_direct_finalize;
						} else {
							tip_flash_initialize_access_single_flash (fiu, cs);
							for (int i = offset; i < offset + size; i += _4KB_) {
								platform_printf ("erase %#010lx" NEWLINE, i);
								status = spi_flash_sector_erase (fl, i);
							}
						}
					}
					goto bmc_direct_finalize;
					break;
				}

				case BMC_DIRECT_COMMAND_FL_READ_PARMAS: {
					uint32_t fiu, cs, spi, offset;
					struct spi_flash *fl;
					uint32_t fl_addr = REG_READ (SCRPAD_10_41 (11));

					status = tip_flash_phys_to_logical (fl_addr, &fiu, &cs, &spi, &offset);
					if (status != 0) {
						platform_printf (KRED "error %#010lx out of range" NEWLINE KNRM, fl_addr);
						goto bmc_direct_finalize;
					}
					fl = tip_flash_get_spi_flash (spi);

					if (fl->state->device_size == 0) {
						platform_printf (KRED "flash not connected" NEWLINE KNRM);
						status = FLASH_NO_MEMORY;
						goto bmc_direct_finalize;
					}

					if (fl != NULL) {
						REG_WRITE (SCRPAD_10_41 (12), fl->state->device_size);
						REG_WRITE (SCRPAD_10_41 (13), FLASH_PAGE_SIZE);
						REG_WRITE (SCRPAD_10_41 (14), FLASH_SECTOR_SIZE);
						REG_WRITE (SCRPAD_10_41 (15), FLASH_BLOCK_SIZE);
						REG_WRITE (SCRPAD_10_41 (16), *(uint32_t*) fl->state->device_id);
					}
					goto bmc_direct_finalize;
					break;
				}

				case BMC_DIRECT_COMMAND_FW_UPDATE: {
					uint32_t src_addr = REG_READ (SCRPAD_10_41 (11));
					uint32_t dst_addr = TIP_VIRTUAL_FLASH_BASE_ADDR;
					uint32_t size = REG_READ (SCRPAD_10_41 (13));

					/* copy to secured staging area */
					platform_printf (KGRN
						"FW UPDATE: copy %#010lx to %#010lx size %#010lx" NEWLINE KNRM,
						src_addr, dst_addr, size);
					memcpy ((void *) dst_addr, (void *) src_addr, size);

					status = cerberus_update.base.start_update (&cerberus_update.base);
					goto bmc_direct_finalize;
					break;
				}

bmc_direct_finalize:
					if (ROT_IS_ERROR(status) == false) {
						status = 0;
					}
					platform_printf ("Done status=%#010lx" NEWLINE, status);
					
					/* return status to BMC and the command register */
					REG_WRITE (SCRPAD_10_41 (10), status);
					
					/* Reenable the IRQ after complition */
					NVIC_EnableInt (NVIC_INT_2, TRUE);
					
					/* notify BMC on complition */
					tip_mbx_notify_to_bmc (BMC_DIRECT_NOTIFCATION);
					break;
#endif

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

/**
 * Initialize and start the task to process bmc reset.
 *
 * @note PCD and CFM must be initialized prior to calling this, if component attestation is enabled.
 *
 * @param task The bmc command task to initialize.
 * @param priority The priority level for running the bmc reset task.
 * @param stack_words The size of the command task stack. The stack size is measured in words.
 *
 * @return Initialization status, 0 if success or an error code.
 */
int bmc_task_init (struct bmc_task *task, int priority, uint16_t stack_words)
{
	int status;

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

	status = xTaskCreate (bmc_task_loop, "BMC_LOOP", stack_words, task, priority, &task->bmc_task);
	if (status != pdPASS) {
		return status;
	}

	return 0;
}
#endif

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
	extern uint32_t __dme_dice_table_start;
	SEC_DME_DICE_T_A2 *dme_dice = (SEC_DME_DICE_T_A2 *) &__dme_dice_table_start;

	/* export not needed for Z1 */
	if (chip_revision == 0x00) {
		return;
	}

#if TIP_DUMMY_OTP_ON_RAM
	memset (mailbox, 0, PCIMBX_RAM_SIZE - SHA512_HASH_LENGTH);
#else 
	memset (mailbox, 0, PCIMBX_RAM_SIZE);
#endif

	/* A2 and above */
	if (chip_revision >= 0x08) {
		platform_printf ("export dme_nonce at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dme_dice->dme_nonce));
		memcpy (mailbox, (uint8_t *) dme_dice->dme_nonce, sizeof (dme_dice->dme_nonce));
		mailbox += sizeof (dme_dice->dme_nonce);

		platform_printf ("export dme_challenge at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dme_dice->dme_challenge));
		memcpy (mailbox, (uint8_t *) dme_dice->dme_challenge, sizeof (dme_dice->dme_challenge));
		mailbox += sizeof (dme_dice->dme_challenge);

		platform_printf ("export dice_pub_key at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dme_dice->dice_pub_key));
		memcpy (mailbox, (uint8_t *) dme_dice->dice_pub_key, sizeof (dme_dice->dice_pub_key));
		mailbox += sizeof (dme_dice->dice_pub_key);

		platform_printf ("export dme_pcr0 at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dme_dice->dme_pcr0));
		memcpy (mailbox, (uint8_t *) dme_dice->dme_pcr0, sizeof (dme_dice->dme_pcr0));
		mailbox += sizeof (dme_dice->dme_pcr0);

		platform_printf ("export dme_pub_key at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dme_dice->dme_pub_key));
		memcpy (mailbox, (uint8_t *) dme_dice->dme_pub_key, sizeof (dme_dice->dme_pub_key));
		mailbox += sizeof (dme_dice->dme_pub_key);

		platform_printf ("export dme_signature at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dme_dice->dme_signature));
		memcpy (mailbox, (uint8_t *) dme_dice->dme_signature, sizeof (dme_dice->dme_signature));
		mailbox += sizeof (dme_dice->dme_signature);
	}

	/* A1 */
	if (chip_revision == 0x04) {
		SEC_DME_DICE_T_A1 *dice_a1 = (SEC_DME_DICE_T_A1 *) &dme_dice;

		platform_printf ("export dme_nonce at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dice_a1->dme_nonce));
		memcpy (mailbox, (uint8_t *) dice_a1->dme_nonce, sizeof (dice_a1->dme_nonce));
		mailbox += sizeof (dice_a1->dme_nonce);

		platform_printf ("export challenge not supported for A1. clear %#010lx %dB" NEWLINE,
			mailbox, sizeof (dice_a1->dme_nonce));
		memset (mailbox, 0, sizeof (dme_dice->dme_challenge));
		mailbox += sizeof (dme_dice->dme_challenge);

		platform_printf ("export dice_pub_key at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dice_a1->dice_pub_key));
		memcpy (mailbox, (uint8_t *) dice_a1->dice_pub_key, sizeof (dice_a1->dice_pub_key));
		mailbox += sizeof (dice_a1->dice_pub_key);

		platform_printf ("export dme_pcr0 at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dice_a1->dme_pcr0));
		memcpy (mailbox, (uint8_t *) dice_a1->dme_pcr0, sizeof (dice_a1->dme_pcr0));
		mailbox += sizeof (dice_a1->dme_pcr0);

		platform_printf ("export dme_pub_key at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dice_a1->dme_pub_key));
		memcpy (mailbox, (uint8_t *) dice_a1->dme_pub_key, sizeof (dice_a1->dme_pub_key));
		mailbox += sizeof (dice_a1->dme_pub_key);

		platform_printf ("export dme_signature at %#010lx %dB" NEWLINE, mailbox,
			sizeof (dice_a1->dme_signature));
		memcpy (mailbox, (uint8_t *) dice_a1->dme_signature, sizeof (dice_a1->dme_signature));
		mailbox += sizeof (dice_a1->dme_signature);
	}

	memcpy (mailbox, (uint8_t *) riot, sizeof (struct riot_shared_attestation));
	mailbox += sizeof (struct riot_shared_attestation);

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
