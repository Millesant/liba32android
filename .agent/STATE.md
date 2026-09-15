# Current State

Last updated: 2026-09-15
Current milestone: M0 Architecture / Bootstrap
Current branch: m0-bootstrap
Validated implementation commit: c74ea163a1b85d50493ece062234babdd9c267ac

## Working / proven

- Dynarmic CPU strategy is researched, selected and pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.
- The Dynarmic-specific integration is isolated behind `src/cpu/`.
- The M0 ARM guest smoke is TESTED/PASS in GitHub Actions: `guest_arm_return_42` executed guest ARM code and observed guest `r0 == 42`.
- The M0 Thumb guest smoke is TESTED/PASS in GitHub Actions: `guest_thumb_return_42` executed guest Thumb code and observed guest `r0 == 42`.
- The runtime and Dynarmic AArch64 backend are TESTED/PASS for Android `arm64-v8a` cross-compilation with NDK `27.3.13750724`.
- GitHub Actions run `34957900341` completed successfully in both jobs for validated implementation commit `c74ea163a1b85d50493ece062234babdd9c267ac`.

## Test status

- `guest_arm_return_42`: PASS — GitHub Actions run `34957900341`, job `Linux A32 smoke`; CTest reported Passed.
- `guest_thumb_return_42`: PASS — GitHub Actions run `34957900341`, job `Linux A32 smoke`; CTest reported Passed.
- Linux host configure/build: PASS — GitHub Actions run `34957900341`.
- Android `arm64-v8a` configure/build: PASS — GitHub Actions run `34957900341`, NDK `27.3.13750724`; the AArch64 Dynarmic backend and `libliba32android.so` linked successfully.
- Actual execution of the smoke test on Android/AArch64 hardware: NOT RUN.

## Evidence boundary

The ARM and Thumb execution tests above ran on the GitHub-hosted Linux x86-64 runner using Dynarmic's x86-64 backend. The Android job proves that the same runtime integration and Dynarmic AArch64 backend compile and link for Android `arm64-v8a`; it does **not** yet prove guest execution on a physical/emulated Android AArch64 device. On-device execution therefore remains NOT RUN.

## CI history / resolved failures

- Run `34956597653`: BLOCKED/FAIL at CI environment setup because `android-actions/setup-android@v3` attempted to install the obsolete SDK `tools` package. No Android code validation was obtained from that job.
- Run `34956800492`: Linux ARM/Thumb smoke tests PASS. Android configure PASS, then build FAIL because host Boost headers were not visible to Android Clang (`boost/variant.hpp` not found).
- Run `34957900341`: PASS. Android CI now uses the runner SDK directly, pins NDK `27.3.13750724`, stages Boost headers in a cross-compile-visible directory, and explicitly supplies `ARCHITECTURE=arm64` for the known `arm64-v8a` target.

## Not implemented / not proven

- Guest address-space implementation beyond the 4 KiB M0 callback-memory test scaffold: NOT IMPLEMENTED.
- Low-VA/direct mapping or Dynarmic fastmem strategy on Android: HYPOTHESIS / NOT TESTED.
- ELF32 loader/linker and ARM relocations: NOT IMPLEMENTED.
- Guest AAPCS32 <-> host AAPCS64 ABI bridge and `host_add(20,22)` proof: NOT IMPLEMENTED.
- libc/libm/libdl/pthread/TLS/signals/JNI/EGL/GLES/OpenSL bridges: NOT IMPLEMENTED.
- Application profiles and Minecraft-specific compatibility work: NOT IMPLEMENTED.
- Project-level open-source license selection: NOT IMPLEMENTED.

## Current blocker

None for M0 bootstrap. A suitable Android/AArch64 execution environment will be required to convert the on-device execution item from NOT RUN to TESTED.
