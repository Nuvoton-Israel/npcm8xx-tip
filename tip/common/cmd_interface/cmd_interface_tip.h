/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

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
#include "common/observable.h"
#include "cmd_interface/cerberus_protocol_observer.h"


/**
 * TIP Command interface for processing received requests from system.
 */
struct cmd_interface_tip {
	struct cmd_interface base;								/**< Base command interface */
	struct firmware_update_control *control;				/**< FW update control instance */
	struct cmd_background *background;						/**< Context for completing background commands */
	struct pcr_store *pcr_store;							/**< PCR storage */
	struct riot_key_manager *riot;							/**< RIoT key manager */
	struct cmd_authorization *auth;							/**< Authorization handler */
	struct attestation_responder *attestation;				/**< Attestation responder instance */
	struct hash_engine *hash;								/**< The hashing engine for PCR operations. */
	struct cmd_interface_fw_version *fw_version;			/**< FW version numbers */
	struct device_manager *device_manager;					/**< Device manager instance */
	struct cmd_device *cmd_device;							/**< Device command handler instance */
	struct cmd_interface_device_id device_id;				/**< Device ID information */
	struct manifest_cmd_interface *cfm;						/**< CFM update command interface instance */
	struct manifest_cmd_interface *pcd;						/**< PCD update command interface instance */
	struct cfm_manager *cfm_manager;						/**< CFM manager instance */
	struct pcd_manager *pcd_manager;						/**< PCD manager instance */
	struct observable observable;							/**< Observer manager for the interface. */
};

int cmd_interface_tip_init (struct cmd_interface_tip *intf, struct firmware_update_control *control,
	struct attestation_responder *attestation, struct device_manager *device_manager,
	struct pcr_store *store, struct hash_engine *hash, struct cmd_background *background,
	struct cmd_interface_fw_version *fw_version, struct riot_key_manager *riot,
	struct cmd_authorization *auth, struct cmd_device *cmd_device,
	struct manifest_cmd_interface *pcd, struct pcd_manager *pcd_manager,
	struct manifest_cmd_interface *cfm, struct cfm_manager *cfm_manager, uint16_t vendor_id,
	uint16_t device_id, uint16_t subsystem_vid, uint16_t subsystem_id,
	struct session_manager *session);
void cmd_interface_tip_deinit (struct cmd_interface_tip *intf);
int cmd_interface_tip_add_cerberus_protocol_observer (struct cmd_interface_tip *intf,
	struct cerberus_protocol_observer *observer);
int cmd_interface_tip_remove_cerberus_protocol_observer (struct cmd_interface_tip *intf,
	struct cerberus_protocol_observer *observer);


#endif /* CMD_INTERFACE_TIP_H_ */
