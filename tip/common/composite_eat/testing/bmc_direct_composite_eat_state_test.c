/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "testing.h"
#include "composite_eat/bmc_direct_composite_eat_abi.h"
#include "composite_eat/bmc_direct_composite_eat_state.h"


TEST_SUITE_LABEL ("bmc_direct_composite_eat_state");

struct publication_log {
	uint32_t event[6];
	uint32_t index[6];
	uint32_t value[6];
	size_t count;
};

static void publication_write (void *context, uint32_t index, uint32_t value)
{
	struct publication_log *log = context;

	log->event[log->count] = 1;
	log->index[log->count] = index;
	log->value[log->count++] = value;
}

static void publication_barrier (void *context)
{
	struct publication_log *log = context;

	log->event[log->count++] = 2;
}

static void publication_clear (void *context, uint32_t notification)
{
	struct publication_log *log = context;

	log->event[log->count] = 3;
	log->value[log->count++] = notification;
}

static void publication_notify (void *context, uint32_t notification)
{
	struct publication_log *log = context;

	log->event[log->count] = 4;
	log->value[log->count++] = notification;
}


static void bmc_direct_composite_eat_state_test_buffers (CuTest *test)
{
	uint32_t request_base = BMC_DIRECT_COMPOSITE_EAT_SHM_BASE;
	uint32_t response_base = request_base + BMC_DIRECT_COMPOSITE_EAT_RESP_OFFSET;

	TEST_START;
	CuAssertTrue (test, bmc_direct_composite_eat_buffers_valid (request_base, 1, response_base, 1));
	CuAssertTrue (test,
		bmc_direct_composite_eat_buffers_valid (request_base + BMC_DIRECT_COMPOSITE_EAT_REQ_SIZE -
				1,
			1, response_base + BMC_DIRECT_COMPOSITE_EAT_RESP_SIZE - 1, 1));
	CuAssertTrue (test,
		!bmc_direct_composite_eat_buffers_valid (request_base, 0, response_base, 1));
	CuAssertTrue (test,
		!bmc_direct_composite_eat_buffers_valid (request_base, 1, response_base, 0));
	CuAssertTrue (test,
		!bmc_direct_composite_eat_buffers_valid (request_base - 1, 1, response_base, 1));
	CuAssertTrue (test,
		!bmc_direct_composite_eat_buffers_valid (request_base + BMC_DIRECT_COMPOSITE_EAT_REQ_SIZE -
				1,
			2, response_base, 1));
	CuAssertTrue (test,
		!bmc_direct_composite_eat_buffers_valid (response_base, 1, response_base, 1));
	CuAssertTrue (test,
		!bmc_direct_composite_eat_buffers_valid (0xfffffff0u, 0x100, response_base, 1));
}

static void bmc_direct_composite_eat_state_test_ownership (CuTest *test)
{
	struct bmc_direct_composite_eat_state state = {0};
	struct bmc_direct_composite_eat_request first = {
		.id = 1,
		.request_address = BMC_DIRECT_COMPOSITE_EAT_SHM_BASE,
		.request_length = 128,
		.response_address =
			BMC_DIRECT_COMPOSITE_EAT_SHM_BASE + BMC_DIRECT_COMPOSITE_EAT_RESP_OFFSET,
		.response_capacity = 1024,
	};
	struct bmc_direct_composite_eat_request second = first;

	TEST_START;
	second.id = 2;
	CuAssertTrue (test, bmc_direct_composite_eat_begin (&state, &first));
	first = state.pending;
	CuAssertTrue (test, bmc_direct_composite_eat_request_current (&state, &first));
	first.request_address = 0;
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_SHM_BASE, state.pending.request_address);
	CuAssertTrue (test, !bmc_direct_composite_eat_begin (&state, &second));
	CuAssertIntEquals (test, 1, state.pending.id);
	bmc_direct_composite_eat_complete (&state);
	first.request_address = BMC_DIRECT_COMPOSITE_EAT_SHM_BASE;
	CuAssertTrue (test, !bmc_direct_composite_eat_begin (&state, &first));
	CuAssertTrue (test, bmc_direct_composite_eat_begin (&state, &second));
	CuAssertIntEquals (test, 2, state.pending.id);
	bmc_direct_composite_eat_reset (&state);
	CuAssertTrue (test, !bmc_direct_composite_eat_request_current (&state, &second));
	CuAssertTrue (test, !state.active);
	CuAssertIntEquals (test, 0, state.pending.id);
	CuAssertIntEquals (test, 0, state.last_request_id);
}

static void bmc_direct_composite_eat_state_test_invalid_request (CuTest *test)
{
	struct bmc_direct_composite_eat_state state = {0};
	struct bmc_direct_composite_eat_request request = {
		.id = 1,
		.request_address = 0,
		.request_length = 128,
		.response_address =
			BMC_DIRECT_COMPOSITE_EAT_SHM_BASE + BMC_DIRECT_COMPOSITE_EAT_RESP_OFFSET,
		.response_capacity = 1024,
	};

	TEST_START;
	request.id = 0;
	request.request_address = BMC_DIRECT_COMPOSITE_EAT_SHM_BASE;
	CuAssertTrue (test, !bmc_direct_composite_eat_begin (&state, &request));
	request.id = 1;
	request.request_address = 0;
	CuAssertTrue (test, !bmc_direct_composite_eat_begin (&state, &request));
	CuAssertTrue (test, !state.active);
	CuAssertTrue (test, !bmc_direct_composite_eat_begin (NULL, &request));
	CuAssertTrue (test, !bmc_direct_composite_eat_begin (&state, NULL));
}

static void bmc_direct_composite_eat_state_test_publication_order (CuTest *test)
{
	struct publication_log log = {0};
	const struct bmc_direct_composite_eat_publication_ops ops = {
		.write_scratchpad = publication_write,
		.memory_barrier = publication_barrier,
		.clear_notification = publication_clear,
		.notify_bmc = publication_notify,
	};

	TEST_START;
	CuAssertTrue (test,
		bmc_direct_composite_eat_publish_response (&ops, &log,
			BMC_DIRECT_COMPOSITE_EAT_RESPONSE_TOO_SMALL, 4097, 42));
	CuAssertIntEquals (test, 6, log.count);
	CuAssertIntEquals (test, 1, log.event[0]);
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_SCRPAD_COMMAND, log.index[0]);
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_RESPONSE_TOO_SMALL, log.value[0]);
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_SCRPAD_RESP_LEN, log.index[1]);
	CuAssertIntEquals (test, 4097, log.value[1]);
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_SCRPAD_RESPONSE_ID, log.index[2]);
	CuAssertIntEquals (test, 42, log.value[2]);
	CuAssertIntEquals (test, 2, log.event[3]);
	CuAssertIntEquals (test, 3, log.event[4]);
	CuAssertIntEquals (test, BMC_DIRECT_NOTIFICATION_COMPOSITE_EAT, log.value[4]);
	CuAssertIntEquals (test, 4, log.event[5]);
	CuAssertIntEquals (test, BMC_DIRECT_NOTIFICATION_COMPOSITE_EAT, log.value[5]);
	CuAssertTrue (test, !bmc_direct_composite_eat_publish_response (NULL, &log, 0, 0, 0));
}

static void bmc_direct_composite_eat_state_test_status_mapping (CuTest *test)
{
	TEST_START;
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_OK,
		bmc_direct_composite_eat_map_generator_status (COMPOSITE_EAT_GENERATOR_OK));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_BAD_VERSION,
		bmc_direct_composite_eat_map_generator_status (COMPOSITE_EAT_GENERATOR_BAD_VERSION));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_BAD_REQUEST,
		bmc_direct_composite_eat_map_generator_status (COMPOSITE_EAT_GENERATOR_BAD_REQUEST));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_TOO_MANY_RECORDS,
		bmc_direct_composite_eat_map_generator_status (COMPOSITE_EAT_GENERATOR_TOO_MANY_RECORDS));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_RESPONSE_TOO_SMALL,
		bmc_direct_composite_eat_map_generator_status (COMPOSITE_EAT_GENERATOR_BUFFER_TOO_SMALL));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_SIGN_FAILED,
		bmc_direct_composite_eat_map_generator_status (COMPOSITE_EAT_GENERATOR_SIGN_ERROR));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_BUSY,
		bmc_direct_composite_eat_map_generator_status (COMPOSITE_EAT_GENERATOR_BUSY));
	CuAssertIntEquals (test, BMC_DIRECT_COMPOSITE_EAT_INTERNAL,
		bmc_direct_composite_eat_map_generator_status (COMPOSITE_EAT_GENERATOR_ATTESTATION_ERROR));
}


TEST_SUITE_START (bmc_direct_composite_eat_state);

TEST (bmc_direct_composite_eat_state_test_buffers);
TEST (bmc_direct_composite_eat_state_test_ownership);
TEST (bmc_direct_composite_eat_state_test_invalid_request);
TEST (bmc_direct_composite_eat_state_test_publication_order);
TEST (bmc_direct_composite_eat_state_test_status_mapping);

TEST_SUITE_END;