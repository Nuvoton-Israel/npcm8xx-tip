/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "testing.h"
#include "composite_eat/bmc_direct/bmc_direct_composite_eat_abi.h"
#include "composite_eat/bmc_direct/bmc_direct_composite_eat_transport.h"


TEST_SUITE_LABEL ("bmc_direct_composite_eat_transport");

struct publication_log {
	uint32_t event[6];
	uint32_t index[6];
	uint32_t value[6];
	size_t count;
};

struct output_log {
	uint8_t data[16];
	uint32_t address;
	size_t length;
	size_t lock_count;
	size_t unlock_count;
};

static void output_lock (void *context)
{
	struct output_log *log = context;

	log->lock_count++;
}

static void output_unlock (void *context)
{
	struct output_log *log = context;

	log->unlock_count++;
}

static int output_write (void *context, uint32_t address, const uint8_t *data, size_t length)
{
	struct output_log *log = context;

	log->address = address;
	memcpy (&log->data[log->length], data, length);
	log->length += length;
	return 0;
}

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


static void bmc_direct_composite_eat_transport_test_buffers (CuTest *test)
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

static void bmc_direct_composite_eat_transport_test_ownership (CuTest *test)
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
	CuAssertIntEquals (test, 1, state.epoch);
	CuAssertTrue (test, !bmc_direct_composite_eat_request_current (&state, &second));
	CuAssertTrue (test, !state.active);
	CuAssertIntEquals (test, 0, state.pending.id);
	CuAssertIntEquals (test, 0, state.last_request_id);
}

static void bmc_direct_composite_eat_transport_test_invalid_request (CuTest *test)
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

static void bmc_direct_composite_eat_transport_test_publication_order (CuTest *test)
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

static void bmc_direct_composite_eat_transport_test_reset_stops_output (CuTest *test)
{
	struct bmc_direct_composite_eat_state state = {0};
	struct bmc_direct_composite_eat_request request = {
		.id = 1,
		.request_address = BMC_DIRECT_COMPOSITE_EAT_SHM_BASE,
		.request_length = 128,
		.response_address =
			BMC_DIRECT_COMPOSITE_EAT_SHM_BASE + BMC_DIRECT_COMPOSITE_EAT_RESP_OFFSET,
		.response_capacity = 16,
	};
	struct output_log log = {0};
	const struct bmc_direct_composite_eat_output_ops ops = {
		.lock = output_lock,
		.unlock = output_unlock,
		.write = output_write,
	};
	const uint8_t first[] = {0x11, 0x22};
	const uint8_t stale[] = {0x33, 0x44};
	size_t offset = 0;

	TEST_START;
	CuAssertTrue (test, bmc_direct_composite_eat_begin (&state, &request));
	request = state.pending;
	CuAssertTrue (test, bmc_direct_composite_eat_write_response (&state, &request, &ops,
		&log, &offset, first, sizeof (first)));
	CuAssertIntEquals (test, sizeof (first), offset);
	CuAssertIntEquals (test, sizeof (first), log.length);
	CuAssertIntEquals (test, request.response_address, log.address);

	bmc_direct_composite_eat_reset (&state);
	CuAssertTrue (test, !bmc_direct_composite_eat_write_response (&state, &request, &ops,
		&log, &offset, stale, sizeof (stale)));
	CuAssertIntEquals (test, sizeof (first), offset);
	CuAssertIntEquals (test, sizeof (first), log.length);
	CuAssertIntEquals (test, 2, log.lock_count);
	CuAssertIntEquals (test, 2, log.unlock_count);

#if SIZE_MAX > UINT32_MAX
	offset = (size_t) UINT32_MAX + 1u;
	CuAssertTrue (test, !bmc_direct_composite_eat_write_response (&state, &request, &ops,
		&log, &offset, stale, sizeof (stale)));
#endif
}

TEST_SUITE_START (bmc_direct_composite_eat_transport);

TEST (bmc_direct_composite_eat_transport_test_buffers);
TEST (bmc_direct_composite_eat_transport_test_ownership);
TEST (bmc_direct_composite_eat_transport_test_invalid_request);
TEST (bmc_direct_composite_eat_transport_test_publication_order);
TEST (bmc_direct_composite_eat_transport_test_reset_stops_output);

TEST_SUITE_END;