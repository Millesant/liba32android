# Design — Current Runtime Baseline

Status: implemented baseline through PR #11

## Context

The runtime is a game-agnostic AArch32 compatibility layer intended to execute 32-bit ARM Android native code inside an AArch64 Android process. The existing implementation was built incrementally before the repository adopted the current spec-driven workflow. This document maps that working implementation into the current design layer without redesigning it.

## Chosen Approach

Keep each compatibility concern behind a narrow boundary and move upward from proven primitives:

```text
ARM32 Android ELF image
        |
        v
src/elf/elf32_loader
  validate + map PT_LOAD
  expose guest-only load/PT_DYNAMIC metadata
        |
        v
memory::GuestMemory / MappedGuestMemory
        |
        +---------------------> src/elf/elf32_dynamic
        |                       structural Elf32_Dyn metadata only
        |                              |
        |                              v
        |                       future linker metadata/semantics
        |
        v
src/cpu Dynarmic adapter
  A32 ARM/Thumb execution
  callbacks + optional fastmem
        |
        v
AArch64 Android host
```

The current stack is C++20/CMake, with Dynarmic pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`. Android `arm64-v8a` cross-builds use NDK `27.3.13750724`.

## Architecture / Data Flow

### CPU

`src/cpu/` owns all Dynarmic-specific integration. Higher layers provide logical guest state and memory; they do not depend on Dynarmic types.

The adapter supports A32 ARM/Thumb execution and reports memory failures through the generic execution boundary. Fastmem may be configured when the mapped memory backend exposes an internal reservation base.

### Guest memory

`memory::GuestMemory` is the generic read/write seam.

- `LinearGuestMemory` is the deterministic bounded implementation used for correctness-focused tests.
- `MappedGuestMemory` owns a contiguous 4 GiB high-host-VA reservation and guest page mapping/protection metadata.
- The internal `fastmem_base()` capability can be consumed by the CPU adapter, but that host address is not a guest pointer and is not exposed through loader/runtime contracts.
- Callback access remains available for correctness and fastmem fault fallback.

### ELF32 loader

`src/elf/elf32_loader.*` consumes an in-memory ELF32 image and a mapped guest address space. It validates the supported image shape before creating loader-owned mappings, plans `PT_LOAD` ranges, maps/copies/zeroes them, applies final permissions, and returns guest-only metadata.

For `ET_DYN`, the caller provides an explicit guest base. The resulting load bias must satisfy both host-page mapping requirements and each ELF `PT_LOAD p_align` congruence requirement. For `ET_EXEC`, load bias is zero.

The loader discovers and validates at most one non-empty `PT_DYNAMIC` range inside a readable `PT_LOAD`. It exposes the biased guest address and file/memory sizes but deliberately does not parse tags or perform linking.

### Structural dynamic metadata

`src/elf/elf32_dynamic.*` sits above mapping and below the future linker. It reads the loader-validated file-backed `PT_DYNAMIC` bytes through `GuestMemory` as 8-byte ELF32 entries:

- signed 32-bit `d_tag`;
- raw 32-bit value/pointer field.

The parser requires entry alignment and a terminating `DT_NULL`, retains the terminator, ignores bytes after the first terminator, preserves unknown tags, and does not rebase/dereference pointer-like values.

## Interfaces / Contracts

- CPU <-> memory: `GuestMemory`, plus an internal optional fastmem capability.
- Loader <-> memory: logical guest addresses and mapped-memory operations; no host pointer identity contract.
- Loader -> dynamic parser: `Elf32DynamicSegment` guest address / `p_filesz` / `p_memsz` metadata.
- Dynamic parser -> future linker: ordered raw `d_tag` / raw-value entries only.

## Failure Semantics

- Unsupported/malformed ELF metadata is rejected before mapping when it can be detected from the image/program headers.
- Loader mutation failures trigger rollback attempts for mappings created by that load attempt; pre-existing mappings are not owned or removed.
- Unsupported RWX segments and page-overlapping `PT_LOAD` layouts are rejected rather than silently broadening permissions.
- Structural dynamic parsing rejects invalid ranges, non-8-byte file-backed sizes, unreadable guest bytes, and arrays without `DT_NULL`.
- Unknown dynamic tags are data, not errors; semantic support is deferred to the future linker.

## Security / Compatibility Boundaries

- Never treat guest numeric addresses as trusted host pointers.
- Keep execute/write permissions explicit; do not make segments RWX to simplify loading.
- Do not let application-specific fixes change generic ELF/memory semantics without a demonstrated compatibility requirement and an explicit design decision.
- Android device observations are environment-specific evidence, not universal platform guarantees.

## Test Strategy

The baseline validation pyramid is:

1. focused host tests for CPU state/execution and both memory backends;
2. synthetic ELF32 tests for validation/mapping/dynamic metadata failure modes;
3. reproducible real ARM32 Android ELF generation with pinned NDK/API inputs and byte-identical regeneration checks;
4. real-fixture loader plus structural dynamic-array integration tests;
5. Android `arm64-v8a` cross-build of the runtime and diagnostics;
6. separate real-device probes for host VM/page/fastmem behavior when device access exists.

At `ac008b2d2a3158ffa4cb285e88cfabacea2ca4a2`, GitHub Actions run `35204765081` executed 20/20 host tests successfully and completed the Android `arm64-v8a` cross-build. This historical run is baseline evidence; later changes must use their own executed validation rather than assuming this run covers them.

## Performance Strategy

Correctness uses the generic memory seam. The first acceleration path is a high-base contiguous 4 GiB reservation consumed as Dynarmic fastmem when available, with fault recompilation/callback fallback retained. Direct low-VA guest-pointer identity is not required.

## Alternatives Already Rejected / Deferred

- Building a new A32 decoder/JIT instead of embedding Dynarmic: rejected for the current stack.
- Making guest pointers equal host pointers as a generic invariant: rejected.
- Folding dynamic-linker semantics into ELF mapping: rejected; preserve a separate linker layer.
- Silently merging overlapping load pages or broadening permissions: deferred until a concrete compatibility requirement justifies a designed policy.

## Assumptions

- Future linker/ABI/runtime layers can consume logical guest addresses through the existing memory boundary.
- The current real fixture is representative enough to validate ELF mechanics, not application compatibility.
- Additional Android device/kernel/page-size evidence will be gathered independently from feature implementation.
