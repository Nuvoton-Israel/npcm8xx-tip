<!-- SPDX-License-Identifier: Apache-2.0 -->

# RoT-Neutral Composite EAT

This directory contains bounded protocol machinery for producing a signed Composite EAT main
token. It does not retrieve platform evidence, access an attestation key, or build the complete
tag-602 bundle.

## Modules

| Module | Ownership |
|---|---|
| `types.h` | Shared bounded byte-view and digest-size types. |
| `generation_request.*` | Strict request-v1 decoder.  Decoded byte fields borrow immutable request-snapshot storage. |
| `tcg_concise_evidence.*` | Platform-neutral bounded TCG DICE Concise Evidence model, validator, and streaming serializer. |
| `profile_evidence.*` | Composite EAT profile policy and implementation-limit validation. |
| `main_token_builder.*` | Platform-neutral deterministic CWT/COSE main-token encoder. Claim 273 inputs are opaque registered-format byte views or deterministic streams. |
| `testing/` | Focused tests for request, evidence, profile, and token behavior. |
| `qcbor/` | Pinned, unmodified third-party QCBOR subset described in `THIRD_PARTY_NOTICES.md`. |

## Dependency Direction

```text
main-token builder -> generation request/types
profile evidence   -> TCG concise evidence/types
```

The protocol modules do not include TIP, FreeRTOS, mailbox, PCR-store, or key-manager interfaces.
Platform adapters own trusted evidence retrieval, identity, hashing, signing, and transport.

## Security And Memory

- Decoded request fields borrow immutable caller-owned request storage.
- Local evidence can be supplied as immutable bytes or a deterministic writer with a declared
  length.
- Signing input and final token output use caller-provided writers and buffers.
- The builder accepts only public identity material and signatures; private keys and key handles
  remain platform-owned.
