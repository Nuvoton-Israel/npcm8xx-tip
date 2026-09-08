<!-- SPDX-License-Identifier: Apache-2.0 -->

# TIP Composite EAT

This directory contains the RoT-neutral protocol machinery and feature-gated TIP integration that
produce the signed Composite EAT main token. It does not build the complete tag-602 bundle.

## Modules

| Module | Ownership |
|---|---|
| `types.h` | Shared bounded byte-view and digest-size types. |
| `generation_request.*` | Strict request-v1 decoder.  Decoded byte fields borrow immutable request-snapshot storage. |
| `tcg_concise_evidence.*` | Platform-neutral bounded TCG DICE Concise Evidence model, validator, and streaming serializer. |
| `profile_evidence.*` | Composite EAT profile policy and implementation-limit validation. |
| `main_token_builder.*` | Platform-neutral deterministic CWT/COSE main-token encoder. Claim 273 inputs are opaque registered-format byte views or deterministic streams. |
| `tip_evidence_adapter.*` | BMC/TIP evidence snapshot and Concise Evidence mapping. |
| `tip_main_token_generator.*` | TIP identity, certificate, SHA-384, ES384, cleanup, and generation orchestration. |
| `bmc_direct/bmc_direct_composite_eat_abi.h` | Shared-memory command and status contract. |
| `bmc_direct/bmc_direct_composite_eat_transport.*` | Address validation, one-in-flight correlation, reset epochs, and response publication. |
| `bmc_direct/bmc_direct_composite_eat_status.*` | TIP generator status to BMC_DIRECT ABI translation. |
| `testing/` | Focused generic and TIP tests mirroring the production modules. |
| `testing/bmc_direct/` | BMC_DIRECT transport and status tests. |
| `qcbor/` | Pinned, unmodified third-party QCBOR subset described in `THIRD_PARTY_NOTICES.md`. |

## Dependency Direction

```text
BMC task -> BMC_DIRECT transport
BMC task -> BMC_DIRECT status -> TIP main-token status
BMC task -> TIP main-token generator -> TIP evidence adapter
TIP main-token generator -> main-token builder -> generation request/types
TIP evidence adapter     -> profile evidence -> TCG concise evidence/types
```

The shared `tip_rom_dme_handoff.*` module is outside this directory because legacy BMC export and
Composite EAT both consume the ROM handoff. It exposes only approved public fields and never
exposes CDI through its API.

Transport does not own TIP identity, evidence, crypto, or generator state. RoT-neutral encoders do
not include TIP, FreeRTOS, mailbox, PCR-store, or key-manager interfaces.

## Security And Memory

- Only one generation operation may be active.
- The BMC request is copied into a trusted 4 KiB snapshot before decoding.
- Decoded request fields borrow that snapshot, which remains immutable through signing and output.
- All configured PCR banks remain locked until their computed digests are copied into one snapshot.
- ROM DME PCR0 is copied from the immutable revision-specific handoff into the operation workspace.
- Signing input is streamed from trusted state into SHA-384; mutable BMC response memory is never
  used as signing input.
- TCG Concise Evidence is streamed directly from the immutable operation workspace into both the
  signing input and final token; no second encoded-evidence buffer is retained.
- The final signed token is limited to 16 KiB and emitted through reset-epoch-checked writes into
  the validated BMC response window.
- Keys, certificates, evidence, digest, and signature state are cleared on every completion path.
- `TIP_COMPOSITE_EAT=OFF` removes all modules, QCBOR objects, and static workspaces from the
  firmware ELF.

Linux kernel and runtime measurements are not included in this evidence snapshot. That coverage is
deferred until Linux enables the TrustZone fTPM and its quote and measurement collection path is
integrated into the Composite EAT flow.

## Reference Implementation Evidence Mapping

This section describes the local BMC/TIP evidence emitted by this reference implementation. It is
not an authoritative profile definition. TCG DICE Concise Evidence defines the encoding but does
not assign these environment or measured-element meanings. Verifiers require matching deployment
policy and CoRIM/CoMID reference values.

### Coverage

The implementation covers TIP evidence and BMC boot evidence observed by TIP. It does not capture
Linux kernel or runtime measurements. Defer that coverage until Linux enables the TrustZone fTPM
and the corresponding quote and measurement collection path is integrated into the Composite EAT
flow.

### Environments

| Class ID bytes | Vendor | Model | Scope |
|---|---|---|---|
| `npcm850-tip` | `Nuvoton` | `NPCM850 TIP` | Cerberus PCR store populated by TIP measured boot and configuration flows. |
| `npcm850-rom-dme` | `Nuvoton` | `NPCM850 ROM DME` | Immutable DME/DICE handoff produced by NPCM850 ROM. |

Class IDs are CBOR tag 560 byte strings containing the exact ASCII bytes shown above. Vendor and
model are CBOR text strings.

### Measurements

| Environment | `mkey` | Value | Algorithm | Flags |
|---|---|---|---|---|
| `npcm850-tip` | unsigned PCR-bank index | Computed Cerberus PCR-bank digest | Bank algorithm, currently SHA-256 or SHA-384 | none |
| `npcm850-rom-dme` | text `dme-pcr0` | Exact 64-byte ROM handoff `dme_pcr0` | SHA-512 | integrity-protected, immutable, TCB |

PCR-bank indices retain their existing TIP-local meanings. The DME value uses a separate
environment and text key, so it does not collide with PCR index `0`.

### Collection Rules

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

### Verifier Considerations

A verifier identifies these implementation semantics using the EAT profile together with the
environment class and `mkey`; it must not infer meaning from digest length alone.

For A2 and A3, a deployment verifier can compare claim-273 `dme-pcr0` with the DME PCR0 inside the
Device ID certificate's DME extension. The A1 Device ID certificate does not carry that extension,
so A1 appraisal requires device identity and A1-specific endorsement policy.

Deployments using this mapping should treat absence of the ROM/DME environment as an appraisal
failure. Unsupported chip revisions fail local evidence capture instead of silently omitting DME
PCR0.
