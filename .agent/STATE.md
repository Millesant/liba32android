# Current State

Last updated: 2026-09-16
Current milestone: M2 guest address space
Integration branch: `bleeding`

## Working / proven

- Dynarmic remains isolated behind the internal CPU adapter and pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.
- D-0003 remains accepted: AArch32 guest VAs are logical 32-bit values; guest pointer == host pointer is not a generic-runtime requirement.
- D-0004 remains accepted from real Android/AArch64 evidence: prefer a contiguous high-base 4 GiB reservation as the first Android fastmem acceleration path when available, while retaining callback memory as the mandatory correctness fallback.
- `memory::GuestMemory` remains the engine-independent CPU/memory boundary and now separates instruction reads from data reads/writes plus an optional internal `fastmem_base()` capability.
- `LinearGuestMemory` remains the deterministic callback/correctness implementation and exposes no fastmem capability.
- `MappedGuestMemory` is IMPLEMENTED. It owns one contiguous 4 GiB host reservation, keeps unmapped guest pages `PROT_NONE`, tracks page mapping/permission metadata, and provides page-aligned map/protect/unmap lifecycle operations.
- The mapped backend accepts the initial normal ELF-like permission shapes needed for `R`, `RW`, and `RX` mappings; write-only and execute-only mappings are intentionally rejected by this first direct-fastmem backend.
- Unmapping discards anonymous page contents and returns the page to `PROT_NONE` while preserving the enclosing 4 GiB reservation.
- Dynarmic fastmem integration is IMPLEMENTED behind the generic memory capability. When a backend exposes a reservation, the adapter sets `UserConfig::fastmem_pointer` and keeps `recompile_on_fastmem_failure=true`.
- Internal execution diagnostics record whether fastmem was enabled and how many code/data callbacks ran; these fields are test diagnostics, not a public runtime ABI.
- Linux tests prove callback-only and fastmem-capable backends produce the expected guest-visible A32 load/store behavior.
- Linux tests prove mapped fastmem data load/store executes without data callbacks and an access to an unmapped fastmem page falls back to the callback path and surfaces `memory_fault`.
- `android_runtime_smoke` is IMPLEMENTED as an Android arm64 executable linked to the real `liba32android.so`. It tests A32 return-42, A32 mapped STR/LDR, direct-fastmem callback counts, and optional fastmem-fault fallback.
- The Android runtime smoke emits `A32ERR|component=android_runtime_smoke|...` failures, `A32CRASH|component=android_runtime_smoke|...` fatal markers, runtime Android SDK/release properties, fastmem diagnostics, and a file log.
- CI publishes an `android-runtime-smoke-<sha>` bundle containing `android_runtime_smoke`, `liba32android.so`, `run.sh`, and instructions. `run.sh` sets the bundle directory in `LD_LIBRARY_PATH` for Termux execution.
- The shared runtime still produces exactly `liba32android.so`; CI rejects the former duplicated filename.
- Raw first-device address-space evidence remains stored in `docs/research/evidence/android-termux-arm64-2026-09-16.log` with analysis beside it.

## Validation / evidence status

### Real Android/AArch64 primitive evidence (2026-09-16)

- Address-space probe execution from Termux: PASS
- Architecture: observed `aarch64`
- Host page size: observed 4096 bytes
- Complete explicit file log creation: PASS
- `/proc/sys/vm/mmap_min_addr` read: BLOCKED by `EACCES`
- Contiguous 4 GiB reservation: PASS at a high host VA
- Commit/read/write a page inside the 4 GiB reservation: PASS
- Unmap 4 GiB reservation: PASS
- Sampled low-VA `MAP_FIXED_NOREPLACE`: PASS at all six requested addresses from `0x10000` through `0x80000000`
- Collision behavior at sampled low VAs: PASS / `EEXIST`
- RW->RX transition: PASS
- Generated AArch64 execution: PASS, result 42
- `A32CRASH|...` fatal-signal emission: NOT RUN (no crash occurred)
- Android tombstone/backtrace coexistence: NOT RUN

### Mapped-memory / fastmem GitHub validation

Feature validation run: GitHub Actions `35080732804` (#27) for the mapped-fastmem/runtime-smoke implementation checkpoint.

- Linux configure/build: PASS
- Linux shared-library filename check: PASS
- Linux CTest: 10/10 PASS
  - existing 8 ARM/Thumb/memory tests: PASS
  - `mapped_guest_memory_lifecycle`: PASS
  - `dynarmic_fastmem_and_fallback`: PASS
- Linux callback-only A32 load/store baseline: PASS
- Linux mapped A32 fastmem load/store with zero data callbacks: PASS
- Linux unmapped fastmem access -> callback fallback -> `memory_fault`: PASS
- Android `arm64-v8a` runtime + address-space probe + runtime-smoke configure/build/link: PASS
- Android shared-library filename check: PASS
- Android runtime-smoke diagnostic marker verification: PASS
- Android runtime-smoke `DT_NEEDED` reference to `liba32android.so`: PASS
- Android runtime artifact upload: PASS
- Android address-space probe artifact upload: PASS
- Android runtime-smoke bundle staging/upload: PASS
- Runtime-smoke artifact ID for this validation: `10440331678`
- Runtime-smoke artifact digest: `sha256:4921e6f238a460621a2c80c7f7ec5265f13d252f21fb8015512390e16b126ca3`
- Actual A32 return-42 through Dynarmic's AArch64 backend on Android: NOT RUN
- Actual mapped A32 STR/LDR through fastmem on Android: NOT RUN
- Actual fastmem fault -> callback fallback on Android: NOT RUN

Run #25 (`35080280754`) had Linux 10/10 PASS and Android compile/link PASS but failed a CI-only assertion that required an ELF `$ORIGIN` RPATH. The Android toolchain did not emit that RPATH. Packaging was corrected to use an explicit Termux launcher with `LD_LIBRARY_PATH`; the implementation itself had compiled/linked successfully. Run #26 was superseded/cancelled by the final packaging update and is not failure evidence.

## Evidence boundary

The first real-device address-space probe demonstrates the required mmap/mprotect primitives on one Android/AArch64 Termux environment. Linux CI now proves the actual liba32android mapped-memory/fastmem and callback-fallback logic using Dynarmic's x86-64 backend.

Android CI proves the same runtime source, Dynarmic AArch64 backend, `MappedGuestMemory`, and `android_runtime_smoke` compile/link/package for arm64-v8a. It does **not** yet prove that A32 guest instructions execute correctly through Dynarmic's AArch64 backend on Android; the published runtime-smoke bundle exists specifically to obtain that evidence on the user's device.

The raw `android.api=26` line in address-space probe version 2 is the compile-time NDK target, not the device runtime SDK. `android_runtime_smoke` corrects this distinction by emitting `android.ndk_api`, `android.runtime_sdk`, and `android.release` separately. The standalone address-space probe still needs the same metadata cleanup before another sample.

Low-VA identity remains optional even though sampled low addresses succeeded in one process. The generic loader/ABI/runtime continues to use logical guest VAs.

## Partially working / not implemented

- M1 instruction coverage is PARTIAL: basic integer register state, ARM branch/call, stack and load/store paths are tested; broader Thumb/Thumb-2, VFP/NEON, exception and edge-case coverage remains future work.
- M2 guest address space is substantially advanced but still PARTIAL: mapped lifecycle and fastmem seam are implemented/tested on Linux and cross-built for Android, while real Android A32 execution and wider-device compatibility remain unproven.
- Callback memory: IMPLEMENTED / TESTED as correctness path.
- High-base 4 GiB fastmem strategy: IMPLEMENTED in the mapped backend; primitive feasibility PROVEN on one Android environment; runtime behavior TESTED on Linux; Android A32 execution NOT RUN.
- Dynarmic page-table integration: NOT IMPLEMENTED; retained as a possible secondary acceleration path if device evidence requires it.
- Direct low-VA pointer identity: sampled on one device process but remains experimental and outside the correctness contract.
- ELF32 loader/linker and ARM relocations: NOT IMPLEMENTED.
- Guest AAPCS32 <-> host AAPCS64 ABI bridge and `host_add(20,22)` proof: NOT IMPLEMENTED.
- libc/libm/libdl/pthread/TLS/signals/JNI/EGL/GLES/OpenSL bridges: NOT IMPLEMENTED.
- Public/app-integrated runtime logging API: NOT IMPLEMENTED; current `A32ERR`/`A32CRASH` behavior is scoped to diagnostics executables.
- Application profiles and Minecraft-specific compatibility work: NOT IMPLEMENTED.
- Project-level open-source license selection: NOT IMPLEMENTED.

## Current blocker

No external blocker remains for host-side M2 development. The next decisive validation requires the user's Android arm64 device to execute the published `android_runtime_smoke` bundle from Termux, because GitHub-hosted CI cross-builds Android but does not provide the required Android/AArch64 execution environment.
