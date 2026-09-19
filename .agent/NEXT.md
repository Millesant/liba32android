# Next Work

The current runtime baseline is merged on `bleeding` through PR #17 / commit `c1f0f30d6fde7c73c93dec83f5808353a838c856`. Dependency resolution is complete through bounded host-owned image acquisition; guest placement/graph semantics remain later feature-scale work.

1. Correct standalone address-space probe environment metadata.
   - Status: IMPLEMENTED — fresh CI NOT RUN.
   - Goal: distinguish compile-time NDK/API information from runtime Android SDK/release consistently with `android_runtime_smoke`.
   - Relevant: `tools/android_address_space_probe.cpp`, `.github/workflows/ci.yml`, `docs/diagnostics.md`.
   - Implemented behavior: `android.ndk_api` reports `__ANDROID_API__`; `android.runtime_sdk` and `android.release` are read from Android system properties; CI rejects the obsolete `android.api=%d` marker.
   - Exact next action: run/inspect GitHub Actions on the current branch head; fix only metadata/diagnostic regressions if it fails.
   - Validation: Android `arm64-v8a` cross-build PASS plus existing Linux host suite PASS; GitHub Actions remains the authoritative clean gate.
   - Local fallback: the user's WSL NDK `27.3.13750724` environment can reproduce host tests and the Android cross-build if a focused local check is useful.
   - DoD: exact-head CI PASS, then merge the focused PR and reconcile state.

2. Add the next focused CPU regressions independently of linker work.
   - Goal: cover remaining high-value A32 seam behavior before more runtime layers depend on it.
   - Depends on: current CPU adapter/memory tests.
   - Relevant: `tests/cpu_execution.cpp` and neighboring CPU tests.
   - Scope: Thumb branch/call, Thumb memory/stack, targeted exception and invalid-code/memory behavior.
   - DoD: focused regressions PASS on the host suite and preserve Android cross-build.

3. Collect actual 16 KiB Android host-page evidence when an appropriate device/runner is available.
   - Goal: validate `MappedGuestMemory` and, if practical, the real 16 KiB-aligned ARM32 fixture on a runtime reporting 16384-byte pages.
   - Depends on: access to a suitable Android/AArch64 environment.
   - Relevant: `tools/android_address_space_probe.cpp`, `tools/android_runtime_smoke.cpp`, mapped-memory diagnostics/evidence docs.
   - DoD: executed device evidence is recorded with environment metadata; until then the status remains NOT RUN rather than inferred from ELF `p_align`.

4. Collect at least one materially different Android/vendor/kernel sample before making broad fastmem compatibility claims.
   - Goal: separate implementation confidence from single-device environmental evidence.
   - Depends on: access to another Android/AArch64 environment.
   - DoD: environment + reservation/fastmem/fallback observations are recorded as executed evidence.

5. Add a dedicated safe crash-test mode before deliberately exercising fatal crash diagnostics.
   - Goal: validate fatal diagnostics without turning ordinary probes into destructive tests.
   - Depends on: explicit isolated crash-test entry point.
   - DoD: crash behavior is opt-in, reproducible, documented, and cannot be triggered by normal smoke execution.

6. Decide the project's own open-source license before public release.
   - Goal: make project redistribution terms explicit while retaining dependency-license auditability.
   - Depends on: maintainer choice.
   - DoD: license file and README/dependency notices are consistent.
