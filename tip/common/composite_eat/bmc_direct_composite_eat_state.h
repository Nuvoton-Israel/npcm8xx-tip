/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef BMC_DIRECT_COMPOSITE_EAT_STATE_H_
#define BMC_DIRECT_COMPOSITE_EAT_STATE_H_

#include <stdbool.h>
#include <stdint.h>

#include "composite_eat/bmc_direct_composite_eat_abi.h"
#include "composite_eat/composite_eat_generator.h"


struct bmc_direct_composite_eat_request {
	uint32_t id;
	uint32_t epoch;
	uint32_t request_address;
	uint32_t request_length;
	uint32_t response_address;
	uint32_t response_capacity;
};

struct bmc_direct_composite_eat_state {
	volatile bool active;
	uint32_t epoch;
	uint32_t last_request_id;
	struct bmc_direct_composite_eat_request pending;
};

struct bmc_direct_composite_eat_publication_ops {
	void (*write_scratchpad) (void *context, uint32_t index, uint32_t value);
	void (*memory_barrier) (void *context);
	void (*clear_notification) (void *context, uint32_t notification);
	void (*notify_bmc) (void *context, uint32_t notification);
};

bool bmc_direct_composite_eat_buffers_valid (uint32_t request_address, uint32_t request_length,
	uint32_t response_address, uint32_t response_capacity);

bool bmc_direct_composite_eat_begin (struct bmc_direct_composite_eat_state *state,
	const struct bmc_direct_composite_eat_request *request);

bool bmc_direct_composite_eat_request_current (const struct bmc_direct_composite_eat_state *state,
	const struct bmc_direct_composite_eat_request *request);

void bmc_direct_composite_eat_complete (struct bmc_direct_composite_eat_state *state);
void bmc_direct_composite_eat_reset (struct bmc_direct_composite_eat_state *state);

bool bmc_direct_composite_eat_publish_response (
	const struct bmc_direct_composite_eat_publication_ops *ops, void *context, uint32_t status,
	uint32_t response_length, uint32_t request_id);

enum bmc_direct_composite_eat_status bmc_direct_composite_eat_map_generator_status (
	enum composite_eat_generator_status status);


#endif /* BMC_DIRECT_COMPOSITE_EAT_STATE_H_ */