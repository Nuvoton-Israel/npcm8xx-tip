// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_APP_CONTEXT_H_
#define TIP_APP_CONTEXT_H_

#include "firmware/app_context.h"


/**
 * Implementation to store TIP application context.
 */
struct tip_app_context {
	struct app_context base;		/**< Base application context API. */
};


int tip_app_context_init (struct tip_app_context *context);
int tip_app_context_restore (const struct app_context *context);
void tip_app_context_release (struct tip_app_context *context);


#endif /* TIP_APP_CONTEXT_H_ */
