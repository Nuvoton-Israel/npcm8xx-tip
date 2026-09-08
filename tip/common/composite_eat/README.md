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
| `BMC_TIP_EVIDENCE_PROFILE.md` | Local evidence semantics, verifier rules, and coverage boundary. |
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
