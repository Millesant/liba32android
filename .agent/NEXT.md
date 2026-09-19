# Next Work

The current runtime baseline is merged on `bleeding` through PR #21 / commit `831f1f7dfdc11fcdcc8dc75170fe815d6141a50c`. Focused ARM/Thumb CPU seam regressions now include branch/call, memory/stack, SVC exception, instruction-fetch fault, and data-fault coverage.

1. Add a dedicated safe crash-test mode before deliberately exercising fatal crash diagnostics.
   - Goal: validate fatal-diagnostic/tombstone coexistence without making ordinary probe or runtime-smoke execution destructive.
   - Depends on: existing crash marker handlers in `tools/android_address_space_probe.cpp` and `tools/android_runtime_smoke.cpp`.
   - Relevant: Android diagnostic tools, `docs/diagnostics.md`, CI artifact bundle.
   - Scope: explicit opt-in crash entry point/mode only; normal smoke/probe paths must remain non-crashing.
   - Cleanup: while touching these diagnostics, simplify nearby comments/helpers only where behavior becomes clearer; avoid unrelated formatting churn.
   - Validation: host suite remains PASS, Android `arm64-v8a` cross-build PASS, CI verifies the opt-in marker/path is present, and normal diagnostic invocation remains unchanged.
   - DoD: dedicated crash behavior is explicit, reproducible, documented, impossible to trigger through ordinary smoke execution, and exact-head CI PASS.
   - Termux: real fatal-signal/tombstone coexistence remains NOT RUN until the resulting CI artifact is executed on-device.

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
