/*------------------------------------------------------------------------
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2022 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 * ---------------------------------------------------------------------
 * File Contents:
 *   tip_mbx.h
 *            This file contains the mailbox handlers
 *  Project:  Arbel
 *------------------------------------------------------------------------*/

#ifndef TIP_MBX_H_
#define TIP_MBX_H_

#include "cmd_interface/cmd_channel.h"


#define CERBERUS_SLAVE_ADDR 	0x41

/**
* Notification target bit.
*/
enum notification_bit {
	NOTIFICATION_BIT_0 = 1 << 0,   /**< Notification from\to bootblock core 0 */
	NOTIFICATION_BIT_1 = 1 << 1,   /**< Notification from\to bootblock core 1 */
	NOTIFICATION_BIT_2 = 1 << 2,   /**< Notification from\to bootblock core 2 */
	NOTIFICATION_BIT_3 = 1 << 3,   /**< Notification from\to bootblock core 3 */
	NOTIFICATION_BIT_4 = 1 << 4,   /**< Notification from\to BL31 core 0 */
	NOTIFICATION_BIT_5 = 1 << 5,   /**< Notification from\to BL31 core 1 */
	NOTIFICATION_BIT_6 = 1 << 6,   /**< Notification from\to BL31 core 2 */
	NOTIFICATION_BIT_7 = 1 << 7,   /**< Notification from\to BL31 core 3 */
	NOTIFICATION_BIT_8 = 1 << 8,   /**< Notification from\to OPTEE core 0 */
	NOTIFICATION_BIT_9 = 1 << 9,   /**< Notification from\to OPTEE core 1 */
	NOTIFICATION_BIT_10 = 1 << 10, /**< Notification from\to OPTEE core 2 */
	NOTIFICATION_BIT_11 = 1 << 11, /**< Notification from\to OPTEE core 3 */
	NOTIFICATION_BIT_12 = 1 << 12, /**< Notification from\to UBOOT core 0 */
	NOTIFICATION_BIT_13 = 1 << 13, /**< Notification from\to UBOOT core 1 */
	NOTIFICATION_BIT_14 = 1 << 14, /**< Notification from\to UBOOT core 2 */
	NOTIFICATION_BIT_15 = 1 << 15, /**< Notification from\to UBOOT core 3 */
	NOTIFICATION_BIT_16 = 1 << 16, /**< Notification from\to LINUX core 0 */
	NOTIFICATION_BIT_17 = 1 << 17, /**< Notification from\to LINUX core 1 */
	NOTIFICATION_BIT_18 = 1 << 18, /**< Notification from\to LINUX core 2 */
	NOTIFICATION_BIT_19 = 1 << 19, /**< Notification from\to LINUX core 3 */
	NOTIFICATION_BIT_20 = 1 << 20, /**< Notification from\to OPENBMC  core 0 */
	NOTIFICATION_BIT_21 = 1 << 21, /**< Notification from\to OPENBMC  core 1 */
	NOTIFICATION_BIT_22 = 1 << 22, /**< Notification from\to OPENBMC  core 2 */
	NOTIFICATION_BIT_23 = 1 << 23, /**< Notification from\to OPENBMC  core 3 */
	NOTIFICATION_BIT_24 = 1 << 24, /**< Notification from\to none secure  core 0 */
	NOTIFICATION_BIT_25 = 1 << 25, /**< Notification from\to none secure  core 1 */
	NOTIFICATION_BIT_26 = 1 << 26, /**< Notification from\to none secure  core 2 */
	NOTIFICATION_BIT_27 = 1 << 27, /**< Notification from\to none secure  core 3 */
	NOTIFICATION_BIT_28 = 1 << 28, /**< Notification from\to secure  core 0 */
	NOTIFICATION_BIT_29 = 1 << 29, /**< Notification from\to secure  core 1 */
	NOTIFICATION_BIT_30 = 1 << 30, /**< Notification from\to secure  core 2 */
	NOTIFICATION_BIT_31 = 1 << 31, /**< Notification from\to secure  core 3 */
};

/**
 * TIP command channel
 */
struct tip_cmd_channel {
	struct cmd_channel base;
	uint32_t notification_idx; 	/**< bit idx in B2TIP \ TIP2BB regs used for signaling. */
	uint32_t win_addr;		   	/**< fixed per channel */
	uint32_t win_size;		   	/**< fixed per channel */
	bool secure;			   	/**< lock the window. */
};


void tip_mbx_clear_notification (uint32_t notification);
void tip_mbx_notify_to_bmc (uint32_t notify);
void tip_mbx_wait_for_ack (uint32_t notify);
int tip_receive_packet (struct cmd_channel *channel, struct cmd_packet *packet, int ms_timeout);
int tip_send_packet (struct cmd_channel *channel, struct cmd_packet *packet);
int tip_cmd_channel_init (struct tip_cmd_channel *channel, int id, uint32_t addr, uint32_t size);


#endif /* TIP_MBX_H_ */