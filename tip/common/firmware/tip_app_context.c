/*---------------------------------------------------------------------------------------------------------*/
/*  Copyright (c) 2026 by Nuvoton Technology Corporation.                                                       */
/*  SPDX-License-Identifier: Apache-2.0                                                                                              */
/*<<<---------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "tip_app_context.h"


static int tip_app_context_save (const struct app_context *context)
{
    struct tip_app_context *tip_context = (struct tip_app_context*) context;
	if (tip_context == NULL) {
		return APP_CONTEXT_INVALID_ARGUMENT;
	}
	return 0;
}

int tip_app_context_restore (const struct app_context *context)
{
	struct tip_app_context *tip_context = (struct tip_app_context*) context;
	if (tip_context == NULL) {
		return APP_CONTEXT_INVALID_ARGUMENT;
	}
	return 0;
}

/**
 * Initialize application context storage for TIP.
 *
 * @param context The context instance to initialize.
 *
 * @return 0 if the instance was successfully initialized or an error code.
 */
int tip_app_context_init (struct tip_app_context *context)
{
	if (context == NULL) {
		return APP_CONTEXT_INVALID_ARGUMENT;
	}

	memset (context, 0, sizeof (struct tip_app_context));

	context->base.save = tip_app_context_save;

	return 0;
}

/**
 * Release the resources used for TIP context storage.
 *
 * @param context The context to release.
 */
void tip_app_context_release (struct tip_app_context *context)
{

}
