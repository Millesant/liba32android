# Next Work

Repository state is merged on `bleeding` through PR #30 / commit `428a76ca7d8335b0198b7a2e26c736bc8dbe198f`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Gate T003 of `specs/004-elf32-dynamic-placement`: automatic ET_DYN placement.
   - Status: IMPLEMENTED through `1bb455af587bc1742cf5fd0ce343628a65fd36b2`; latest branch-head CI PENDING, validation NOT RUN.
   - Predecessors: T001 DONE with CI #156 PASS; T002 DONE with PR-head CI #162 PASS.
   - Behavior: valid ET_DYN images are planned once through `Elf32LoadPlan`, searched low-to-high through the non-mutating guest-VA allocator, and return an explicit `dynamic_base` without mapping guest pages.
   - Coverage added: first-fit + unchanged loader acceptance, occupied-candidate skip, bounded `NoSpace`, malformed-image propagation, ET_EXEC rejection, invalid-window rejection, and non-mutation.
   - Exact next action: inspect the current PR-head CI; if PASS, mark T003 DONE and begin T004 real ARM32 fixture auto-placement integration in a fresh bounded round.
   - DoD: focused placement test + full Linux CTest + both Android cross-build jobs PASS before T004 begins.

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
