# Current State

Last updated: 2026-09-15
Current milestone: M2 guest address-space research
Integration branch: `bleeding`
Last merged PR: #5 (`Persist Android probe logs and crash markers`)
Merged integration commit: `65640512f9a79fef9264cae77a3c61b3f38cac5b`
Validated PR head: `74d78e6f3015b445f6a8ebf44e4099237fb554ad`

## Working / proven

- Dynarmic remains isolated behind the internal CPU adapter and pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.
- `memory::GuestMemory` remains the engine-independent CPU/memory boundary; `LinearGuestMemory` remains the correctness-oriented initial implementation.
- D-0003 is accepted: AArch32 guest VAs remain logical 32-bit values; guest pointer == host pointer is not a generic-runtime requirement.
- Dynarmic fastmem is treated separately from low-VA pointer identity: fastmem needs a contiguous 4 GiB host range, but its host base may be above 4 GiB.
- `android_address_space_probe` is IMPLEMENTED as a standalone Android arm64-v8a diagnostic executable. It measures kernel/page information, low-4-GiB mappings, `mmap_min_addr`, a 4 GiB `PROT_NONE` reservation plus page commit, `MAP_FIXED_NOREPLACE`, RW->RX transition, and optional generated AArch64 return-42 execution.
- The probe never uses destructive `MAP_FIXED` against unowned ranges.
- The probe now mirrors stdout/stderr diagnostics into a file when possible. Default selection prefers `/sdcard/Download/liba32android/address-space-probe.log`, then `$HOME/liba32android/address-space-probe.log`, then the current working directory. Exact path is reported through `diagnostics.log_file=`.
- Probe logging can be overridden with `--log-file PATH` or `LIBA32ANDROID_LOG_FILE`; `--no-log-file` disables file output explicitly.
- The probe installs minimal async-signal-safe fatal-signal markers for SIGABRT/SIGBUS/SIGFPE/SIGILL/SIGSEGV using `A32CRASH|component=android_address_space_probe|...`, while leaving Android's normal fatal-signal/tombstone path to continue.
- The file log is opened before generated-code probing and is unbuffered, improving the chance that diagnostics survive a native crash.
- Android scoped-storage limitations are documented: direct Downloads logging is a standalone/adb-shell convenience, not a generic embedded-runtime assumption. A future host app must provide an app-appropriate writable directory/export mechanism.
- The shared runtime produces exactly `liba32android.so`; CI rejects the former duplicated `libliba32android.so` filename on Linux and Android.
- GitHub Actions uploads both the Android arm64-v8a runtime and address-space probe as artifacts.
- `docs/diagnostics.md` documents copy-pasteable CI, file-log, crash-marker and adb collection flows.
- Existing ARM/Thumb and generic-memory regression tests remain TESTED/PASS.

## Test status

Final PR #5 validation: GitHub Actions run `35016070736` (#19) on validated head `74d78e6f3015b445f6a8ebf44e4099237fb554ad`.

- Linux configure/build: PASS
- Linux shared-library filename check: PASS
- Linux CTest regression suite: PASS
- Android `arm64-v8a` runtime + address-space probe configure/build/link: PASS
- Android shared-library filename check: PASS
- Android diagnostic marker verification: PASS
- Android runtime artifact upload: PASS
- Runtime artifact ID: `10415339143`
- Runtime artifact digest: `sha256:77397f4e55074956e4ab7cef896a97170315fb90ff0481882c4bb92c5f09e903`
- Address-space probe artifact upload: PASS
- Probe artifact ID: `10416115128`
- Probe artifact size: 31,537 bytes
- Probe artifact digest: `sha256:30b677ad20c88720886a3ddefd7e3ddb66d5694874a679d025673f91120c2bae`
- Actual address-space probe execution on Android/AArch64: NOT RUN
- Actual creation of `/sdcard/Download/liba32android/address-space-probe.log`: NOT RUN
- Android fallback logging under real storage permissions/scoped storage: NOT RUN
- `A32CRASH|...` emission during a real fatal signal: NOT RUN
- Android tombstone/backtrace coexistence with the crash marker: NOT RUN
- Generated AArch64 RW->RX execution probe on Android/AArch64: NOT RUN
- Actual A32 guest execution through Dynarmic's AArch64 backend on Android: NOT RUN
- Runtime-wide public/app-integrated `A32ERR|...` logging API: NOT IMPLEMENTED.

Run #18 (`35015821583`) had Android build/link PASS but failed an initial CI-only assertion that expected the compiler to preserve the `A32CRASH` prefix as one contiguous string for `strings(1)`. That test assertion was corrected; it was not an implementation compile/link failure.

PR #5 was squash-merged into `bleeding` as `65640512f9a79fef9264cae77a3c61b3f38cac5b`.

## Evidence boundary

The Linux execution/memory tests run on the GitHub-hosted Linux x86-64 runner through Dynarmic's x86-64 backend. The Android job proves the runtime, Dynarmic AArch64 backend, correctly named shared object, enhanced diagnostic probe, and logger/crash-marker code compile/link for `arm64-v8a`; it does not prove storage permissions, crash behavior, runtime address-space behavior or guest execution on an Android device.

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

A suitable Android arm64 device/emulator execution path is required to turn address-space, shared-Downloads logging and crash-marker behavior from NOT RUN into observed device evidence. This does not block independent M1 instruction-regression work.
