/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#include <string.h>

#include "tip_rom_dme_handoff.h"
#include "tip_utils.h"
#ifndef TIP_ROM_DME_HANDOFF_NO_DEFAULT_OPS
#include "hal.h"
#endif


_Static_assert (sizeof (SEC_PRE_PCR0_T) == 88, "Unexpected ROM pre-PCR0 layout");
_Static_assert (sizeof (SEC_DME_DICE_T_A1) == 784, "Unexpected A1 ROM DME layout");
_Static_assert (sizeof (SEC_DME_DICE_T_A2) == 848, "Unexpected A2 ROM DME layout");

static struct tip_rom_dme_field tip_rom_dme_handoff_field (const uint8_t *data,
	size_t length)
{
	struct tip_rom_dme_field field = {
		.data = data,
		.length = length,
	};

	return field;
}

enum tip_rom_dme_handoff_status tip_rom_dme_handoff_parse (uint32_t chip_revision,
	const void *handoff, struct tip_rom_dme_public_evidence *evidence)
{
	if ((handoff == NULL) || (evidence == NULL)) {
		return TIP_ROM_DME_HANDOFF_BAD_ARGUMENT;
	}
	memset (evidence, 0, sizeof (*evidence));

	switch (chip_revision) {
		case ARBEL_VERSION_A1:
		{
			const SEC_DME_DICE_T_A1 *data = handoff;

			evidence->dme_nonce = tip_rom_dme_handoff_field (data->dme_nonce,
				sizeof (data->dme_nonce));
			evidence->dice_public_key = tip_rom_dme_handoff_field (data->dice_pub_key,
				sizeof (data->dice_pub_key));
			evidence->dme_pcr0 = tip_rom_dme_handoff_field (data->dme_pcr0,
				sizeof (data->dme_pcr0));
			evidence->dme_public_key = tip_rom_dme_handoff_field (data->dme_pub_key,
				sizeof (data->dme_pub_key));
			evidence->dme_signature = tip_rom_dme_handoff_field (data->dme_signature,
				sizeof (data->dme_signature));
			break;
		}

		case ARBEL_VERSION_A2:
		case ARBEL_VERSION_A3:
		{
			const SEC_DME_DICE_T_A2 *data = handoff;

			evidence->dme_nonce = tip_rom_dme_handoff_field (data->dme_nonce,
				sizeof (data->dme_nonce));
			evidence->dme_challenge = tip_rom_dme_handoff_field (data->dme_challenge,
				sizeof (data->dme_challenge));
			evidence->dice_public_key = tip_rom_dme_handoff_field (data->dice_pub_key,
				sizeof (data->dice_pub_key));
			evidence->dme_pcr0 = tip_rom_dme_handoff_field (data->dme_pcr0,
				sizeof (data->dme_pcr0));
			evidence->dme_public_key = tip_rom_dme_handoff_field (data->dme_pub_key,
				sizeof (data->dme_pub_key));
			evidence->dme_signature = tip_rom_dme_handoff_field (data->dme_signature,
				sizeof (data->dme_signature));
			break;
		}

		default:
			return TIP_ROM_DME_HANDOFF_UNSUPPORTED_REVISION;
	}

	return TIP_ROM_DME_HANDOFF_OK;
}

enum tip_rom_dme_handoff_status tip_rom_dme_handoff_get (
	struct tip_rom_dme_public_evidence *evidence)
{
#ifdef TIP_ROM_DME_HANDOFF_NO_DEFAULT_OPS
	(void) evidence;
	return TIP_ROM_DME_HANDOFF_UNSUPPORTED_REVISION;
#else
	extern uint32_t __dme_dice_table_start;

	return tip_rom_dme_handoff_parse (CHIP_Get_Version (), &__dme_dice_table_start,
		evidence);
#endif
}