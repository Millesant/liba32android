# Next Work

Repository state is merged on `bleeding` through PR #30 / commit `428a76ca7d8335b0198b7a2e26c736bc8dbe198f`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Gate and merge T005 of `specs/004-elf32-dynamic-placement`.
   - Status: CONVERGENCE UPDATED; final exact-head CI NOT RUN.
   - Completed evidence: T001 #156 PASS; T002 #162 PASS; T003 #166 PASS; T004 PR-head #169 PASS at `053c6435b902eb0b0f6412b9d63a44e32274d060`.
   - Converged behavior: deterministic bounded non-mutating guest-VA search, one shared immutable ELF load plan, automatic ET_DYN placement returning an explicit loader-ready `dynamic_base`, and real pinned ARMv7 fixture auto-placement preserving `p_align=0x4000`.
   - Compatibility: `load_elf32` retains explicit-base semantics; the dependency resolver remains acquisition-only; recursive graph/link-map/loading semantics remain later work.
   - Exact next action: require the current PR-head CI to PASS, mark T005 DONE, then squash-merge PR #31 with exact-head protection and verify `bleeding`.
   - DoD: final exact-head Linux + both Android jobs PASS and no unrecorded requirements/design/code/tests/docs/state gap.

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
