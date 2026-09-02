/*----------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *   tip_mbx.c
 *            This file contains the mailbox handlers
 *  Project:  Arbel
 *------------------------------------------------------------------------*/

#include <string.h>
#include "tip_mbx.h"
#include "platform_api.h"
#include "hal.h"
#include "hal_regs.h"
#include "platform_io.h"

/*
 * Indicate that first packet was reecived.
 * Upon receiveing the first packet need to adjust the uart
 */

static int b_first_packet;

/**
 * Clear a notification. This is with a retry to workaround a FW issue.
 *
 * @param notification  Notification value to clear
 */
void tip_mbx_clear_notification (uint32_t notification)
{
	uint32_t		  retry = 10;
	volatile uint32_t verifyNotification = 0;

	while (retry > 0) {
		REG_WRITE (B2TIPST0, LSW (notification));
		REG_WRITE (B2TIPST1, MSW (notification));

		verifyNotification = MAKE32 (REG_READ (B2TIPST0), REG_READ (B2TIPST1));

		if ((verifyNotification & notification) != 0) {
			notification &= verifyNotification;
			retry--;
		} else {
			break;
		}
	}
}

/**
 * TIP sends a notification to the BMC.
 *
 * @param bit Notification value to send.
 * 		  Normally this value would be a single bit field.
 *        For example: value 0x2L means waiting for the second channel.
 */
void tip_mbx_notify_to_bmc (uint32_t notify)
{
	uint16_t low = LSW (notify);
	uint16_t high = MSW (notify);

	if (low)
		REG_WRITE (TIP2BNT0, low);

	if (high)
		REG_WRITE (TIP2BNT1, high);
}

/**
 * TIP waits for notification from BMC by polling on notification status register.
 *
 *  @param  notify: notification value to wait for.
 *          Normally this value would be a single bit field.
 *          For example: value 0x2L means waiting for the second channel.
 */
void tip_mbx_wait_for_ack (uint32_t notify)
{
	uint16_t low = LSW (notify);
	uint16_t high = MSW (notify);

	if (low)
		while(REG_READ(TIP2BNT0) & low);

	if (high)
		while(REG_READ(TIP2BNT1) & high);
}

/**
 * Receive a command packet from a communication channel.  This call will block until a packet
 * has been received or the timeout has expired.
 *
 * @param channel The channel to receive a packet from.
 * @param packet Output for the packet data being received.
 * @param ms_timeout The amount of time to wait for a received packet, in milliseconds.
 * A negative value will wait forever, and a value of 0 will return immediately.
 *
 * @return 0 if a packet was successfully received or an error code.
 */
int tip_receive_packet (struct cmd_channel *channel, struct cmd_packet *packet, int ms_timeout)
{
	struct tip_cmd_channel *tip_cmd_cnl = (struct tip_cmd_channel *) channel;
	uint32_t				notification;
	uint32_t				header = 0;
	uint16_t				low;
	uint16_t				high;

	if (tip_cmd_cnl == NULL || packet == NULL) {
		return CMD_CHANNEL_INVALID_ARGUMENT;
	}

	notification = tip_cmd_cnl->notification_idx;
	low = LSW (notification);
	high = MSW (notification);

	memset (packet, 0, sizeof (struct cmd_packet));

	/* This is blocking, but we can a also respond to interrupt instead: */
	if (low) {
		while ((REG_READ (B2TIPST0) & low) == 0) {
			platform_msleep (0);
		}
	}

	if (high) {
		while ((REG_READ (B2TIPST1) & high) == 0) {
			platform_msleep (0);
		}
	}

	if (b_first_packet < 5) {
		b_first_packet++;
		serial_printf_reconfig ();
	}

	platform_printf_dbg ("mbx rcv: notification=%#010lx  ST0=%#010lx ST1=%#010lx" NEWLINE,
						 notification, REG_READ (B2TIPST0), REG_READ (B2TIPST1));

	/* TIP parses header after receiving notification */
	header = *(uint32_t *) tip_cmd_cnl->win_addr;
	packet->pkt_size = (uint16_t) ((header & 0xFF00) >> 8) + MCTP_BASE_PROTOCOL_SMBUS_OVERHEAD;
	packet->dest_addr = CERBERUS_SLAVE_ADDR;

	if (packet->pkt_size > CMD_MAX_PACKET_SIZE)
		packet->state = CMD_OVERFLOW_PACKET;
	else
		packet->state = CMD_VALID_PACKET;

	memcpy (packet->data, (uint8_t *) tip_cmd_cnl->win_addr, packet->pkt_size);


	tip_mbx_clear_notification (notification);

	platform_printf_dbg ("mbx rcv done: notification=%#010lx  ST0=%#010lx ST1=%#010lx" NEWLINE,
						 notification, REG_READ (B2TIPST0), REG_READ (B2TIPST1));

	return 0;
}

/**
 * Send a command packet over a communication channel.
 *
 * Returning from this function does not guarantee the packet has been fully transmitted.
 * Depending on the channel implementation, it is possible the packet is still in flight with
 * the data buffered in the channel driver.
 *
 * @param channel The channel to send a packet on.
 * @param packet The packet to send.
 *
 * @return 0 if the the packet was successfully sent or an error code.
 */
int tip_send_packet (struct cmd_channel *channel, struct cmd_packet *packet)
{
	struct tip_cmd_channel *tip_cmd_cnl = (struct tip_cmd_channel *) channel;

	if (packet == NULL) {
		return CMD_CHANNEL_INVALID_ARGUMENT;
	}

	/* 1st half of the channel is for TIP read, 2nd half is for TIP write */
	uint32_t wr_win_addr = tip_cmd_cnl->win_addr + (tip_cmd_cnl->win_size >> 1);

	memcpy ((uint8_t *) wr_win_addr, packet->data, packet->pkt_size);

	platform_printf_dbg (NEWLINE "mbx send: msg: %s" NEWLINE, (char *) packet->data);

	tip_mbx_notify_to_bmc (tip_cmd_cnl->notification_idx);

	tip_mbx_wait_for_ack (tip_cmd_cnl->notification_idx);

	return 0;
}

/**
 *  Initialize a channel (mailbox) to communicate with BMC
 *
 *  @param channel     channel handler
 *  @param id          channel index, equivalent to bit in the notification register.
 *  @param addr        base address for mailbox window
 *  @param size        size of mailbox window
 *  @return 0  if pass or an error code.
 */
int tip_cmd_channel_init (struct tip_cmd_channel *channel, int id, uint32_t addr, uint32_t size)
{
	int status;

	if (channel == NULL) {
		return CMD_CHANNEL_INVALID_ARGUMENT;
	}

	memset (channel, 0, sizeof (struct tip_cmd_channel));

	status = cmd_channel_init (&channel->base, id);
	if (status != 0) {
		return status;
	}

	channel->win_addr = addr;
	channel->win_size = size;
	channel->notification_idx = NOTIFICATION_BIT_16;
	channel->base.send_packet = tip_send_packet;
	channel->base.receive_packet = tip_receive_packet;

	b_first_packet = 0;

	return 0;
}
