# Next Work

Repository state is merged on `bleeding` through PR #25 / commit `ce5e3504765b98ca97405580e26e417e702c68de`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Collect actual 16 KiB Android host-page evidence with the PC-hosted AArch64 emulator path.
   - Status: HARNESS IMPLEMENTED — exact-head CI NOT RUN; emulator execution NOT RUN.
   - Setup: Android 15-or-newer Google APIs Experimental 16 KB Page Size ARM 64 v8a system image.
   - Gate: the running target must return `16384` from `adb shell getconf PAGE_SIZE` and `aarch64` from `uname -m`.
   - Harness: `tools/run_android_16k_validation.sh PROBE RUNTIME_SMOKE LIBA32ANDROID [OUTPUT_DIR]`.
   - Coverage: address-space probe, generated AArch64 code, normal runtime smoke/direct fastmem, and fastmem-fault -> callback fallback; destructive crash mode is excluded.
   - Exact next action: CI-gate the harness/docs branch, then run it from the user's PC against the 16 KiB ARM64 emulator using matching Android artifacts.
   - DoD: host-side evidence logs show the 16 KiB environment and all expected PASS markers; record Observed/Inferred/Not-demonstrated results under `docs/research/evidence/`.

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
