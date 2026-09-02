// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technologies. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "tip_boot.h"
#include "tip_flash.h"
#include "platform_io.h"
#include "crypto/hash_thread_safe.h"
#include "bmc_task.h"
#include "tip_firmware_component.h"
#include "tip_rom_utils.h"
#include "tip_mbx.h"
#include "rot_memory_map.h"


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
 * NVIC Reset BMC interrupt
 *
 * @param num Interrupt number
 */
void NVIC_BMC_reset (uint16_t num)
{
	BaseType_t reset_priority = pdTRUE;
	uint16_t int_num = SCS_GetActiveVectorNumber ();

	serial_printf_reconfig ();

	/* interrupt will be reenabled after BMC is reloaded in bmc_task */
	NVIC_EnableInt (NVIC_INT_46, FALSE);

	/* Clear BMC reset event */
	NVIC_ClearInt ((NVIC_INT_SRC_T) int_num);

	platform_printf (KGRN NEWLINE "========" NEWLINE "TIP_FW: detected BMC reset" NEWLINE
					 "========" NEWLINE "NVIC_BMC_reset: int %d" NEWLINE KNRM, int_num);

	tip_get_reset_indication ();

	xTaskNotifyFromISR (bmc_reset_task.bmc_task, 1, eIncrement, &reset_priority);
}

/**
 *  This function is called only after A35 firmware load.
 *	Once FW is ready and verified on A35 RAM, reset A35 and configure it to start
 *  executing the pre-loaded fw.
 *
 *  @param [in] addr  Addr of FW, including header.
 *  @param [in] wait_for_finish  True if waiting for BB to finish execution.
 *
 */
void bmc_start (uint32_t addr, bool wait_for_finish)
{
	uint32_t tip_bmc_rctl, a35_mode;

	a35_mode = (BUILD_FIELD_VAL (A35_MODE_AA64B_AA32, 1) | BUILD_FIELD_VAL (A35_MODE_AA32BAS, 1) |
				BUILD_FIELD_VAL (A35_MODE_AA64BA, (addr + sizeof (HEADER_GENERAL_T)) >> 8));

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

	tip_bmc_rctl = REG_READ (TIP_BMC_RCTL);

	/* Set TIP_BMC_RCTL_A35_ASYNC_RST to R/W */
	SET_VAR_FIELD (tip_bmc_rctl, TIP_BMC_RCTL_RCTL_QUAL, 0xA5);
	SET_VAR_FIELD (tip_bmc_rctl, TIP_BMC_RCTL_A35_ASYNC_RST, 1);

	platform_printf ("TIP_BMC_RCTL => %#010lx " NEWLINE, tip_bmc_rctl);
	REG_WRITE (TIP_BMC_RCTL, tip_bmc_rctl);
	DELAY_USEC (100, 0);
	platform_printf ("TIP_BMC_RCTL = %#010lx " NEWLINE, REG_READ (TIP_BMC_RCTL));

	/* release A35 */
	SET_VAR_FIELD (tip_bmc_rctl, TIP_BMC_RCTL_A35_ASYNC_RST, 0);

	platform_printf ("TIP_BMC_RCTL => %#010lx \n\nWait for bootblock..", tip_bmc_rctl);
	REG_WRITE (TIP_BMC_RCTL, tip_bmc_rctl);

	/* wait for BootBlock to init the DDR: */
	if (wait_for_finish) {
		while (REG_READ (B2TIPST0) == 0);
		tip_mbx_clear_notification (0xFFFFFFFF);
		serial_printf_reconfig ();
	}

	/* clear status registers of BMC */
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_BMC_CRST_STS, 1);
	SET_REG_FIELD (TIP_CTL_STS, TIP_CTL_STS_DBGRST_STS, 1);
	NVIC_ClearInt (NVIC_INT_46);

	ENABLE_INTERRUPTS ();
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
	CLK_Delay_Cycles(1000);

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

	/* continue to bl31 */
	bmc_continue_l (bmc_start_addr[1] + sizeof (HEADER_GENERAL_T));
}

/**
 * Load BMC firmware components to memory and verify in order
 *
 * @param flash Flash to load firmware from.
 * @param start_offset  Start offset from the flash
 * @param hash Hash engine to utilize.
 * @return 1 if success or an error code.
 */
int tip_load_bmc_firmware (struct spi_flash *flash, uint32_t start_offset, struct hash_engine *hash)
{
	struct tip_firmware_component fw;
	uint32_t img_type;
	int status;
	uint32_t scan_offset = start_offset;
	int img_cnt = 0;

	/* Iterate on the BMC images. Note: image enums are bitwise. */
	for (img_type = IMG_BOOTBLOCK; img_type <= IMG_UBOOT; img_type = img_type << 1) {
		platform_printf (KCYN NEWLINE "==========" NEWLINE "Start %s" NEWLINE
						 "==========" NEWLINE KNRM, tip_firmware_component_get_name (img_type));

		status = tip_firmware_component_init (&fw, flash, img_type, scan_offset);
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

/* W.A. for yocto not supporting openssl signatures on BMC side */
#ifndef PHASE_DEVELOPER
			return status;
#endif
		}

		/* if bootblock not found return, otherwise release from reset */
		if (img_type == IMG_BOOTBLOCK) {
			bmc_start (bmc_start_addr[0], true);
		}

		/* start search for the next BMC component address. images are all alligned to 4KB. */
		scan_offset = fw.base.start_addr + ROUND_UP (fw.size, 0x1000);
	}

	return 0;
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

	do {
		xTaskNotifyWait (pdFALSE, ULONG_MAX, &notification, portMAX_DELAY);

		if (notification > 0) {
			platform_printf (KGRN "reload BMC" NEWLINE KNRM);

			/* Do not trust BMC that SPI is still configured properly.
			 * In next phase SPI will be locked so this step can be skipped */
			tip_flash_initialize_access (FIU_MODULE_0, FIU_CS_0);

			/* Reload BMC only (without MC retraining) */
			status = tip_load_bmc_firmware (main_flash, ROT_COMBO1_ADDR, &shared_hash.base);

			if (status == 0) {
				/* mark that the reset was handled */
				tip_clear_reset_indication ();

				bmc_export_data ();

				bmc_continue ();

				/* Reenable the IRQ after complition */
				NVIC_EnableInt (NVIC_INT_46, TRUE);
			} else {
				/* fail to reload, nothing left to do but reset TIP */
				platform_reset (0);
			}
			notification = 0;
		} else {
			platform_msleep (0);
		}

	} while (1);
}

/**
 * Initialize and start the task to process bmc reset.
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

	memset (mailbox, 0, PCIMBX_RAM_SIZE);

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
