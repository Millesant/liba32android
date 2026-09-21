# Next Work

Repository state is merged on `bleeding` through PR #27 / commit `3e776e4baaf9862affb2b42fb0f706292cdf179a`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Implement and execute the Fedora-native x86_64 16 KiB Android address-space evidence path.
   - Status: NOT IMPLEMENTED / NOT RUN.
   - Local host: native Fedora 44 on x86_64; use KVM rather than WSL/nested virtualization.
   - Implementation slice: allow `android_address_space_probe` to build for Android x86_64, emit the real architecture, execute architecture-native return-42 generated code, publish an x86_64 probe CI artifact, and add a 16 KiB probe harness.
   - Evidence boundary: x86_64 emulator results validate Android/kernel page-size + mmap/W^X/address-space behavior only; they do not validate the AArch64 `liba32android`/Dynarmic runtime.
   - Gate: Fedora emulator must report `16384` from `adb shell getconf PAGE_SIZE` and `x86_64` from `uname -m`.
   - DoD: exact-head CI builds the x86_64 probe, then the Fedora emulator run records the 16 KiB environment plus 4 GiB reservation/commit, fixed-address behavior, RW->RX, and generated-code return-42 evidence.

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
