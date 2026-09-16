# Current State

Last updated: 2026-09-16
Current milestone: M2 guest address space
Integration branch: `bleeding`

## Working / proven

- Dynarmic remains isolated behind the internal CPU adapter and pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.
- `memory::GuestMemory` remains the engine-independent CPU/memory boundary; `LinearGuestMemory` remains the correctness-oriented initial implementation.
- D-0003 remains accepted: AArch32 guest VAs are logical 32-bit values; guest pointer == host pointer is not a generic-runtime requirement.
- D-0004 is accepted from first real Android/AArch64 evidence: prefer a contiguous high-base 4 GiB reservation as the first Android fastmem acceleration path when available, while retaining callback memory as the mandatory correctness fallback.
- Raw first-device evidence is stored in `docs/research/evidence/android-termux-arm64-2026-09-16.log`; interpretation and evidence boundaries are stored beside it in `.md` form.
- On the observed real Android/AArch64 Termux process, a contiguous 4 GiB `PROT_NONE` reservation succeeded above 4 GiB, one page inside it was committed read/write successfully, and the reservation unmapped successfully.
- On that process, all six sampled `MAP_FIXED_NOREPLACE` low-VA requests (`0x10000` through `0x80000000`) mapped exactly; a second mapping at each occupied address failed with `EEXIST`.
- On that process, RW->RX `mprotect` succeeded and generated AArch64 code returned 42 (`jit_wx.result_check=PASS`).
- Explicit file logging through the Termux storage path succeeded and preserved the complete probe output.
- The probe installs minimal async-signal-safe fatal-signal markers for SIGABRT/SIGBUS/SIGFPE/SIGILL/SIGSEGV using `A32CRASH|component=android_address_space_probe|...`, while leaving Android's normal fatal-signal/tombstone path to continue.
- The shared runtime produces exactly `liba32android.so`; CI rejects the former duplicated filename.
- Existing ARM/Thumb and generic-memory regression tests remain TESTED/PASS in GitHub Actions.

## Validation / evidence status

### Real Android/AArch64 device evidence (2026-09-16)

- Probe execution from Termux: PASS
- Architecture: observed `aarch64`
- Host page size: observed 4096 bytes
- Complete explicit file log creation: PASS
- Default direct `/sdcard/Download/liba32android/...` selection in the Termux process: unavailable; explicit Termux Downloads path worked
- `/proc/sys/vm/mmap_min_addr` read: BLOCKED by `EACCES`
- Low mappings present before probes: observed count 0 below 4 GiB
- Contiguous 4 GiB reservation: PASS at `0x7888058000` (high host VA)
- Commit/read/write a page inside the 4 GiB reservation: PASS
- Unmap 4 GiB reservation: PASS
- Sampled low-VA `MAP_FIXED_NOREPLACE`: PASS at all six requested addresses
- Collision behavior at sampled low VAs: PASS / `EEXIST`
- RW->RX transition: PASS
- Generated AArch64 execution: PASS, result 42
- `A32CRASH|...` fatal-signal emission: NOT RUN (no crash occurred)
- Android tombstone/backtrace coexistence: NOT RUN
- Actual A32 guest execution through Dynarmic's AArch64 backend on Android: NOT RUN
- Dynarmic fastmem fault/fallback behavior through `liba32android` on Android: NOT RUN

### GitHub Actions baseline

Latest fully validated integration checkpoint before this evidence round: run `35016438405` (#22) on `f6b0b666d9619953a7d1b6e9367c2a4935f8075e`.

- Linux configure/build: PASS
- Linux shared-library filename check: PASS
- Linux CTest: 8/8 PASS
- Android arm64-v8a runtime + probe configure/build/link: PASS
- Android shared-library filename check: PASS
- Android diagnostic marker verification: PASS
- Android runtime artifact upload: PASS
- Android probe artifact upload: PASS

## Evidence boundary

The real-device result demonstrates feasibility in one Termux process on one Android/AArch64 environment. It is sufficient to select the first implementation direction, but it is not a broad device-compatibility claim.

The raw line `android.api=26` in probe version 2 is the compile-time `__ANDROID_API__`/NDK target, not proof of the device runtime SDK level. A future probe revision should rename this metadata and report runtime Android SDK/release separately.

Low-VA identity remains optional even though the six sampled addresses succeeded. Different app processes may have different low-address occupancy and policy constraints. The generic loader/ABI/runtime must continue to use logical guest VAs.

## Partially working / not implemented

- M1 instruction coverage is PARTIAL: basic integer register state, ARM branch/call, stack and load/store paths are tested; broader Thumb/Thumb-2, VFP/NEON, exception and edge-case coverage remains future work.
- M2 guest address space is PARTIAL: strategy is now selected, but mapped-region lifecycle, permissions, host reservation ownership and the production backend are NOT IMPLEMENTED.
- Callback memory: IMPLEMENTED as correctness path.
- High-base 4 GiB fastmem strategy: SELECTED / device primitive PROVEN on one environment; runtime integration NOT IMPLEMENTED.
- Dynarmic page-table integration: NOT IMPLEMENTED; retained as a possible secondary/fallback acceleration path.
- Direct low-VA pointer identity: device samples PROVEN on one process, but remains experimental and outside the correctness contract.
- ELF32 loader/linker and ARM relocations: NOT IMPLEMENTED.
- Guest AAPCS32 <-> host AAPCS64 ABI bridge and `host_add(20,22)` proof: NOT IMPLEMENTED.
- libc/libm/libdl/pthread/TLS/signals/JNI/EGL/GLES/OpenSL bridges: NOT IMPLEMENTED.
- Public/app-integrated `A32ERR|...` logging API: NOT IMPLEMENTED.
- Application profiles and Minecraft-specific compatibility work: NOT IMPLEMENTED.
- Project-level open-source license selection: NOT IMPLEMENTED.

## Current blocker

There is no external blocker to implementing the first mapped guest-address-space backend and internal fastmem seam. Additional devices are still needed before making broad Android compatibility claims, and a real-device run is still required to prove Dynarmic A32 execution through its AArch64 backend.
