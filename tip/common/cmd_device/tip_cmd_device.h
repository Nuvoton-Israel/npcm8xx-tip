// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_CMD_DEVICE_H_
#define TIP_CMD_DEVICE_H_

#include "cmd_interface/cmd_device.h"

/**
 * The max len of device UUID
 */
#define TIP_UID_LEN  16

/**
 * TIP device command handler.
 */
struct tip_cmd_device {
	struct cmd_device base; /**< Device command handler instance */
};


int tip_cmd_device_init (struct tip_cmd_device *cmd);


#endif /* TIP_CMD_DEVICE_H_ */
