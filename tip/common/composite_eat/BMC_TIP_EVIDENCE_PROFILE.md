# Local BMC/TIP Evidence Profile

This document defines the local BMC/TIP evidence semantics used by the Composite EAT adapter.
TCG DICE Concise Evidence defines the encoding; it does not assign these environment or
measured-element meanings.

## Coverage

This profile covers TIP evidence and BMC boot evidence observed by TIP. It does not capture Linux
kernel or runtime measurements. Defer that coverage until Linux enables the TrustZone fTPM and the
corresponding quote and measurement collection path is integrated into the Composite EAT flow.

## Environments

| Class ID bytes | Vendor | Model | Scope |
|---|---|---|---|
| `npcm850-tip` | `Nuvoton` | `NPCM850 TIP` | Cerberus PCR store populated by TIP measured boot and configuration flows. |
| `npcm850-rom-dme` | `Nuvoton` | `NPCM850 ROM DME` | Immutable DME/DICE handoff produced by NPCM850 ROM. |

Class IDs are CBOR tag 560 byte strings containing the exact ASCII bytes shown above. Vendor and
model are CBOR text strings.

## Measurements

| Environment | `mkey` | Value | Algorithm | Flags |
|---|---|---|---|---|
| `npcm850-tip` | unsigned PCR-bank index | Computed Cerberus PCR-bank digest | Bank algorithm, currently SHA-256 or SHA-384 | none |
| `npcm850-rom-dme` | text `dme-pcr0` | Exact 64-byte ROM handoff `dme_pcr0` | SHA-512 | integrity-protected, immutable, TCB |

PCR-bank indices retain their existing TIP-local meanings. The DME value uses a separate
environment and text key, so it does not collide with PCR index `0`.

## Collection Rules

1. The adapter locks every configured Cerberus PCR bank before computing any PCR digest and
   releases all locks only after all PCR digests have been copied.
2. DME PCR0 is copied from the ROM handoff after the PCR snapshot. It is immutable for the boot
   epoch and therefore does not require a shared PCR lock.
3. A1 uses `SEC_DME_DICE_T_A1`; A2 and A3 use `SEC_DME_DICE_T_A2`. Other chip revisions are
   rejected.
4. Only public handoff fields are exposed by `tip_rom_dme_handoff.*`. CDI is not part of its public
   evidence view.
5. The encoded TCG evidence is generated deterministically from the immutable operation workspace
   and streamed into both COSE signing input and final output.

## Verifier Rules

A verifier identifies these semantics using the EAT profile together with the environment class
and `mkey`; it must not infer meaning from digest length alone. Reference values and appraisal
policy belong in the corresponding CoRIM/CoMID material.

For A2 and A3, the verifier should compare claim-273 `dme-pcr0` with the DME PCR0 inside the Device
ID certificate's DME extension. A mismatch fails appraisal. The A1 Device ID certificate does not
carry that DME extension, so A1 appraisal must rely on the signed claim, device identity chain, and
A1-specific endorsement policy rather than that cross-check.

The verifier must treat absence of the required ROM/DME environment as an error for this profile.
Unsupported chip revisions fail local evidence capture rather than silently omitting DME PCR0.