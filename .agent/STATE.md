# Current State

Last updated: 2026-09-15
Current milestone: M2 guest address-space research
Integration branch: `bleeding`
Last merged PR: #4 (`Fix Android shared library name and document diagnostics`)
Merged integration commit: `dffa186314637558ec4130a251a1c10ca313c5e0`
Validated PR head: `a1333d9f1f9d5af9ad0c6b1b6a3ed6c209db6c38`

## Working / proven

- Dynarmic remains isolated behind the internal CPU adapter and pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.
- `memory::GuestMemory` remains the engine-independent CPU/memory boundary; `LinearGuestMemory` remains the correctness-oriented initial implementation.
- D-0003 is accepted: AArch32 guest VAs remain logical 32-bit values; guest pointer == host pointer is not a generic-runtime requirement.
- Dynarmic fastmem is treated separately from low-VA pointer identity: fastmem needs a contiguous 4 GiB host range, but its host base may be above 4 GiB.
- `android_address_space_probe` is IMPLEMENTED as a standalone Android arm64-v8a diagnostic executable. It measures kernel/page information, low-4-GiB mappings, `mmap_min_addr`, a 4 GiB `PROT_NONE` reservation plus page commit, `MAP_FIXED_NOREPLACE`, RW->RX transition, and optional generated AArch64 return-42 execution.
- The probe never uses destructive `MAP_FIXED` against unowned ranges.
- The shared runtime now produces exactly `liba32android.so`; CI explicitly rejects the former duplicated `libliba32android.so` filename on Linux and Android.
- GitHub Actions uploads the Android arm64-v8a runtime as an artifact in addition to the address-space probe.
- `docs/diagnostics.md` documents copy-pasteable evidence collection: CI run URLs, complete probe `key=value` output, and the intended future `A32ERR|...` runtime format.
- Existing ARM/Thumb and generic-memory regression tests remain TESTED/PASS.

## Test status

GitHub Actions run `35014025702` on PR #4 / validated head `a1333d9f1f9d5af9ad0c6b1b6a3ed6c209db6c38`:

- Linux configure/build: PASS
- Linux shared-library filename check: PASS (`build/liba32android.so` exists; `build/libliba32android.so` does not)
- Linux CTest: 8/8 PASS, 0 failures
- Android `arm64-v8a` runtime + address-space probe configure/build/link: PASS
- Android shared-library filename check: PASS (`build-android/liba32android.so` exists; `build-android/libliba32android.so` does not)
- Android runtime artifact upload: PASS
- Runtime artifact ID: `10414882071`
- Runtime artifact size: 7,260,173 bytes
- Runtime artifact digest: `sha256:55cafdb60861639b5e59a2414709dc10749a0b376c26952b56bfe856633e5275`
- Address-space probe artifact upload: PASS
- Probe artifact ID: `10415465115`
- Actual address-space probe execution on Android/AArch64: NOT RUN
- Generated AArch64 RW->RX execution probe on Android/AArch64: NOT RUN
- Actual A32 guest execution through Dynarmic's AArch64 backend on Android: NOT RUN
- Runtime-wide `A32ERR|...` structured error emission: NOT IMPLEMENTED; documented as the intended public-runtime diagnostic contract.

PR #4 was subsequently squash-merged into `bleeding` as `dffa186314637558ec4130a251a1c10ca313c5e0`.

## Evidence boundary

The eight execution/memory tests run on the GitHub-hosted Linux x86-64 runner through Dynarmic's x86-64 backend. The Android job proves the runtime, Dynarmic AArch64 backend, correctly named shared object, and address-space diagnostic probe compile/link for `arm64-v8a`; it does not prove runtime address-space behavior or guest execution on Android.

The provided `libemu32.so` was statically inspected as a behavioral reference. Direct observations include AArch64 Android ELF metadata, imports for `mmap`/`mprotect`/`munmap`/`sysconf`/environment parsing, and `EMU32_ARENA_BASE`, `EMU32_ARENA_MB`, `EMU32_ARENA_LOG` strings. Exact arena semantics, `MAP_FIXED_NOREPLACE`, low-VA identity and 4 GiB reservation remain unproven for that binary.

## Partially working / not implemented

- M1 instruction coverage is PARTIAL: basic integer register state, ARM branch/call, stack and load/store paths are tested; broader Thumb/Thumb-2, VFP/NEON, exception and edge-case coverage remains future work.
- M2 guest address space is PARTIAL: the generic memory contract exists and a reproducible Android probe is built, but mapped-region lifecycle, permissions and a selected Android mapping strategy are NOT IMPLEMENTED.
- Callback memory: IMPLEMENTED as correctness path.
- Dynarmic page-table integration: NOT IMPLEMENTED.
- Dynarmic fastmem integration: NOT IMPLEMENTED; device feasibility evidence NOT RUN.
- Direct low-VA pointer identity: HYPOTHESIS / optional experimental strategy; device evidence NOT RUN.
- ELF32 loader/linker and ARM relocations: NOT IMPLEMENTED.
- Guest AAPCS32 <-> host AAPCS64 ABI bridge and `host_add(20,22)` proof: NOT IMPLEMENTED.
- libc/libm/libdl/pthread/TLS/signals/JNI/EGL/GLES/OpenSL bridges: NOT IMPLEMENTED.
- Application profiles and Minecraft-specific compatibility work: NOT IMPLEMENTED.
- Project-level open-source license selection: NOT IMPLEMENTED.

## Current blocker

A suitable Android arm64 device/emulator execution path is required to turn the address-space strategy measurements from NOT RUN into observed device evidence. This does not block independent M1 instruction-regression work.
