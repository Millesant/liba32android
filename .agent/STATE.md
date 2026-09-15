# Current State

Last updated: 2026-09-15
Current milestone: M1/M2 CPU + guest-memory foundation
Integration branch: `bleeding`
Last merged PR: #2 (`M1/M2: generic guest memory and A32 execution seam`)
Merged integration commit: `f1224743e9f52e1bdfca7e4c1ea2e39f0487d942`
Validated PR head: `48602ec5fafa1a6107e628d2f44f68a6cd12254c`

## Working / proven

- Dynarmic remains isolated behind the internal CPU adapter and pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.
- The hidden M0 4 KiB callback scratch buffer has been replaced by the engine-independent `memory::GuestMemory` contract.
- `LinearGuestMemory` provides a bounded contiguous AArch32 guest-memory implementation with explicit read/write failure outside its mapped range.
- The generic A32 execution seam accepts initial registers, entry PC, instruction set and a bounded instruction count, and reports register/CPSR state, exceptions and memory faults without exposing Dynarmic types.
- ARM and Thumb return-42 regression smokes remain TESTED/PASS.
- Focused ARM tests for register state, branch, BL/LR, memory load/store and stack use are TESTED/PASS.
- Guest-memory boundary validation is TESTED/PASS.
- Android `arm64-v8a` configure/build/link remains TESTED/PASS with NDK `27.3.13750724`.

## Test status

Final pre-merge GitHub Actions validation: run `34992539556` on PR #2, head `48602ec5fafa1a6107e628d2f44f68a6cd12254c`.

- `guest_arm_return_42`: PASS
- `guest_thumb_return_42`: PASS
- `guest_register_state`: PASS
- `guest_branch`: PASS
- `guest_call`: PASS
- `guest_memory_load_store`: PASS
- `guest_stack`: PASS
- `guest_memory_bounds`: PASS
- Linux configure/build: PASS
- Android `arm64-v8a` configure/build/link: PASS
- Actual execution on Android/AArch64 hardware or emulator: NOT RUN

CTest reported 8/8 passed with 0 failures. PR #2 was subsequently squash-merged into `bleeding` as `f1224743e9f52e1bdfca7e4c1ea2e39f0487d942`.

## Evidence boundary

The eight execution/memory tests run on the GitHub-hosted Linux x86-64 runner. The Android job proves the same runtime and Dynarmic AArch64 backend compile and link for `arm64-v8a`; it does not yet prove guest execution through the AArch64 backend on Android.

`LinearGuestMemory` proves the generic callback seam and bounded address checks only. It does not prove low-address host mappings, a 4 GiB reservation, page-table mode, fastmem, mapping permissions, guard pages or Android-specific address-space behavior.

## Partially working / not implemented

- M1 instruction coverage is PARTIAL: basic integer register state, ARM branch/call, stack and load/store paths are tested; broader Thumb/Thumb-2, VFP/NEON, exception and edge-case coverage remains future work.
- M2 guest address space is PARTIAL: a generic memory contract and linear test implementation exist, but mapped regions, permissions, lifecycle and Android mapping strategy are NOT IMPLEMENTED.
- Low-VA/direct mapping or Dynarmic fastmem strategy on Android: HYPOTHESIS / NOT TESTED.
- ELF32 loader/linker and ARM relocations: NOT IMPLEMENTED.
- Guest AAPCS32 <-> host AAPCS64 ABI bridge and `host_add(20,22)` proof: NOT IMPLEMENTED.
- libc/libm/libdl/pthread/TLS/signals/JNI/EGL/GLES/OpenSL bridges: NOT IMPLEMENTED.
- Application profiles and Minecraft-specific compatibility work: NOT IMPLEMENTED.
- Project-level open-source license selection: NOT IMPLEMENTED.

## Current blocker

None for the generic CPU/memory seam. Device/emulator access is required for Android/AArch64 execution evidence and Android-specific address-space experiments.
