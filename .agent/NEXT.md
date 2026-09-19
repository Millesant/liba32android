# Next Work

The current runtime baseline is merged on `bleeding` through PR #21 / commit `831f1f7dfdc11fcdcc8dc75170fe815d6141a50c`. Focused ARM/Thumb CPU seam regressions now include branch/call, memory/stack, SVC exception, instruction-fetch fault, and data-fault coverage.

1. Add a dedicated safe crash-test mode before deliberately exercising fatal crash diagnostics.
   - Status: IMPLEMENTED — exact-head CI NOT RUN.
   - Behavior: both Android diagnostic executables accept explicit `--crash-test`; runtime smoke rejects combination with `--exercise-fastmem-fault`, address-space probe rejects combination with `--execute-generated-code`.
   - Safety boundary: the crash mode refuses to abort unless all marker handlers install successfully; ordinary invocations never enter the destructive path.
   - Evidence markers: `crash_test.status=ARMED`, `crash_test.signal=SIGABRT`, then the existing `A32CRASH|...` handler marker when executed on-device.
   - CI: cross-build/static marker checks only; CI must never execute `--crash-test`.
   - Exact next action: review the branch diff, open the focused PR, and inspect exact-head GitHub Actions.
   - DoD: exact-head Linux host suite PASS, Android arm64-v8a cross-build PASS, then merge; real tombstone coexistence stays NOT RUN until the CI artifact is run in Termux.

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
