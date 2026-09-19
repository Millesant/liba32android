# Next Work

The current runtime baseline is merged on `bleeding` through PR #23 / commit `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. Exact-head CI #134 passed the Linux host suite and Android `arm64-v8a` cross-build for the opt-in crash-test diagnostics.

1. Execute the merged runtime-smoke artifact on a real Android/AArch64 Termux environment.
   - Status: NOT RUN.
   - Goal: validate the merged diagnostic behavior on-device, including the explicit fatal-signal path.
   - Depends on: the CI runtime-smoke artifact from exact-head run #134 (`35436087356`) or an equivalent artifact built from the merged baseline.
   - Sequence: run the ordinary smoke first; then invoke `./run.sh --crash-test` separately.
   - Expected pre-crash evidence: `crash_test.status=ARMED`, `crash_test.signal=SIGABRT`, and the existing `A32CRASH|component=android_runtime_smoke|signal=6|...` marker if the installed handler executes.
   - Capture: complete runtime-smoke log plus Android tombstone/native backtrace if available.
   - Safety: `--crash-test` is intentionally destructive to that process; do not combine it with `--exercise-fastmem-fault`.
   - DoD: real-device evidence is recorded; until then fatal-signal/tombstone coexistence remains NOT RUN.

2. Collect actual 16 KiB Android host-page evidence when an appropriate device/runner is available.
   - Goal: validate `MappedGuestMemory` and, if practical, the real 16 KiB-aligned ARM32 fixture on a runtime reporting 16384-byte pages.
   - Depends on: access to a suitable Android/AArch64 environment.
   - Relevant: `tools/android_address_space_probe.cpp`, `tools/android_runtime_smoke.cpp`, mapped-memory diagnostics/evidence docs.
   - DoD: executed device evidence is recorded with environment metadata; until then the status remains NOT RUN rather than inferred from ELF `p_align`.

3. Collect at least one materially different Android/vendor/kernel sample before making broad fastmem compatibility claims.
   - Goal: separate implementation confidence from single-device environmental evidence.
   - Depends on: access to another Android/AArch64 environment.
   - DoD: environment + reservation/fastmem/fallback observations are recorded as executed evidence.

4. Decide the project's own open-source license before public release.
   - Goal: make project redistribution terms explicit while retaining dependency-license auditability.
   - Depends on: maintainer choice.
   - DoD: license file and README/dependency notices are consistent.
