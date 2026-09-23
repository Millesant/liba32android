# Requirements — ELF32 PLT REL Metadata

Status: IMPLEMENTED — T001 required-job validation passed CI #222 / run `35933694619` at `9a81ed71a027beb166970bcf137bac9a71112f98`; T002 final feature gate pending

## Goal

Extend `elf32_linker_metadata` with a validated, guest-only descriptor for the ARM ELF32 PLT relocation table so later work can implement eager `R_ARM_JUMP_SLOT` processing without reinterpreting raw dynamic entries.

This feature is metadata-only and read-only. It validates `DT_JMPREL`, `DT_PLTRELSZ`, and `DT_PLTREL` for the AArch32 `Elf32_Rel` form. It does not decode relocation entries or write guest memory.

## Scope

- Recognize `DT_JMPREL`, `DT_PLTRELSZ`, and `DT_PLTREL` as singleton tags.
- Require all three tags to be present together or absent together.
- Require `DT_PLTREL == DT_REL`.
- Require `DT_PLTRELSZ` to be divisible by 8.
- Rebase `DT_JMPREL` exactly once with checked 32-bit guest arithmetic.
- Validate the declared PLT REL range through `memory::GuestMemory`.
- Expose the PLT table independently from the existing main `DT_REL` descriptor.
- Preserve current string, symbol, hash, SONAME/NEEDED, version-marker, and main-REL behavior.

## Non-goals

- PLT relocation entry decoding or `R_ARM_JUMP_SLOT`.
- Lazy binding, resolver trampolines, or `DT_PLTGOT` semantics.
- Main+PLT relocation transactionality.
- RELA, RELR, Android packed relocations, APS2, or section relocations.
- Version-aware/protected requester semantics, process-wide/global-group interposition, namespaces, TLS, IFUNC, RELRO, constructors, or guest execution.
- Host-pointer exposure.

## Requirements

### R1 — Layer boundary
The change remains inside `elf32_linker_metadata` and is read-only.

### R2 — PLT REL group
`DT_JMPREL`, `DT_PLTRELSZ`, and `DT_PLTREL` form one optional group. Partial groups and duplicate recognized singletons fail explicitly.

### R3 — REL-only format
When present, `DT_PLTREL` must equal the dynamic-tag value `DT_REL`. RELA or any other discriminator is unsupported in this AArch32 slice.

### R4 — Size and range
The ELF32 REL entry size is 8 bytes. `DT_PLTRELSZ` must be divisible by 8; zero bytes are valid. Rebased address/range overflow and unreadable non-empty ranges fail.

### R5 — Output model
Add a separate optional PLT REL descriptor to collected and validated metadata. The validated descriptor contains only logical guest base, byte size, and entry size 8.

### R6 — Independence from main REL
Main REL and PLT REL may coexist and are validated independently. This metadata layer does not reject overlap or duplicate targets across them.

### R7 — Failure model
Add explicit errors for incomplete PLT REL metadata, unsupported `DT_PLTREL`, and invalid PLT REL size. Existing duplicate/overflow/read errors remain applicable. Failure never mutates guest state.

### R8 — Compatibility
Existing metadata fields/errors remain source-compatible except for additive fields and enum values. Unknown/deferred tags remain tolerated.

### R9 — Validation
Synthetic coverage must prove valid zero/non-zero-bias PLT metadata, coexistence with main REL, duplicates, partial groups, wrong `DT_PLTREL`, bad size, overflow, unreadable range, zero length, and no mutation. The current real ARM32 fixture has no PLT group and must continue validating with no PLT descriptor. Final acceptance requires exact-head Linux CTest, Android x86_64 probe, and Android arm64-v8a cross-build PASS.

## Acceptance Criteria

- AC1: Valid `DT_JMPREL` / `DT_PLTRELSZ` / `DT_PLTREL=DT_REL` produces one rebased guest-only PLT REL descriptor.
- AC2: Main REL and PLT REL descriptors coexist without changing main-REL semantics.
- AC3: Duplicate or incomplete PLT groups fail.
- AC4: Non-REL `DT_PLTREL` fails explicitly.
- AC5: Invalid size, overflow, or unreadable bytes fail without mutation.
- AC6: A zero-byte PLT REL table is valid.
- AC7: Existing callers remain source-compatible apart from additive fields/errors.
- AC8: The current real fixture still reports no PLT REL descriptor.
- AC9: Exact-head Linux and both Android CI jobs PASS.

## Invariants

- Guest VAs remain logical 32-bit values.
- Linker metadata remains read-only and does not decode/apply relocations.
- PLT REL metadata stays distinct from the main REL table.
- Lazy binding and `R_ARM_JUMP_SLOT` remain future contracts.

## Evidence Basis

System V ELF defines `DT_JMPREL` as the PLT relocation table, `DT_PLTRELSZ` as its byte size, and `DT_PLTREL` as the REL/RELA selector. Android bionic's ARM linker requires `DT_PLTREL == DT_REL`, rebases `DT_JMPREL`, and derives the REL count from `DT_PLTRELSZ / sizeof(Elf32_Rel)`. Feature 007 explicitly deferred PLT/JMPREL because this metadata was not yet validated.

## Open Questions

None block T001. Positive real `R_ARM_JUMP_SLOT` artifact evidence is deferred to the later relocation-application feature.
