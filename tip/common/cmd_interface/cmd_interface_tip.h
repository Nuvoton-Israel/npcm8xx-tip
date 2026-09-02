// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef CMD_INTERFACE_TIP_H_
#define CMD_INTERFACE_TIP_H_

#include <stdint.h>
#include <stdbool.h>
#include "attestation/attestation_responder.h"
#include "cmd_interface/cmd_interface.h"
#include "cmd_interface/device_manager.h"
#include "cmd_interface/session_manager.h"
#include "cmd_interface/cmd_background.h"
#include "crypto/hash.h"
#include "firmware/firmware_update_control.h"
#include "attestation/pcr_store.h"
#include "riot/riot_key_manager.h"
#include "cmd_interface/cmd_device.h"


/**
 * TIP Command interface for processing received requests from system.
 */
struct cmd_interface_tip {
	struct cmd_interface base;								/**< Base command interface */
	struct firmware_update_control *control;				/**< FW update control instance */
	struct cmd_background *background;						/**< Context for completing background commands */
	struct pcr_store *pcr_store;							/**< PCR storage */
	struct riot_key_manager *riot;							/**< RIoT key manager */
	struct attestation_responder *attestation;				/**< Attestation responder instance */
	struct hash_engine *hash;								/**< The hashing engine for PCR operations. */
	struct cmd_interface_fw_version *fw_version;			/**< FW version numbers */
	struct device_manager *device_manager;					/**< Device manager instance */
	struct cmd_device *cmd_device;							/**< Device command handler instance */
	struct cmd_interface_device_id device_id;				/**< Device ID information */
};


int cmd_interface_tip_init (struct cmd_interface_tip *intf,
    struct firmware_update_control *control, struct attestation_responder *attestation,
    struct device_manager *device_manager, struct pcr_store *store, struct hash_engine *hash,
    struct cmd_background *background, struct cmd_interface_fw_version *fw_version,
    struct riot_key_manager *riot, struct cmd_device *cmd_device, uint16_t vendor_id,
    uint16_t device_id, uint16_t subsystem_vid, uint16_t subsystem_id,
    struct session_manager *session);
void cmd_interface_tip_deinit (struct cmd_interface_tip *intf);

#endif /* CMD_INTERFACE_TIP_H_ */
