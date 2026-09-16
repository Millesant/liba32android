# Current State

Last updated: 2026-09-16
Current milestone: M3 ELF32 loading
Integration branch: `bleeding`

## Working / proven

- Dynarmic remains isolated behind the internal CPU adapter and pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.
- D-0003 remains accepted: AArch32 guest VAs are logical 32-bit values; guest pointer == host pointer is not a generic-runtime requirement.
- D-0004 remains accepted: prefer a contiguous high-base 4 GiB reservation as the first Android fastmem acceleration path when available, while retaining callback memory as the mandatory correctness fallback.
- `memory::GuestMemory` is the engine-independent CPU/memory boundary and separates instruction reads from data reads/writes plus an optional internal `fastmem_base()` capability.
- `LinearGuestMemory` remains the deterministic callback/correctness implementation and exposes no fastmem capability.
- `MappedGuestMemory` is IMPLEMENTED. It owns one contiguous 4 GiB host reservation, keeps unmapped guest pages `PROT_NONE`, tracks page mapping/permission metadata, and provides page-aligned map/protect/unmap lifecycle operations.
- The mapped backend accepts the initial ELF-like permission shapes needed for `R`, `RW`, and `RX`; write-only and execute-only mappings are intentionally rejected by this first direct-fastmem backend.
- Dynarmic fastmem integration is IMPLEMENTED behind the generic memory capability. When a backend exposes a reservation, the adapter sets `UserConfig::fastmem_pointer` and keeps `recompile_on_fastmem_failure=true`.
- Linux tests prove callback-only and fastmem-capable backends produce the expected guest-visible A32 load/store behavior, including fastmem fault -> callback fallback for unmapped guest data.
- `android_runtime_smoke` is IMPLEMENTED as an Android arm64 executable linked to the real `liba32android.so` and packaged with a Termux launcher.
- On the observed Android 16 / runtime SDK 36 / AArch64 Termux environment, A32 `mov r0,#42` executed through `liba32android` / Dynarmic's AArch64 backend and returned 42.
- On that same environment, mapped A32 `STR`/`LDR` completed with `r2=0x12345678` and stored `0x12345678`; both mapped data callback counts were zero and `a32.memory.fastmem_direct=true`.
- On that same environment, the optional unmapped-data fault test reported `memory_fault=true`, `data_read_callbacks=1`, `a32.fastmem_fault.status=PASS`, and still reached `runtime_smoke.complete=true` without a fatal process crash.
- The shared runtime produces exactly `liba32android.so`; CI rejects the former duplicated filename.
- Raw Android runtime-smoke evidence is stored under `docs/research/evidence/`, including the normal direct-fastmem run and the fastmem fault/fallback run with separate evidence classification files.

## Validation / evidence status

### Real Android/AArch64 runtime evidence (2026-09-16)

Environment reported by the runtime smoke:

- Android runtime SDK: 36
- Android release: 16
- Architecture: `aarch64`
- Host page size: 4096 bytes
- Kernel release: `6.6.102-android15-8-abA566EXXSDCZHB-4k`
- High-base fastmem reservation: PASS
- A32 return-42 through Dynarmic AArch64 backend: PASS
- A32 mapped `STR`/`LDR`: PASS
- Direct fastmem data path (`data_read_callbacks=0`, `data_write_callbacks=0`): PASS
- Fastmem fault -> callback fallback on unmapped guest data: PASS
- Fault surfaced as guest `memory_fault=true`: PASS
- Fault fallback data-read callback count: observed 1
- Runtime process survived the fallback test and reached `runtime_smoke.complete=true`: PASS
- Fatal `A32CRASH|...` emission: NOT RUN because no fatal crash occurred
- Android tombstone/backtrace coexistence: NOT RUN

Normal direct-fastmem raw evidence:
- `docs/research/evidence/android-runtime-smoke-termux-arm64-2026-09-16.log`

Fault/fallback raw evidence:
- `docs/research/evidence/android-runtime-smoke-fastmem-fallback-termux-arm64-2026-09-16.log`

### Real Android/AArch64 primitive evidence (2026-09-16)

- Address-space probe execution from Termux: PASS
- Complete explicit file log creation: PASS
- `/proc/sys/vm/mmap_min_addr` read: BLOCKED by `EACCES`
- Contiguous 4 GiB reservation: PASS at a high host VA
- Commit/read/write a page inside the 4 GiB reservation: PASS
- Unmap 4 GiB reservation: PASS
- Sampled low-VA `MAP_FIXED_NOREPLACE`: PASS at all six requested addresses from `0x10000` through `0x80000000`
- Collision behavior at sampled low VAs: PASS / `EEXIST`
- RW->RX transition: PASS
- Generated AArch64 execution: PASS, result 42

### GitHub validation

PR #7 was squash-merged to `bleeding` as `e703f1b72083513581c98a32e18328a6547a45cd`.

Post-merge GitHub Actions run `35083413111` (#41): PASS.

- Linux configure/build/test: PASS
- Linux CTest: 10/10 PASS
- `mapped_guest_memory_lifecycle`: PASS
- `dynarmic_fastmem_and_fallback`: PASS
- Android `arm64-v8a` runtime + address-space probe + runtime-smoke configure/build/link: PASS
- Android shared-library filename and diagnostic checks: PASS
- Android runtime, probe, and runtime-smoke artifact publication: PASS

## Milestone status

### M2 guest address space — COMPLETE for current scope

The current M2 design now has:

- logical 32-bit guest-VA abstraction independent from host pointer identity;
- callback correctness implementation;
- mapped 4 GiB guest address-space implementation;
- page map/protect/unmap lifecycle and permission metadata;
- Dynarmic fastmem capability integration;
- Linux regression proof for direct fastmem and callback fallback;
- direct Android/AArch64 proof for A32 execution, direct mapped fastmem data access, and fastmem fault -> callback fallback on the known environment.

This completion does not imply broad Android compatibility or broad ISA completeness. Additional devices remain compatibility evidence work rather than a prerequisite for beginning M3.

## Evidence boundary

The Android evidence proves the tested paths on one Android 16 / SDK 36 AArch64 Termux environment. It does not establish broad vendor/kernel compatibility, broad A32/Thumb/VFP/NEON correctness, ELF/application compatibility, or public runtime API completeness.

Low-VA identity remains optional even though sampled low addresses succeeded in one process. The generic loader/ABI/runtime continues to use logical guest VAs.

## Partially working / not implemented

- M1 instruction coverage remains PARTIAL: basic integer register state, ARM branch/call, stack and load/store paths are tested; broader Thumb/Thumb-2, VFP/NEON, exception and edge-case coverage remains future work.
- M3 ELF32 loader: NOT IMPLEMENTED; now the active milestone.
- ARM relocations and dynamic linking: NOT IMPLEMENTED.
- Guest AAPCS32 <-> host AAPCS64 ABI bridge and `host_add(20,22)` proof: NOT IMPLEMENTED.
- libc/libm/libdl/pthread/TLS/signals/JNI/EGL/GLES/OpenSL bridges: NOT IMPLEMENTED.
- Dynarmic page-table integration: NOT IMPLEMENTED; retained only as a possible secondary acceleration path if later compatibility evidence requires it.
- Direct low-VA pointer identity remains experimental and outside the correctness contract.
- Public/app-integrated runtime logging API: NOT IMPLEMENTED; current `A32ERR`/`A32CRASH` behavior is scoped to diagnostics executables.
- Application profiles and Minecraft-specific compatibility work: NOT IMPLEMENTED.
- Project-level open-source license selection: NOT IMPLEMENTED.

## Current blocker

No blocker prevents beginning M3 ELF32 loading. Broad Android compatibility still requires additional device/vendor/kernel samples, but that work can proceed independently of the first ELF32 loader slice.
