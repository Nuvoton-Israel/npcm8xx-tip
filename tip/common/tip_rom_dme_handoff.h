/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Nuvoton Technology Corporation. */

#ifndef TIP_ROM_DME_HANDOFF_H_
#define TIP_ROM_DME_HANDOFF_H_

#include <stddef.h>
#include <stdint.h>


#define SEC_DME_NONCE_LENGTH 64u
#define SEC_DME_CHALLENGE_LENGTH 64u
#define SEC_PCR0_LENGTH 64u
#define SEC_PUB_KEY_SIZE 96u

#pragma pack(push, 1)
typedef struct {
	uint16_t tip_dbg_ctl;
	uint16_t tip_mem_ctl;
	uint16_t tip_pqa_ctl;
	uint16_t tip_sec_boot;
	uint32_t fustrap1;
	uint32_t fustrap2;
	uint32_t tip_fcfg[8];
	uint32_t bmc_fcfg[8];
	uint8_t otp_life_cycle;
	uint8_t uds_valid[3];
	uint8_t uds_version[3];
	uint8_t padding[1];
} SEC_PRE_PCR0_T;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct SEC_DME_DICE_A1_tag {
	uint8_t dme_pub_key[SEC_PUB_KEY_SIZE];
	uint8_t dme_signature[SEC_PUB_KEY_SIZE];
	uint8_t dme_hash[SEC_PCR0_LENGTH];
	uint8_t dme_nonce[SEC_DME_NONCE_LENGTH];
	uint8_t dme_pcr0[SEC_PCR0_LENGTH];
	uint8_t dice_pcr0[SEC_PCR0_LENGTH];
	SEC_PRE_PCR0_T dme_pre_pcr0;
	SEC_PRE_PCR0_T dice_pre_pcr0;
	uint8_t cdi[SEC_PCR0_LENGTH];
	uint8_t dice_pub_key[SEC_PUB_KEY_SIZE];
} SEC_DME_DICE_T_A1;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
	uint8_t dme_pub_key[SEC_PUB_KEY_SIZE];
	uint8_t dme_signature[SEC_PUB_KEY_SIZE];
	uint8_t dme_hash[SEC_PCR0_LENGTH];
	uint8_t dme_nonce[SEC_DME_NONCE_LENGTH];
	uint8_t dme_challenge[SEC_DME_CHALLENGE_LENGTH];
	uint8_t dme_pcr0[SEC_PCR0_LENGTH];
	uint8_t dice_pcr0[SEC_PCR0_LENGTH];
	SEC_PRE_PCR0_T dme_pre_pcr0;
	SEC_PRE_PCR0_T dice_pre_pcr0;
	uint8_t cdi[SEC_PCR0_LENGTH];
	uint8_t dice_pub_key[SEC_PUB_KEY_SIZE];
} SEC_DME_DICE_T_A2;
#pragma pack(pop)

#pragma pack(push, 1)
typedef union {
	SEC_DME_DICE_T_A1 *a1;
	SEC_DME_DICE_T_A2 *a2;
} SEC_DME_DICE_T_PTR;
#pragma pack(pop)

#pragma pack(push, 1)
struct tip_dme_struct_data_a2 {
	uint8_t dme_nonce[SEC_DME_NONCE_LENGTH];
	uint8_t dme_challenge[SEC_DME_CHALLENGE_LENGTH];
	uint8_t dice_pub_key[SEC_PUB_KEY_SIZE];
	uint8_t dme_pcr0[SEC_PCR0_LENGTH];
};
#pragma pack(pop)

struct tip_rom_dme_field {
	const uint8_t *data;
	size_t length;
};

struct tip_rom_dme_public_evidence {
	struct tip_rom_dme_field dme_nonce;
	struct tip_rom_dme_field dme_challenge;
	struct tip_rom_dme_field dice_public_key;
	struct tip_rom_dme_field dme_pcr0;
	struct tip_rom_dme_field dme_public_key;
	struct tip_rom_dme_field dme_signature;
};

enum tip_rom_dme_handoff_status {
	TIP_ROM_DME_HANDOFF_OK = 0,
	TIP_ROM_DME_HANDOFF_BAD_ARGUMENT,
	TIP_ROM_DME_HANDOFF_UNSUPPORTED_REVISION,
};

enum tip_rom_dme_handoff_status tip_rom_dme_handoff_parse (uint32_t chip_revision,
	const void *handoff, struct tip_rom_dme_public_evidence *evidence);

enum tip_rom_dme_handoff_status tip_rom_dme_handoff_get (
	struct tip_rom_dme_public_evidence *evidence);


#endif /* TIP_ROM_DME_HANDOFF_H_ */
