/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "composite_eat/bmc_direct_composite_eat_abi.h"
#include "composite_eat/bmc_direct_composite_eat_state.h"


static bool bmc_direct_composite_eat_range_valid (uint32_t address, uint32_t length, uint32_t base,
	uint32_t size)
{
	uint64_t end = (uint64_t) address + length;
	uint64_t limit = (uint64_t) base + size;

	return (length != 0) && (address >= base) && (end <= limit);
}

bool bmc_direct_composite_eat_buffers_valid (uint32_t request_address, uint32_t request_length,
	uint32_t response_address, uint32_t response_capacity)
{
	uint32_t request_base = BMC_DIRECT_COMPOSITE_EAT_SHM_BASE + BMC_DIRECT_COMPOSITE_EAT_REQ_OFFSET;
	uint32_t response_base =
		BMC_DIRECT_COMPOSITE_EAT_SHM_BASE + BMC_DIRECT_COMPOSITE_EAT_RESP_OFFSET;
	uint64_t request_end = (uint64_t) request_address + request_length;
	uint64_t response_end = (uint64_t) response_address + response_capacity;

	return bmc_direct_composite_eat_range_valid (request_address, request_length, request_base,
			   BMC_DIRECT_COMPOSITE_EAT_REQ_SIZE) &&
		bmc_direct_composite_eat_range_valid (response_address, response_capacity, response_base,
			BMC_DIRECT_COMPOSITE_EAT_RESP_SIZE) &&
		((request_end <= response_address) || (response_end <= request_address));
}

bool bmc_direct_composite_eat_begin (struct bmc_direct_composite_eat_state *state,
	const struct bmc_direct_composite_eat_request *request)
{
	if ((state == NULL) || (request == NULL) || state->active || (request->id == 0) ||
		(request->id == state->last_request_id) ||
		!bmc_direct_composite_eat_buffers_valid (request->request_address, request->request_length,
			request->response_address, request->response_capacity)) {
		return false;
	}

	state->pending = *request;
	state->pending.epoch = state->epoch;
	state->active = true;
	return true;
}

bool bmc_direct_composite_eat_request_current (const struct bmc_direct_composite_eat_state *state,
	const struct bmc_direct_composite_eat_request *request)
{
	return (state != NULL) && (request != NULL) && state->active &&
		(request->epoch == state->epoch) && (request->id == state->pending.id);
}

void bmc_direct_composite_eat_complete (struct bmc_direct_composite_eat_state *state)
{
	if (state != NULL) {
		state->last_request_id = state->pending.id;
		state->active = false;
	}
}

void bmc_direct_composite_eat_reset (struct bmc_direct_composite_eat_state *state)
{
	if (state != NULL) {
		state->epoch++;
		memset (&state->pending, 0, sizeof (state->pending));
		state->last_request_id = 0;
		state->active = false;
	}
}

bool bmc_direct_composite_eat_publish_response (
	const struct bmc_direct_composite_eat_publication_ops *ops, void *context, uint32_t status,
	uint32_t response_length, uint32_t request_id)
{
	if ((ops == NULL) || (ops->write_scratchpad == NULL) || (ops->memory_barrier == NULL) ||
		(ops->clear_notification == NULL) || (ops->notify_bmc == NULL)) {
		return false;
	}

	ops->write_scratchpad (context, BMC_DIRECT_COMPOSITE_EAT_SCRPAD_COMMAND, status);
	ops->write_scratchpad (context, BMC_DIRECT_COMPOSITE_EAT_SCRPAD_RESP_LEN, response_length);
	ops->write_scratchpad (context, BMC_DIRECT_COMPOSITE_EAT_SCRPAD_RESPONSE_ID, request_id);
	ops->memory_barrier (context);
	ops->clear_notification (context, BMC_DIRECT_NOTIFICATION_COMPOSITE_EAT);
	ops->notify_bmc (context, BMC_DIRECT_NOTIFICATION_COMPOSITE_EAT);
	return true;
}

enum bmc_direct_composite_eat_status bmc_direct_composite_eat_map_generator_status (
	enum composite_eat_generator_status status)
{
	switch (status) {
		case COMPOSITE_EAT_GENERATOR_OK:
			return BMC_DIRECT_COMPOSITE_EAT_OK;
		case COMPOSITE_EAT_GENERATOR_BAD_VERSION:
			return BMC_DIRECT_COMPOSITE_EAT_BAD_VERSION;
		case COMPOSITE_EAT_GENERATOR_BAD_REQUEST:
			return BMC_DIRECT_COMPOSITE_EAT_BAD_REQUEST;
		case COMPOSITE_EAT_GENERATOR_TOO_MANY_RECORDS:
			return BMC_DIRECT_COMPOSITE_EAT_TOO_MANY_RECORDS;
		case COMPOSITE_EAT_GENERATOR_BUFFER_TOO_SMALL:
			return BMC_DIRECT_COMPOSITE_EAT_RESPONSE_TOO_SMALL;
		case COMPOSITE_EAT_GENERATOR_SIGN_ERROR:
			return BMC_DIRECT_COMPOSITE_EAT_SIGN_FAILED;
		case COMPOSITE_EAT_GENERATOR_BUSY:
			return BMC_DIRECT_COMPOSITE_EAT_BUSY;
		default:
			return BMC_DIRECT_COMPOSITE_EAT_INTERNAL;
	}
}