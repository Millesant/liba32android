# Next Work

Repository state is merged on `bleeding` through PR #29 / commit `004bb8a42f3be2638e7a700c6c6a83bf73117a83`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Gate the Android 16 KiB ELF-alignment fix and rerun the Fedora x86_64 probe.
   - Status: FIX IMPLEMENTED — exact-head CI NOT RUN; emulator retest NOT RUN.
   - Branch: `fix/android-16k-elf-alignment`.
   - Reproduced evidence: 16 KiB x86_64 probe observed 4 GiB reservation/commit, exact low-VA mappings with `EEXIST` collision semantics, and RW->RX success; the harness then emitted Android linker warnings + SIGSEGV and never produced generated-code return-42 or final PASS.
   - Root-cause direction: pinned NDK r27d requires explicit 16 KiB ELF linker alignment flags; these were missing from project-owned Android final ELF targets.
   - Fix: apply `-Wl,-z,max-page-size=16384` and `-Wl,-z,common-page-size=16384` to `liba32android.so`, `android_address_space_probe`, and `android_runtime_smoke`; CI rejects PT_LOAD alignment below `0x4000`.
   - Exact next action: require exact-head CI PASS, rebuild/pull the fixed x86_64 probe on Fedora, confirm `readelf -lW` LOAD alignment is `0x4000`, then rerun `tools/run_android_16k_probe_validation.sh`.
   - DoD: no Android linker-warning/SIGSEGV failure, generated-code returns 42, harness emits `android_16k_probe_validation.status=PASS`, and evidence is reconciled.

2. Capture an Android native crash backtrace/tombstone for the opt-in crash test when an accessible device channel is available.
   - Current evidence: crash test armed, emitted `A32CRASH|...|signal=6|...`, and the shell reported `Aborted`.
   - Missing evidence: Android tombstone/native backtrace itself was not included in the captured output.
   - DoD: marker + Android-native crash record are captured from the same explicit `--crash-test` invocation.

3. Collect at least one materially different Android/vendor/kernel sample before making broad fastmem compatibility claims.
   - Goal: separate implementation confidence from single-device environmental evidence.
   - Depends on: access to another Android/AArch64 environment.
   - DoD: environment + reservation/fastmem/fallback observations are recorded as executed evidence.

4. Decide the project's own open-source license before public release.
   - Goal: make project redistribution terms explicit while retaining dependency-license auditability.
   - Depends on: maintainer choice.
   - DoD: license file and README/dependency notices are consistent.
