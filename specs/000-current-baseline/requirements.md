# Requirements — Current Runtime Baseline

Status: implemented baseline through PR #11 (`ac008b2d2a3158ffa4cb285e88cfabacea2ca4a2`)

## Goal

Describe the runtime behavior already implemented from M0 through the current M3 mapping/metadata slice in the repository's current requirements/design/tasks structure. This package establishes the contract that future work must preserve; it does not add runtime behavior.

## Scope

- AArch32 ARM/Thumb execution behind an internal CPU adapter.
- Engine-independent logical 32-bit guest memory.
- Callback-backed and mapped 4 GiB guest-memory implementations.
- Dynarmic fastmem acceleration with correctness fallback.
- ELF32 ARM `ET_EXEC` / `ET_DYN` mapping and validation.
- Validated `PT_DYNAMIC` guest-range discovery.
- Structural `Elf32_Dyn` parsing from guest memory.
- Linux host validation, reproducible Android ARM32 fixture validation, Android `arm64-v8a` cross-build evidence, and the already recorded Android/AArch64 mapped-memory runtime evidence.

## Non-goals

- Dynamic dependency loading / `DT_NEEDED` resolution.
- Dynamic string/symbol table consumption or symbol lookup/interposition.
- ARM relocation application.
- RELRO or TLS implementation.
- Automatic `ET_DYN` guest-VA allocation.
- Android libc/JNI/graphics/audio compatibility layers.
- General game/application compatibility claims.
- Executing the current real ARM32 ELF fixture end-to-end on Android.

## Requirements

### R1 — CPU engine boundary

A32 execution must remain behind the repository's CPU adapter. Dynarmic-specific types and configuration must not become public contracts for ELF, memory, ABI, or application layers.

### R2 — Logical guest addresses

Guest virtual addresses must remain logical 32-bit values independent from host pointer identity. Higher layers must access guest memory through the guest-memory boundary rather than treating a guest address as a host pointer.

### R3 — Guest-memory correctness and acceleration

`memory::GuestMemory` must remain the engine-independent memory contract. The runtime may accelerate mapped guest pages through a contiguous 4 GiB high-host-VA fastmem reservation, but callback-backed memory access must remain a correctness fallback when fastmem is unavailable or faults.

### R4 — ELF32 mapping contract

The ELF loader must validate supported ARM ELF32 images before mutating guest mappings, map `PT_LOAD` segments into guest memory, copy file bytes, zero BSS, apply final supported permissions, preserve ELF alignment constraints, and return guest-only load metadata.

`ET_EXEC` must use fixed guest VAs. `ET_DYN` must use an explicit base/load bias that preserves all relevant `PT_LOAD p_align` constraints.

### R5 — Dynamic-segment boundary

The loader may expose one validated `PT_DYNAMIC` guest range but must not perform dynamic-linker semantics as a side effect of mapping. A separate structural parser must consume the loader-validated range through `GuestMemory`, preserve raw signed `d_tag` plus raw 32-bit values, retain the first `DT_NULL`, reject malformed/truncated/unterminated ranges, and preserve unknown tags.

### R6 — Layer separation

CPU execution, guest address space, ELF mapping, structural dynamic metadata, future dynamic linking, ABI bridging, compatibility libraries, pthread/TLS, signals, JNI, graphics/audio, instrumentation, and application profiles must remain separable concerns.

### R7 — Evidence discipline

Repository documentation must distinguish executed evidence from inference. A check is `PASS` only when it actually ran and succeeded; actual 16 KiB Android host-page behavior and end-to-end execution of the real ARM32 fixture remain unproven until directly executed.

## Acceptance Criteria

- AC1: ARM and Thumb guest smoke execution are covered by the host test suite.
- AC2: register state, branch/call, memory load/store, stack, and guest-memory bounds behavior are covered by host tests.
- AC3: mapped guest-memory lifecycle plus Dynarmic fastmem/fallback behavior are covered by host tests and remain compatible with the already recorded Android/AArch64 mapped-memory evidence.
- AC4: ELF32 `ET_DYN` and `ET_EXEC` mapping, malformed-header/segment rejection, conflicts, alignment, BSS, and final permissions are covered by host tests.
- AC5: a reproducible NDK-generated ARM32 Android fixture proves real `PT_LOAD` and `PT_DYNAMIC` metadata handling without being treated as application compatibility evidence.
- AC6: structural `Elf32_Dyn` parsing is covered by synthetic and real-fixture tests, including `DT_NULL` termination and unknown-tag preservation.
- AC7: the Android `arm64-v8a` build produces exactly `liba32android.so` and the Android diagnostics/smoke binaries continue to cross-build.
- AC8: no current API or documentation claims `DT_NEEDED`, symbol resolution, relocations, RELRO/TLS, or end-to-end ARM32 Android library execution as implemented.

## Invariants

- No host pointer is exposed as a guest pointer through generic runtime APIs.
- Fastmem is optional acceleration, not the correctness contract.
- Loader validation happens before loader-owned guest mutations wherever malformed metadata can be detected up front.
- Unknown ELF dynamic tags are preserved structurally rather than silently accepted as supported semantics.
- Application-specific behavior does not enter the generic core.

## Compatibility / Migration

This package documents the repository as it exists through PR #11. Future feature packages must preserve these contracts unless an explicit accepted decision supersedes them.

## Open Questions

- Broad Android/vendor/kernel compatibility for the high-base 4 GiB reservation needs more device evidence.
- Actual 16 KiB Android host-page behavior remains NOT RUN.
- The project's own open-source license is still undecided.
