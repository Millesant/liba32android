# Next Work

Repository state is merged on `bleeding` through PR #27 / commit `3e776e4baaf9862affb2b42fb0f706292cdf179a`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Gate and execute the Fedora-native x86_64 16 KiB Android address-space probe.
   - Status: IMPLEMENTED — exact-head CI NOT RUN; Fedora project-probe execution NOT RUN.
   - Branch: `tools/x86-16k-address-probe`.
   - Environment already observed: Android 15 / SDK 35 x86_64 emulator, `PAGE_SIZE=16384`, kernel `6.6.50-android15-8-g8adecb593e9b-ab12525588`.
   - Implementation: `android_address_space_probe` now supports Android x86_64 and arm64-v8a; generated-code mode uses architecture-native return-42 bytes.
   - Harness: `tools/run_android_16k_probe_validation.sh PROBE [OUTPUT_DIR]`.
   - CI: build and upload `android-address-space-probe-x86_64-<sha>` with pinned NDK r27d.
   - Boundary: x86_64 results validate page-size/mmap/W^X/address-space behavior only; they do not validate AArch64 `liba32android`/Dynarmic behavior.
   - Exact next action: require exact-head CI PASS, then run the matching probe on the already-running Fedora 16 KiB emulator and record the resulting evidence.
   - DoD: CI x86_64 probe build PASS plus emulator harness PASS with 4 GiB reservation/commit and generated-code return 42; persist observed/inferred/not-demonstrated evidence.

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
