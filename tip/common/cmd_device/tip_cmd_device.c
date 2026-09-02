// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include <string.h>
#include <stdlib.h>
#include "tip_cmd_device.h"
#include "platform_io.h"
#include "tip_boot.h"
#include "tip_utils.h"
#include "tip_log.h"
#include "tip_rom_utils.h"
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
	size_t uuid_len;

	if (device == NULL) {
		return CMD_DEVICE_INVALID_ARGUMENT;
	}

	uuid_len = (FUSE_WRPR_PROP_SIZE (DIE_LOCATION_PROPERTY) +
		FUSE_WRPR_PROP_SIZE (FINAL_TEST_SIGNATURE_PROPERTY));

	if (buf_len < uuid_len) {
		return CMD_DEVICE_UUID_BUFFER_TOO_SMALL;
	}

	memset (buffer, 0, buf_len);

	FUSE_WRPR_get (DIE_LOCATION_PROPERTY, buffer);
	FUSE_WRPR_get (FINAL_TEST_SIGNATURE_PROPERTY,
		buffer + FUSE_WRPR_PROP_SIZE (DIE_LOCATION_PROPERTY));

	platform_printf(KYEL "Chip data: Wafer 0x%x, X 0x%x, Y 0x%x, 202%d, 2Y%02dB%03x" KNRM NEWLINE,
					buffer[0],
					buffer[1] & 0x3F,
					((buffer[1] & 0xC0) >> 6) | ((buffer[2] & 0x0F) << 2),
					(buffer[2] & 0xF0) >> 4,
					(buffer[4] & 0xFC) >> 2,
					((uint16_t)buffer[4] & 0x03) << 8 | buffer[3]);

	return uuid_len;
}

/**
 * Reset the device.
 *
 * @param device The device command handler.
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

	/* Force TIP ROM to scan from the beginning of active flash after a new FW update */
	tip_select_next_boot_image (0);

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

	return 0;
}
