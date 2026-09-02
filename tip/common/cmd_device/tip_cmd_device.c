// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <string.h>
#include <stdlib.h>
#ifdef CMD_ENABLE_HEAP_STATS
#include "FreeRTOS.h"
#endif
#include "tip_cmd_device.h"
#include "platform_io.h"
#include "tip_boot.h"
#include "tip_utils.h"
#include "tip_log.h"
#include "tip_rom_utils.h"
#include "tip_reset.h"
#include "hal.h"
#include "Chips/npcm850/npcm850_fuse_wrapper.h"

/* stuff from TIP_ROM */
TIP_LOG_Arr_T TIP_LOG_Arr __attribute__ ((section (".log")));

/**
 * Retrieve the device unique chip identifier.
 *
 * @param device The device command handler.
 * @param buffer The output buffer to store the chip ID.
 * @param buf_len The size of the output buffer.
 *
 * @return The length of the chip ID or an error code.  Use ROT_IS_ERROR to check the return
 * value.
*/
static int tip_cmd_device_get_uuid (const struct cmd_device *device, uint8_t *buffer, size_t buf_len)
{
	if (device == NULL) {
		return CMD_DEVICE_INVALID_ARGUMENT;
	}

	uint16_t length = FUSE_WRPR_PROP_SIZE (DIE_LOCATION_PROPERTY);
	if (buf_len < length) {
		return CMD_DEVICE_UUID_BUFFER_TOO_SMALL;
	}

	memset (buffer, 0, buf_len);

	FUSE_WRPR_get (DIE_LOCATION_PROPERTY, buffer);

	platform_printf(KYEL "length of UUID is %d , Chip data: Wafer 0x%x, X 0x%x, Y 0x%x, 202%d, 2Y%02dB%03x" KNRM NEWLINE,
					length,
					buffer[0],
					buffer[1] & 0x3F,
					((buffer[1] & 0xC0) >> 6) | ((buffer[2] & 0x0F) << 2),
					(buffer[2] & 0xF0) >> 4,
					(buffer[4] & 0xFC) >> 2,
					((uint16_t)buffer[4] & 0x03) << 8 | buffer[3]);

	return length;
}

/**
 * Reset the device.
 *
 * @param device The device command handler.
 *
 * @note The next boot address for TIP is set prior to calling device reset.
 *
 * @return A error code if the device could not be reset. If the device is reset, this will not
 * return.
 */
static int tip_cmd_device_reset (const struct cmd_device *device)
{
	if (device == NULL) {
		return CMD_DEVICE_INVALID_ARGUMENT;
	}

	platform_printf(KRED NEWLINE "==========" NEWLINE "SW RESET" NEWLINE "==========" NEWLINE KNRM);

	platform_reset(0);

	/* Unreachable line if reset succeeds. */
	return CMD_DEVICE_RESET_FAILED;
}

/**
 * Retrieve the reset counter.
 *
 * @param device The device command handler.
 * @param type Reset counter type.
 * @param port The port identifier.
 * @param counter The output buffer to store the reset counter data.
 *
 * @return 0 if the reset counter was successfully retrieved or an error code.
 */
static int tip_cmd_device_get_reset_counter (const struct cmd_device *device, uint8_t type,
	uint8_t port, uint16_t *counter)
{
	if (device == NULL || counter == NULL) {
		return CMD_DEVICE_INVALID_ARGUMENT;
	}

	*counter = (uint16_t) TIP_LOG_Arr.TIP_LOG_ResetCounter;

	platform_printf ("Reset count = %d" NEWLINE, *counter);

	return 0;
}

#ifdef CMD_ENABLE_HEAP_STATS
/**
 * Retrieve the heap statistics.
 *
 * @param device The device command handler.
 * @param stats The output buffer to store the heap statistics.
 *
 * @return 0 if the heap statistics were successfully retrieved or an error code.
 */

static int tip_cmd_device_get_heap_stats (const struct cmd_device *device,
	struct cmd_device_heap_stats *heap)
{
	if (device == NULL || heap == NULL) {
		return CMD_DEVICE_INVALID_ARGUMENT;
	}

	memset (heap, 0xff, sizeof (struct cmd_device_heap_stats));

	heap->total = configTOTAL_HEAP_SIZE;
	heap->free = xPortGetFreeHeapSize ();
	heap->min_free = xPortGetMinimumEverFreeHeapSize ();

	return 0;
}
#endif

/**
 * Initialize TIP device command handler.
 *
 * @param device TIP device command handler instance.
 * @return 0 if success or an error code.
 */
int tip_cmd_device_init (struct tip_cmd_device *device)
{
	if (device == NULL) {
		return CMD_DEVICE_INVALID_ARGUMENT;
	}

	memset (device, 0, sizeof (struct tip_cmd_device));

	device->base.get_uuid = tip_cmd_device_get_uuid;
	device->base.reset = tip_cmd_device_reset;
	device->base.get_reset_counter = tip_cmd_device_get_reset_counter;
#ifdef CMD_ENABLE_HEAP_STATS
	device->base.get_heap_stats = tip_cmd_device_get_heap_stats;
#endif

	return 0;
}

/**
 * Dump configuration register of the chip.
 *
 * @return void
 */
static void tip_otp_cfg_dump (void)
{
	uint8_t otp_read[94] __attribute__ ((aligned (8)));
	uint32_t otp_cfg_addr = 0;

	FUSE_WRPR_get (REG_CFG_TABLE_PROPERTY, otp_read);
	hex_dump ((uint32_t) otp_read, sizeof (otp_read), "OTP_CFG2");

	FUSE_WRPR_get (REG_CFG_PTR1_PROPERTY, (uint8_t *) &otp_cfg_addr);

	platform_printf ("otp_ptr1  = 0x%08lx\n", otp_cfg_addr);
	platform_printf ("TIPRSTC   = 0x%08lx\n", REG_READ (TIPRSTC));
	platform_printf ("TIPRSTCLK = 0x%08lx\n", REG_READ (TIPRSTCLK));
	platform_printf ("MCR_171   = 0x%08lx\n", REG_READ (MCRn (171)));
	platform_printf ("MCR_67    = 0x%08lx\n", REG_READ (MCRn (67)));
	platform_printf ("TIP_FCFG6 = 0x%08lx\n", REG_READ (TIP_FCFG0_7 (6)));
}

/**
 * Update device OTP if needed.
 *
 * @return 0 if success or an error code.
 */
int tip_update_device_prop (void)
{
	uint8_t otp[94] __attribute__ ((aligned (8))) = {
		   /* MCR_67 */  0x03, 0x0C, 0x09, 0x80, 0xF0, 0xF2, 0x1F, 0x00, 0x01,
		   /* MCR_171 */ 0x03, 0xAC, 0x0A, 0x80, 0xF0, 0xF3, 0x1F, 0x00, 0x01,
		   /* TIPRSTC.CA35C */ 0x07, 0x50, 0x10, 0x80, 0xF0, 0x01, 0x00, 0x00, 0x00,
		   0x01, 0x00, 0x00, 0x00,
		   /* TIPRSTCLK.CA35C */ 0x03, 0x18, 0x04, 0x80, 0xF0, 0x01, 0x00, 0x00, 0x00};

	uint8_t otp_cfg_addr[2] = {0x22, 0x15};
	DEFS_STATUS status1, status2;
	int status = 0;

	/* check if required values are already set by TIP_ROM */
	if (((REG_READ (MCRn (67)) & 0x01001FF2) == 0x01001FF2) &&
		((REG_READ (MCRn (171)) & 0x01001FF3) == 0x01001FF3) &&
		(READ_REG_FIELD (TIPRSTC, WD0RCR_CA35C) == 1) &&
		(READ_REG_FIELD (TIPRSTCLK, WD0RCR_CA35C) == 1)) {

		platform_printf ("Update OTP skip" NEWLINE);
		goto otp_lock_and_return;
	}

	platform_printf ("Update OTP" NEWLINE);

	/* enable read\write from block 8 */
	CLEAR_REG_BIT (TIP_FCFG0_7 (6), 0);
	CLEAR_REG_BIT (TIP_FCFG0_7 (6), 8);

	/* configure the registers to expected value, till next PORST */
	REG_WRITE (MCRn (67), 0x01001FF2);
	REG_WRITE (MCRn (171), 0x01001FF3);
	SET_REG_FIELD (TIPRSTC, WD0RCR_CA35C, 1);
	SET_REG_FIELD (TIPRSTCLK, WD0RCR_CA35C, 1);

	/* program the OTP */
	status1 = FUSE_WRPR_set (REG_CFG_TABLE_PROPERTY, otp);
	status2 = FUSE_WRPR_set (REG_CFG_PTR1_PROPERTY, otp_cfg_addr);

	/* check status.
	 * fail to program OTP is not a show stopper. chip should still boot to uboot
	 */

	if ((status1 != DEFS_STATUS_OK) || (status2 != DEFS_STATUS_OK)) {
		platform_printf (KRED "WARNING: OTP patch not applied" KNRM NEWLINE);
		status = -1;
	}

	tip_otp_cfg_dump ();

otp_lock_and_return:
	/* disable writing to block 8, and lock */
	SET_REG_BIT (TIP_FCFG0_7 (6), 8);  /* FCFG0_7_FPRGDIS */
	SET_REG_BIT (TIP_FCFG0_7 (6), 16); /* FCFG0_7_FCFGLK */
	SET_REG_BIT (BMC_FCFG0_7 (6), 8);  /* FCFG0_7_FPRGDIS */
	SET_REG_BIT (BMC_FCFG0_7 (6), 16); /* FCFG0_7_FCFGLK */

	return status;
}

