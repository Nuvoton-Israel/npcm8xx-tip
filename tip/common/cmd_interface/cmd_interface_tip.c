// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "cmd_interface/cmd_logging.h"
#include "cmd_interface/cmd_interface.h"
#include "cmd_interface/cerberus_protocol.h"
#include "cmd_interface/cerberus_protocol_required_commands.h"
#include "cmd_interface/cerberus_protocol_optional_commands.h"
#include "cmd_interface/cerberus_protocol_master_commands.h"
#include "cmd_interface/cerberus_protocol_debug_commands.h"
#include "cmd_interface_tip.h"


static int cmd_interface_tip_process_request (struct cmd_interface *intf,
	struct cmd_interface_msg *request)
{
	struct cmd_interface_tip *tip_intf = (struct cmd_interface_tip*) intf;
	uint8_t command_id;
	uint8_t command_set;
	int status;

	status = cmd_interface_process_cerberus_protocol_message (&tip_intf->base, request, &command_id,
		&command_set, true, true);
	if (status != 0) {
		return status;
	}

	switch (command_id) {
		case CERBERUS_PROTOCOL_GET_FW_VERSION:
			status = cerberus_protocol_get_fw_version (tip_intf->fw_version, request);
			break;

		case CERBERUS_PROTOCOL_GET_DIGEST:
			status = cerberus_protocol_get_certificate_digest (tip_intf->attestation,
				tip_intf->base.session, request);
			break;

		case CERBERUS_PROTOCOL_GET_CERTIFICATE:
			status = cerberus_protocol_get_certificate (tip_intf->attestation, request);
			break;

		case CERBERUS_PROTOCOL_ATTESTATION_CHALLENGE:
			status = cerberus_protocol_get_challenge_response (tip_intf->attestation,
				tip_intf->base.session, request);
			break;

        case CERBERUS_PROTOCOL_GET_LOG_INFO:
			status = cerberus_protocol_get_log_info (tip_intf->pcr_store, request);
			break;

		case CERBERUS_PROTOCOL_READ_LOG:
			status = cerberus_protocol_log_read (tip_intf->pcr_store, tip_intf->hash, request);
			break;

		case CERBERUS_PROTOCOL_CLEAR_LOG:
			status = cerberus_protocol_log_clear (tip_intf->background, request);
            break;

        case CERBERUS_PROTOCOL_INIT_FW_UPDATE:
			status = cerberus_protocol_fw_update_init (tip_intf->control, request);
			break;

		case CERBERUS_PROTOCOL_FW_UPDATE:
			status = cerberus_protocol_fw_update (tip_intf->control, request);
			break;

		case CERBERUS_PROTOCOL_COMPLETE_FW_UPDATE:
			status = cerberus_protocol_fw_update_start (tip_intf->control, request);
			break;

		case CERBERUS_PROTOCOL_GET_UPDATE_STATUS: {
			const struct manifest_cmd_interface *pfm_cmd[2] = { NULL };
			struct host_processor *host[2] = { NULL };
			status = cerberus_protocol_get_update_status (tip_intf->control, 2, pfm_cmd,
				NULL, NULL, host, NULL, NULL, tip_intf->background, request);
			break;
		}

		case CERBERUS_PROTOCOL_GET_EXT_UPDATE_STATUS:
			status = cerberus_protocol_get_extended_update_status (tip_intf->control, NULL, NULL,
				NULL, NULL, request);
			break;

		case CERBERUS_PROTOCOL_GET_DEVICE_CAPABILITIES:
			status = cerberus_protocol_get_device_capabilities (tip_intf->device_manager,
				request);
			break;

		case CERBERUS_PROTOCOL_EXPORT_CSR:
			status = cerberus_protocol_export_csr (tip_intf->riot, request);
			break;

		case CERBERUS_PROTOCOL_IMPORT_CA_SIGNED_CERT:
			status = cerberus_protocol_import_ca_signed_cert (tip_intf->riot,
				tip_intf->background, request);
			break;

		case CERBERUS_PROTOCOL_GET_SIGNED_CERT_STATE:
			status = cerberus_protocol_get_signed_cert_state (tip_intf->background, request);
			break;

		case CERBERUS_PROTOCOL_GET_DEVICE_INFO:
			status = cerberus_protocol_get_device_info (tip_intf->cmd_device, request);
			break;

		case CERBERUS_PROTOCOL_GET_DEVICE_ID:
			status = cerberus_protocol_get_device_id (&tip_intf->device_id, request);
			break;

		case CERBERUS_PROTOCOL_RESET_COUNTER:
			status = cerberus_protocol_reset_counter (tip_intf->cmd_device, request);
			break;

        case CERBERUS_PROTOCOL_UNSEAL_MESSAGE:
            status = cerberus_protocol_unseal_message (tip_intf->background, request);
            break;

		case CERBERUS_PROTOCOL_UNSEAL_MESSAGE_RESULT:
			status = cerberus_protocol_unseal_message_result (tip_intf->background, request);
			break;

		case CERBERUS_PROTOCOL_GET_ATTESTATION_DATA:
			status = cerberus_protocol_get_attestation_data (tip_intf->pcr_store, request);
			break;

#ifdef CMD_SUPPORT_ENCRYPTED_SESSIONS
		case CERBERUS_PROTOCOL_EXCHANGE_KEYS:
			status = cerberus_protocol_key_exchange (tip_intf->base.session, request,
				intf->curr_txn_encrypted);
			break;

		case CERBERUS_PROTOCOL_SESSION_SYNC:
			status = cerberus_protocol_session_sync (tip_intf->base.session, request,
				intf->curr_txn_encrypted);
			break;
#endif
		default:
			return CMD_HANDLER_UNKNOWN_REQUEST;
	}

	if (status == 0) {
		status = cmd_interface_prepare_response (&tip_intf->base, request);
	}

	return status;
}

#ifdef CMD_ENABLE_ISSUE_REQUEST
static int cmd_interface_tip_process_response (struct cmd_interface *intf,
	struct cmd_interface_msg *response)
{
	return CMD_HANDLER_UNSUPPORTED_OPERATION;
}
#endif

/**
 * Initialize System command interface instance
 *
 * @param intf The System command interface instance to initialize
 * @param control The FW update control instance to use
 * @param attestation Slave attestation manager
 * @param device_manager Device manager
 * @param store PCR storage
 * @param hash Hash engine to to use for PCR operations
 * @param background Context for executing long-running operations in the background.
 * @param fw_version The FW version strings
 * @param riot RIoT keys manager
 * @param cmd_device Device command handler instance
 * @param vendor_id Device vendor ID
 * @param device_id Device ID
 * @param subsystem_vid Subsystem vendor ID
 * @param subsystem_id Subsystem ID
 * @param session Session manager for channel encryption
 *
 * @return Initialization status, 0 if success or an error code.
 */
int cmd_interface_tip_init (struct cmd_interface_tip *intf,
    struct firmware_update_control *control, struct attestation_responder *attestation,
    struct device_manager *device_manager, struct pcr_store *store, struct hash_engine *hash,
    struct cmd_background *background, struct cmd_interface_fw_version *fw_version,
    struct riot_key_manager *riot, struct cmd_device *cmd_device, uint16_t vendor_id,
    uint16_t device_id, uint16_t subsystem_vid, uint16_t subsystem_id,
    struct session_manager *session)
{
	if ((intf == NULL) || (control == NULL) || (store == NULL) || (hash == NULL) ||
        (background == NULL) || (riot == NULL) || (attestation == NULL) ||
		(device_manager == NULL) || (fw_version == NULL) || (cmd_device == NULL)) {
		return CMD_HANDLER_INVALID_ARGUMENT;
	}

	memset (intf, 0, sizeof (struct cmd_interface_tip));

	intf->control = control;
	intf->riot = riot;
	intf->background = background;
	intf->attestation = attestation;
	intf->device_manager = device_manager;
	intf->fw_version = fw_version;
	intf->cmd_device = cmd_device;
	intf->pcr_store = store;
	intf->hash = hash;
	intf->device_id.vendor_id = vendor_id;
	intf->device_id.device_id = device_id;
	intf->device_id.subsystem_vid = subsystem_vid;
	intf->device_id.subsystem_id = subsystem_id;

	intf->base.process_request = cmd_interface_tip_process_request;
#ifdef CMD_ENABLE_ISSUE_REQUEST
	intf->base.process_response = cmd_interface_tip_process_response;
#endif
	intf->base.generate_error_packet = cmd_interface_generate_error_packet;

#ifdef CMD_SUPPORT_ENCRYPTED_SESSIONS
	intf->base.session = session;
#endif

	return 0;
}

/**
 * Deinitialize slave system command interface instance
 *
 * @param intf The slave system command interface instance to deinitialize
 */
void cmd_interface_tip_deinit (struct cmd_interface_tip *intf)
{
	if (intf != NULL) {
		memset (intf, 0, sizeof (struct cmd_interface_tip));
	}
}
