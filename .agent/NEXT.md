# Next Work

Repository state is merged on `bleeding` through PR #30 / commit `428a76ca7d8335b0198b7a2e26c736bc8dbe198f`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Implement T001 of `specs/004-elf32-dynamic-placement`: deterministic free guest-range search.
   - Status: SPEC READY; implementation NOT RUN.
   - Branch: `m4-elf32-dynamic-placement`.
   - Goal: add `src/memory/guest_va_allocator.{h,cpp}` as a non-mutating low-to-high first-fit search over `const MappedGuestMemory&`.
   - Inputs: explicit search window, length, alignment and alignment offset; all arithmetic checked.
   - Invariants: no ELF concepts in the memory primitive, no mapping/protection mutation, no guest==host pointer assumption.
   - Validation: first-fit, conflict skip, multi-page conflict, non-zero congruence offset, 16 KiB-equivalent alignment, invalid/overflow/no-space cases, and proof mapped state/permissions are unchanged.
   - DoD: focused host test PASS and neighboring Linux build/CTest PASS before moving to T002 shared ELF load-layout planning.

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
