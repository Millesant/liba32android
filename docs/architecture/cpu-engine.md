# CPU engine architecture

Status: Accepted for M0 and the initial M1/M2 seam

## Boundary

The CPU engine is an internal service. It executes AArch32 instructions and interacts with guest memory only through the generic memory contract. It must not know about ELF dependency resolution, Android APIs, JNI, graphics, audio, Minecraft, or application profiles.

Dependency direction is intentionally one-way:

```text
runtime / loader / ABI layers
          |
          v
 guest address space
          |
          v
   CPU engine adapter
          |
          v
       Dynarmic
```

Application-specific code may depend on the generic runtime, never the reverse.

## Selected engine

The runtime uses Dynarmic, pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516` (2026-06-24).

The selection is based on directly observed upstream properties:

- A32 guest frontend including ARMv7 and Thumb/Thumb-2 families.
- AArch64 host backend.
- Android listed as a supported host OS.
- Bring-your-own-memory callbacks plus page-table and 4 GiB fastmem hooks.
- Cache invalidation API and configurable code cache.
- C++20 embedding API.
- 0BSD license for Dynarmic itself.
- The selected fork had a verified upstream commit in June 2026 and is used by the actively maintained Azahar emulator tree.

## CPU adapter

`src/cpu/a32_cpu.h` is the generic execution seam. Callers provide an instruction set, entry PC, initial A32 registers and a bounded instruction count. Results contain the final registers/CPSR plus exception and memory-fault state.

`src/cpu/dynarmic_cpu.cpp` owns the Dynarmic-specific `UserCallbacks` implementation. Dynarmic types do not appear in the generic CPU or memory APIs.

## Guest-memory seam

M0's fixed 4 KiB callback-owned scratch array has been removed.

`src/memory/guest_memory.h` defines `memory::GuestMemory`, an engine-independent read/write contract. The Dynarmic adapter translates all callback memory accesses through this interface and reports failed accesses through `ExecutionResult::memory_fault`.

`LinearGuestMemory` is the first concrete implementation. It provides a bounded contiguous guest range and explicit out-of-range failure. It exists to make CPU/memory integration deterministic and testable; it is not the final M2 address-space model.

The current CI test set proves basic ARM register state, branch, BL/LR, load/store and stack behavior through this seam, while retaining ARM and Thumb return-42 smoke tests.

## Future address-space design

Dynarmic exposes both a page table and `fastmem_pointer`, where fastmem can model a contiguous 4 GiB guest address space. This is relevant to the low-VA/direct-address hypothesis from reverse-engineering research, but no low-address reservation strategy is accepted yet.

Android mapping restrictions, `MAP_FIXED_NOREPLACE` behavior, ASLR collisions, executable permissions, guard regions, lifecycle and fallback behavior must be measured before M2 selects an optimized mapping strategy. Callback memory remains the correctness baseline until stronger evidence exists.

## Correctness policy

Dynarmic documents known accuracy tradeoffs and is not treated as a formal ARM reference implementation. Tiny regression binaries and, where practical, a slower reference path will be used to verify runtime behavior. Unsupported behavior must be surfaced rather than silently declared compatible.

## Sources

- https://github.com/azahar-emu/dynarmic/tree/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516
- https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/README.md
- https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/src/dynarmic/interface/A32/config.h
- https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/src/dynarmic/interface/A32/a32.h
