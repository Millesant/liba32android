# Next Work

The M3 loader + structural dynamic-array baseline is green on `bleeding` through PR #11 / commit `ac008b2d2a3158ffa4cb285e88cfabacea2ca4a2`. Future feature-scale work uses the repository's current requirements -> design -> tasks structure.

1. `001-elf32-linker-metadata` — implement and CI-gate T002.
   - Status: T001 semantic collection is DONE with GitHub Actions run `35329098071` (#78) PASS.
   - Goal: add checked load-bias rebasing and guest-range validation for STRTAB/SYMTAB/REL metadata, plus entry-size and string-offset validation.
   - Relevant: `src/elf/elf32_linker_metadata.{h,cpp}`, `tests/elf32_linker_metadata.cpp`, `specs/001-elf32-linker-metadata/tasks.md`.
   - Exact next action: implement T002 only; do not begin real-fixture integration (T003) in this round.
   - DoD: focused synthetic overflow/readability/entry-size/RELSZ/string-offset cases are committed and a fresh CI run is opened for the exact head.

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
