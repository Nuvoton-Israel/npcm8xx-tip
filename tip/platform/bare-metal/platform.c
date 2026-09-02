// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#include "platform_api.h"
#include "status/rot_status.h"
#include "hal.h"
#include "common/unused.h"


/**
 * Initialize components for the platform abstraction routines used in a bare metal system.
 */
void platform_init ()
{
}

/**
 * Sleep for a specified number of milliseconds.
 *
 * @param msec The number of milliseconds to sleep.
 */
void platform_msleep (uint32_t msec)
{
	TWD_WatchDogRestart ();
	CLK_Delay_MicroSec (msec * 1000);
}


#define PLATFORM_TIMEOUT_ERROR(code) ROT_ERROR (ROT_MODULE_PLATFORM_TIMEOUT, code)

/**
 * Initialize a clock structure to represent the time at which a timeout expires.
 *
 * @param msec The number of milliseconds to use for the timeout.
 * @param timeout The timeout clock to initialize.
 *
 * @return 0 if the timeout was initialized successfully or an error code.
 */
int platform_init_timeout (uint32_t msec, platform_clock *timeout)
{
	if (timeout == NULL) {
		return PLATFORM_TIMEOUT_ERROR (PLATFORM_INVALID_ARGUMENT);
	}

	return 0;
}

/**
 * Increase the amount of time for an existing timeout.
 *
 * @param msec The number of milliseconds to increase the timeout expiration by.
 * @param timeout The timeout clock to update.
 *
 * @return 0 if the timeout was updated successfully or an error code.
 */
int platform_increase_timeout (uint32_t msec, platform_clock *timeout)
{
	if (timeout == NULL) {
		return PLATFORM_TIMEOUT_ERROR (PLATFORM_INVALID_ARGUMENT);
	}

	return 0;
}

/**
 * Determine if the specified timeout has expired.
 *
 * @param timeout The timeout to check.
 *
 * @return 1 if the timeout has expired, 0 if it has not, or an error code.
 */
int platform_has_timeout_expired (const platform_clock *timeout)
{
	if (timeout == NULL) {
		return PLATFORM_TIMEOUT_ERROR (PLATFORM_INVALID_ARGUMENT);
	}

	return 0;
}

/**
 * Initialize a clock structure to represent current tick count.
 *
 * @param currtime The platform_clock type to initialize.
 *
 * @return 0 if the current tick count was initialized successfully or an error code.
 */
int platform_init_current_tick (platform_clock *currtime)
{
	UNUSED (currtime);
	return 0;
}

/**
 * Get the duration between two clock instances.  These are expected to be initialized with
 * {@link platform_init_current_tick}.
 *
 * This is intended to measure small durations.  Very long durations may not be accurately
 * calculated due value limitations/overflow.
 *
 * @param start The start time for the time duration.
 * @param end The end time for the time duration.
 *
 * @return The elapsed time, in milliseconds.  If either clock is null, the elapsed time will be 0.
 */
uint32_t platform_get_duration (const platform_clock *start, const platform_clock *end)
{
	UNUSED (start);
	UNUSED (end);
	return 0;
}

/**
 * Get the current system time.
 *
 * @return The current time, in milliseconds.
 */
uint64_t platform_get_time (void)
{
	return 0;
}

/**
 * Create a timer that is not yet armed.
 *
 * @param timer The container for the created timer.
 * @param callback The function to call when the timer expires.
 * @param context The context to pass to the notification function.
 *
 * @return 0 if the timer was created or an error code.
 */
int platform_timer_create (platform_timer *timer, timer_callback callback, void *context)
{
	UNUSED (timer);
	UNUSED (callback);
	UNUSED (context);
	return 0;
}

/**
 * Start a timer that will call the notification function once on expiration.  If additional
 * notifications are required, the timer must be rearmed.
 *
 * Calling this on an already armed timer will restart the timer with the specified timeout.
 *
 * @param timer The timer to start.
 * @param ms_timeout The timeout to wait for timer expiration, in milliseconds.
 *
 * @return 0 if the timer has started or an error code.
 */
int platform_timer_arm_one_shot (platform_timer *timer, uint32_t ms_timeout)
{
	UNUSED (timer);
	UNUSED (ms_timeout);
	return 0;
}

/**
 * Stop a timer and prevent the notification from being called.  The timer instance remains valid
 * and can rearmed.
 *
 * @param timer The timer to stop.
 *
 * @return 0 if the timer is stopped or an error code.
 */
int platform_timer_disarm (platform_timer *timer)
{
	UNUSED (timer);
	return 0;
}

/**
 * Stop a timer and prevent the notification from being called.  The timer instance will be deleted
 * and cannot be reused without calling {@link platform_timer_create}.
 *
 * A timer instance must never be deleted from within the context of the event callback.
 *
 * @param timer The timer to delete.
 */
void platform_timer_delete (platform_timer *timer)
{
	UNUSED (timer);
}

