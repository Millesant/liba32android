# CPU engine architecture

Status: Accepted through the completed M2 mapped-memory/fastmem seam

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

## CPU adapter

`src/cpu/a32_cpu.h` is the generic execution seam. Callers provide an instruction set, entry PC, initial A32 registers and a bounded instruction count. Results contain final registers/CPSR plus exception and memory-fault state.

`src/cpu/dynarmic_cpu.cpp` owns the Dynarmic-specific `UserCallbacks` implementation. Dynarmic types do not appear in the generic CPU or memory APIs.

The execution result currently also carries internal diagnostics (`fastmem_enabled` and callback counters) used by regression/device-smoke tests to prove which memory path actually ran. These fields are not a public runtime ABI.

## Guest-memory seam

`src/memory/guest_memory.h` defines `memory::GuestMemory`, the engine-independent memory contract. It separates data reads/writes from instruction reads and exposes an optional internal `fastmem_base()` capability. Higher runtime layers continue to traffic only in logical 32-bit guest virtual addresses; the host reservation pointer is not a guest pointer and must not leak into loader/ABI APIs.

Two implementations currently exist:

- `LinearGuestMemory`: bounded vector-backed correctness/test implementation; no fastmem capability.
- `MappedGuestMemory`: sparse logical 32-bit address space backed by one contiguous 4 GiB host reservation.

`MappedGuestMemory` owns page mapping metadata and page-aligned `map`, `protect`, and `unmap` lifecycle operations. Unmapped pages remain `PROT_NONE`. Mapped pages are made host-accessible with `mprotect`, while guest read/write/execute permission checks remain explicit in the generic memory API. Unmap discards anonymous page contents and returns the page to `PROT_NONE` without giving up the enclosing 4 GiB reservation.

The first mapped backend intentionally accepts the normal ELF-like permission shapes `R`, `RW`, and `RX` (plus `None`) and rejects write-only/execute-only mappings. This keeps guest permission metadata compatible with the direct fastmem host protection used by this first implementation; broader permission emulation can be added if a real binary requires it.

## Fastmem integration

D-0004 selects a high-base contiguous 4 GiB reservation as the preferred first Android acceleration path, based on observed Android/AArch64 device evidence. Low host virtual addresses are not required.

When a `GuestMemory` implementation exposes `fastmem_base()`, the Dynarmic adapter sets `UserConfig::fastmem_pointer` to that reservation base and keeps `recompile_on_fastmem_failure=true`. Therefore a guest address `G` is represented by the host address `fastmem_base + G` for fast-path data accesses, while the guest-visible value remains the 32-bit `G`.

If a fastmem access hits a protected/unmapped host page, Dynarmic may recompile the block with fastmem disabled and route the access through the ordinary `GuestMemory` callbacks. The callback path remains mandatory correctness behavior, not a separate API.

Linux regression coverage proves both sides of this seam:

- callback-only `LinearGuestMemory` executes A32 load/store through data callbacks;
- `MappedGuestMemory` executes the same mapped A32 load/store with fastmem enabled and no data callbacks;
- an A32 load from an unmapped fastmem page falls back to the callback path and surfaces `memory_fault`.

Real Android/AArch64 runtime-smoke evidence from 2026-09-16 proves the tested direct and fallback paths on one Android 16 / runtime SDK 36 Termux environment:

- A32 `mov r0,#42` executed through the pinned Dynarmic AArch64 backend and returned 42;
- mapped A32 `STR`/`LDR` produced the expected `0x12345678` value;
- both mapped data callback counters were zero and `a32.memory.fastmem_direct=true`, directly supporting use of the configured fastmem data path;
- an A32 load from an unmapped guest page reported `memory_fault=true`, invoked one data-read callback, reported `a32.fastmem_fault.status=PASS`, and the process still reached `runtime_smoke.complete=true`.

The final point demonstrates the intended fastmem fault -> callback correctness fallback on that environment rather than a fatal host-process crash.

## Android runtime smoke

`android_runtime_smoke` is an Android arm64 diagnostic executable linked to the real `liba32android.so`. It is packaged with the shared library and a Termux-oriented launcher that sets `LD_LIBRARY_PATH` to the bundle directory.

The device smoke has now demonstrated on one real Android/AArch64 environment:

1. creation of `MappedGuestMemory` and its 4 GiB reservation;
2. A32 `mov r0,#42` execution through Dynarmic's AArch64 backend;
3. A32 `STR`/`LDR` through mapped fastmem with zero data callbacks;
4. fastmem fault -> callback fallback via `--exercise-fastmem-fault`, surfacing a guest memory fault without terminating the process;
5. explicit `--crash-test` SIGABRT handling, emitting the `A32CRASH` marker before fatal process termination.

Raw evidence and the Observed/Inferred/Not-demonstrated classifications live under `docs/research/evidence/android-runtime-smoke-termux-arm64-2026-09-16.*`, `docs/research/evidence/android-runtime-smoke-fastmem-fallback-termux-arm64-2026-09-16.*`, and `docs/research/evidence/android-runtime-smoke-crash-test-termux-arm64-2026-09-19.*`.

## Exact stop-PC execution

`ExecutionRequest::stop_pc` is an optional normalized logical guest PC. The
Dynarmic adapter checks it before the first instruction and after every stepped
instruction. When reached, execution stops before fetching from that address
and `ExecutionResult::stop_pc_reached` records the terminal condition.

This is additive: callers that omit `stop_pc` retain the existing fixed
instruction-count semantics. The stop target need not be mapped, which lets
bounded guest calls return through LR without staging executable sentinel code.
Exceptions and memory faults remain explicit terminal states.

Feature 019 uses this seam for INIT_ARRAY constructors; the CPU layer remains
unaware of ELF or lifecycle policy.

## Correctness policy

Dynarmic documents known accuracy tradeoffs and is not treated as a formal ARM reference implementation. Tiny regression binaries and, where practical, a slower reference path will be used to verify runtime behavior. Unsupported behavior must be surfaced rather than silently declared compatible.

The current device evidence is deliberately environment-specific. It proves these tested paths on one Android 16 / SDK 36 AArch64 Termux process; it does not establish broad device compatibility or broad A32 ISA completeness.

## Sources

- https://github.com/azahar-emu/dynarmic/tree/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516
- https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/README.md
- https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/src/dynarmic/interface/A32/config.h
- https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/src/dynarmic/interface/A32/a32.h
