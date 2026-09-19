# Next Work

The current runtime baseline is merged on `bleeding` through PR #24 / commit `240cafced7c935edc6ec9a228e7d757809f0b2bc`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Collect actual 16 KiB Android host-page evidence when an appropriate device/runner is available.
   - Status: NOT RUN / environment unavailable in the current 4096-byte-page Termux sample.
   - Goal: validate `MappedGuestMemory` and, if practical, the real 16 KiB-aligned ARM32 fixture on a runtime reporting 16384-byte pages.
   - Relevant: `tools/android_address_space_probe.cpp`, `tools/android_runtime_smoke.cpp`, mapped-memory diagnostics/evidence docs.
   - DoD: executed device evidence is recorded with environment metadata; until then the status remains NOT RUN rather than inferred from ELF `p_align`.

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
