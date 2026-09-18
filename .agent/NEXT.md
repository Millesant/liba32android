# Next Work

The M3 loader + structural dynamic-array baseline is green on `bleeding` through PR #11 / commit `ac008b2d2a3158ffa4cb285e88cfabacea2ca4a2`. Future feature-scale work uses the repository's current requirements -> design -> tasks structure.

1. `001-elf32-linker-metadata` — complete T005 final exact-head CI gate.
   - Status: T001 PASS (#78), T002 PASS (#84), T003 PASS (#87), and T004 docs/state convergence DONE on `m4-elf32-linker-metadata`.
   - Relevant: PR #13, full feature diff, `docs/architecture/elf32-linker-metadata.md`, README, and all linker-metadata tests.
   - Exact next action: run/inspect GitHub Actions for the current documentation-converged PR head. Fix only regressions attributable to this feature.
   - DoD: Linux configure/build plus all CTest cases PASS, reproducible ARM32 fixture integration PASS, exact shared-library naming PASS, Android `arm64-v8a` runtime/diagnostics cross-build PASS. Then mark T005 DONE and the feature ready to merge.
   - Termux: still no new device run is required; this feature does not yet expose a runtime/device path that exercises linker metadata on-device.

2. Collect actual 16 KiB Android host-page evidence when an appropriate device/runner is available.
   - Goal: validate `MappedGuestMemory` and, if practical, the real 16 KiB-aligned ARM32 fixture on a runtime reporting 16384-byte pages.
   - Depends on: access to a suitable Android/AArch64 environment.
   - Relevant: `tools/android_address_space_probe.cpp`, `tools/android_runtime_smoke.cpp`, mapped-memory diagnostics/evidence docs.
   - DoD: executed device evidence is recorded with environment metadata; until then the status remains NOT RUN rather than inferred from ELF `p_align`.

3. Correct standalone address-space probe environment metadata.
   - Goal: distinguish compile-time NDK API from runtime Android SDK/release consistently with `android_runtime_smoke`.
   - Depends on: none.
   - Relevant: `tools/android_address_space_probe.cpp`, `tools/android_runtime_smoke.cpp`, `docs/diagnostics.md`.
   - DoD: output names no longer label compile-time API as runtime API; cross-build PASS; docs updated.

4. Add the next focused CPU regressions independently of linker work.
   - Goal: cover remaining high-value A32 seam behavior before more runtime layers depend on it.
   - Depends on: current CPU adapter/memory tests.
   - Relevant: `tests/cpu_execution.cpp` and neighboring CPU tests.
   - Scope: Thumb branch/call, Thumb memory/stack, targeted exception and invalid-code/memory behavior.
   - DoD: focused regressions PASS on the host suite and preserve Android cross-build.

5. Collect at least one materially different Android/vendor/kernel sample before making broad fastmem compatibility claims.
   - Goal: separate implementation confidence from single-device environmental evidence.
   - Depends on: access to another Android/AArch64 environment.
   - DoD: environment + reservation/fastmem/fallback observations are recorded as executed evidence.

6. Add a dedicated safe crash-test mode before deliberately exercising fatal crash diagnostics.
   - Goal: validate fatal diagnostics without turning ordinary probes into destructive tests.
   - Depends on: explicit isolated crash-test entry point.
   - DoD: crash behavior is opt-in, reproducible, documented, and cannot be triggered by normal smoke execution.

7. Decide the project's own open-source license before public release.
   - Goal: make project redistribution terms explicit while retaining dependency-license auditability.
   - Depends on: maintainer choice.
   - DoD: license file and README/dependency notices are consistent.
