# Next Work

Repository state is merged on `bleeding` through PR #30 / commit `428a76ca7d8335b0198b7a2e26c736bc8dbe198f`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Gate T004 of `specs/004-elf32-dynamic-placement`: real ARM32 fixture automatic placement + load.
   - Status: IMPLEMENTED through `40c8126cce500ebda200c47acc683e753b8fab75`; latest branch-head CI PENDING, validation NOT RUN.
   - Predecessors: T001 DONE (#156 PASS), T002 DONE (#162 PASS), T003 DONE (#166 PASS).
   - Behavior: the pinned NDK-generated ARMv7 ET_DYN fixture is planned through the shared load plan, required to report `0x4000` load-bias alignment, auto-placed without guest-memory mutation, and loaded using the exact returned `dynamic_base`.
   - Existing explicit-base real-fixture coverage remains intact.
   - CI now retains `auto-placement-evidence.txt` and requires `fixture.auto_placement.required_alignment=0x4000` plus `fixture.auto_placement.status=PASS`.
   - Exact next action: inspect the current PR-head CI; if PASS, mark T004 DONE and begin T005 feature convergence/documentation in a fresh bounded round.
   - DoD: new real-fixture auto-placement test PASS, existing real-fixture integration PASS, full Linux CTest PASS, and both Android jobs PASS.

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
